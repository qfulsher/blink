# Plan: Audio playback subsystem (SDM → PAM8302A → 4Ω speaker)

## Context

The device already lets users upload `.wav` files to `/sdcard/music/` over the web
UI, but nothing plays them. We have a **PAM8302A** analog Class-D amp. The
ESP32-C6 has **no DAC**, so we generate analog audio with the **Sigma-Delta
Modulator (SDM)** peripheral feeding an RC low-pass filter into the amp's analog
input. Goal: a new `audio` subsystem that streams a WAV off the SD card to the
SDM output, controllable from the web UI (play a file, stop, loop toggle), with
the amp's shutdown pin driven by a GPIO so it only un-mutes while playing.

Reference implementation to mirror: `$IDF_PATH/examples/peripherals/sigma_delta/sdm_dac`
(SDM + gptimer-paced per-sample updates). Confirm exact `driver/sdm.h` /
`driver/gptimer.h` API signatures against the installed v6.0.1 headers while
implementing.

> **Volume, dev/prod build modes, and power wiring** are covered in the companion
> doc [`dev-prod-build-and-power.md`](./dev-prod-build-and-power.md). The volume
> gain it describes is applied in the sample→density conversion step below (the
> `>> 8`), and `audio.h` gains `audio_set_volume` / `audio_get_volume` /
> `audio_max_volume`.

## Hardware / wiring

| Signal | ESP32-C6 GPIO | Notes |
|---|---|---|
| SDM audio out | **GPIO 10** | → RC low-pass → PAM8302A `A+` |
| Amp mute (PAM8302A `SD`) | **GPIO 11** | high = enabled; idle low = muted |

- Avoids in-use pins: LED ring (6), TM1637 (4/5), SD SPI2 (18–21).
- **RC low-pass filter** on GPIO 10 before `A+`: start with **1 kΩ series + 10 nF
  to GND** (~16 kHz corner), tune by ear. `A-` → GND.
- PAM8302A `Vin` → clean 5V (same supply concern as the SD module). Speaker to
  the amp's output terminals.

## New files

### `main/audio.h`
Init-and-go API, matching the style of `led.h` / `sd.h`:
```c
void        audio_init(void);                          // call once at startup, after sd_init()
esp_err_t   audio_play(const char *relpath, bool loop);// relpath under SD mount, e.g. "music/foo.wav"
void        audio_stop(void);
bool        audio_is_playing(void);
void        audio_current(char *out, size_t n);        // copies playing filename ("" if idle)
```

### `main/audio.c`
Constants: `AUDIO_SDM_GPIO 10`, `AUDIO_MUTE_GPIO 11`, `AUDIO_SDM_OVERSAMPLE_HZ`
(set as high as the driver allows — a few MHz; higher oversampling = less
quantization hiss after the RC filter), ring buffer size (~8 KB), file read chunk
(4 KB, heap-allocated like the upload path in `web.c`).

`audio_init()`:
- `sdm_new_channel()` on GPIO 10 (`SDM_CLK_SRC_DEFAULT`, `sample_rate_hz =
  AUDIO_SDM_OVERSAMPLE_HZ`), then enable the channel.
- `gptimer_new_timer()` at 1 MHz resolution, register an alarm callback; **don't
  start** yet (alarm period is set per-file from the WAV sample rate).
- Configure `AUDIO_MUTE_GPIO` as output, **idle low** (amp muted).
- Create a FreeRTOS **stream buffer** (the PCM byte pipe), a small **command
  queue** (`{PLAY, name, loop}` / `{STOP}`), and the playback task.

Playback task (blocks on command queue):
- On **PLAY**: `sd_fopen(relpath, "rb")` (reuse `sd.c`), parse WAV header, set
  gptimer alarm = `1_000_000 / sample_rate`, reset stream buffer, drive mute GPIO
  **high**, start gptimer. Then loop: read a chunk, convert each sample to signed
  8-bit density, `xStreamBufferSend` (blocking, but poll the command queue for
  STOP between chunks). On EOF: if `loop`, `fseek` back to the data chunk and
  continue; else wait for the buffer to drain, then stop.
- On **STOP / EOF**: stop gptimer, set SDM density 0, drive mute GPIO **low**,
  `fclose`, clear current-file state.

gptimer alarm callback (`IRAM_ATTR`, fires at the audio sample rate):
- `xStreamBufferReceiveFromISR` one byte → `sdm_channel_set_pulse_density()`.
  On underrun/empty, output density 0 (silence). Yield if a higher-prio task woke.

WAV parsing helper (in `audio.c`): validate `RIFF`/`WAVE`/`fmt `/`data`, scanning
past any extra chunks to find `data`. Support **PCM only**: 16-bit signed and
8-bit unsigned, mono or stereo (mix stereo → mono). Conversion to int8 density:
16-bit → `sample >> 8`; 8-bit → `(int)b - 128`. Reject non-PCM / unsupported with
a logged error so the web call can 4xx.

**Sample-rate guidance (document in code + UI):** the alarm ISR fires once per
sample, so 44.1 kHz = 44k interrupts/s. Recommend source files be **≤ ~16–22 kHz
mono** for clean playback and low CPU load.

## Modified files

### `main/main.c`
Add `#include "audio.h"` and call `audio_init();` right after `sd_init();`
(needs the card mounted; must run before `web_init()` so handlers can call it).

### `main/web.c`
- Generalize `parse_file_uri()` to take the URI prefix as a parameter (currently
  hardcodes `/api/files/`); update the existing `/api/files/` call sites and use
  it for the new play route.
- New routes (register in the `routes[]` table; bump `max_uri_handlers` if needed):
  - `POST /api/play/<filename>` → validate via `parse_file_uri`, build
    `"music/<name>"`, read optional `{"loop":bool}` from the JSON body (default
    false, parse with cJSON as in `led_post_handler`), call `audio_play()`. Return
    `{"playing":true,"file":...,"loop":...}` or a 4xx on bad/unsupported file.
  - `POST /api/stop` → `audio_stop()`, return `{"playing":false}`.
  - `GET  /api/play` → status `{"playing":bool,"file":...}` via `audio_is_playing`/
    `audio_current`.

### `main/CMakeLists.txt`
- Add `"audio.c"` to `idf_component_register(SRCS ...)`.
- Add `esp_driver_sdm` and `esp_driver_gptimer` to `PRIV_REQUIRES` (explicit,
  since `MINIMAL_BUILD ON` may trim transitive deps of the `driver` meta-component).

### `main/web_assets/index.html`
Add a **Music** card: `<ul id="file-list">` (populated from `/api/files`), a global
**Stop** button, and a **Loop** checkbox.

### `main/web_assets/app.js`
- `fetchFiles()` → `GET /api/files`, render each name with a **Play** button.
- `play(name)` → `POST /api/play/<name>` with `{loop: loopCheckbox.checked}`.
- `stop()` → `POST /api/stop`.
- Call `fetchFiles()` on load and again from the existing upload `xhr` "load"
  handler so a freshly uploaded file appears. Show a small now-playing status line.
- (Assets re-gzip automatically via the CMake custom command — no extra step.)

## Verification

1. **Wire** per the table above (don't forget the RC filter; without it the amp
   gets a raw bitstream — noise + heat).
2. **Build/flash** (Windows): `subst E: C:\Users\QuentinFulsher\.espressif`, then
   `E:\v6.0.1\esp-idf\export.ps1`, then `idf.py build flash monitor` (target
   `esp32c6`).
3. **Prepare a test WAV** at a modest rate, e.g.
   `ffmpeg -i in.mp3 -ar 16000 -ac 1 -acodec pcm_s16le test.wav`.
4. **End-to-end via web UI** at `http://openHatch.local`: upload `test.wav`, see it
   appear in the Music list, click **Play** → audio comes out the speaker, and the
   mute GPIO goes high only while playing (verify with the serial log / a meter).
5. **Stop** halts playback and re-mutes; **Loop** checked replays on EOF.
6. **Logs**: confirm sample rate / bits / channels parsed from the header, and no
   stream-buffer underrun spam. Test the unhappy paths: non-PCM or 24-bit file →
   clean 4xx + logged reason; play with no SD card → graceful failure (sd helpers
   already fail soft).

## Open considerations
- SDM `sample_rate_hz` (oversampling) and exact `set_pulse_density` signature: pin
  down against v6.0.1 headers / the `sdm_dac` example during implementation.
- If 16 kHz mono still sounds rough, raise the SDM oversampling rate and/or
  adjust the RC corner before considering a 2-pole filter.
