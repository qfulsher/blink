# Plan: Dev/Prod build modes, volume gating, and power

Companion to [`audio-playback-plan.md`](./audio-playback-plan.md). That doc covers
the audio subsystem itself; this doc covers **how loud it's allowed to get** and
**how the board is powered** in each mode.

## Context

We want two build flavors:

- **Dev** — the whole board (ESP32-C6 + LED ring + SD + PAM8302A amp) runs safely
  off a **laptop USB-C** port. Speaker volume is **hard-capped low** so the amp's
  current draw stays tiny and the 5V rail never browns out.
- **Prod** — speaker can go to **full volume**, powered by a dedicated **5V supply**.

The mode must be selectable **at build time without editing code** — ideally one
flag on the `idf.py` command line. Volume is gated by a compile-time ceiling so a
dev build is *physically incapable* of driving the amp hard, even if the web
volume slider is maxed.

## Build-mode flag (`PROD`)

ESP-IDF doesn't accept an arbitrary `idf.py build --prod`, but it does forward
`-D<var>=<val>` to CMake. So the supported equivalent is a `PROD` CMake cache
variable, and each mode gets its **own build directory** so the flag never bleeds
between them:

| Mode | Build dir | Command |
|---|---|---|
| Dev (default) | `build/` | `idf.py build flash monitor` |
| Prod | `build_prod/` | `idf.py -B build_prod -DPROD=1 build flash` |

Dev uses the default `build/` directory (what the VS Code ESP-IDF extension and a
bare `idf.py build` expect). Prod always lives in `build_prod/` with `-DPROD=1`.
**Convention: never pass `-DPROD=1` to the default `build/` dir** — keep prod
isolated in `build_prod/`.

### `main/CMakeLists.txt`
After `idf_component_register(...)`, translate the flag into a compile definition:
```cmake
# Build mode: dev (default, volume capped for USB power) vs prod (full volume).
#   Dev : idf.py build            (or -DPROD=0)
#   Prod: idf.py -DPROD=1 build
if(PROD)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE OPENHATCH_PROD=1)
    message(STATUS "openHatch build: PROD (full speaker volume)")
else()
    message(STATUS "openHatch build: DEV (volume capped for USB power)")
endif()
```

### Why separate build dirs
`-DPROD=1` is **cached in whatever build directory it's run against**, so if dev
and prod shared one dir, a later plain `idf.py build` would silently stay in prod
mode. Giving each mode its own dir (`build/` vs `build_prod/`) means each has its
own `CMakeCache.txt` — the flag is set once per dir and never leaks across modes.
Switching between dev and prod is then just a matter of which `-B` dir you target;
no `fullclean` or flag-flipping needed. (Cost: two build dirs = more disk + each
mode compiles independently the first time. Fine for a project this size.)

### `.gitignore`
`build/` is already ignored, but `build_prod/` is not — add a line for it:
```
/build_prod/
```

### Optional ergonomics
A tiny PowerShell wrapper can give you the `--prod` feel, e.g. `build.ps1 -Prod`
that expands to the `-B build_prod -DPROD=1` form. Nice-to-have, not required.

## Volume gating in `audio.c`

Volume is an integer gain `0..256` (256 = unity). The compile-time mode sets both
a **hard ceiling** and the **startup default**:

```c
#ifdef OPENHATCH_PROD
#define AUDIO_MAX_VOLUME      256   // full scale allowed
#define AUDIO_DEFAULT_VOLUME  180
#else
#define AUDIO_MAX_VOLUME      48    // hard cap — safe on laptop USB
#define AUDIO_DEFAULT_VOLUME  32
#endif
```

- A module-level `static int s_volume = AUDIO_DEFAULT_VOLUME;`.
- `audio_set_volume(int v)` **clamps to `[0, AUDIO_MAX_VOLUME]`** — this is the
  safety guarantee: in a dev build, even a maxed-out slider stays quiet.
- Apply the gain in the sample→density conversion (the `>> 8` step from the audio
  plan): `density = (int8_t)(((sample >> 8) * s_volume) >> 8);`
- New API in `audio.h`: `void audio_set_volume(int v);` and
  `int audio_get_volume(void);` plus `int audio_max_volume(void);`.

**Note on dev sound quality:** at the dev cap you're using only a few of the SDM's
~256 density levels, so quiet playback will sound grainy. That's expected — it's a
power-safety mode, not a fidelity mode. It cleans up at prod volume.

## Web volume control (extends `web.c` + UI)

- `GET  /api/volume` → `{"volume":v,"max":AUDIO_MAX_VOLUME}` (UI uses `max` to size
  the slider; in a dev build `max` comes back small).
- `POST /api/volume` body `{"volume":v}` → clamp via `audio_set_volume`, return the
  applied value. Parse with cJSON like `led_post_handler` in `web.c`.
- **UI** (`index.html` + `app.js`): a range slider `0..max` in the Music card;
  debounce `input` events into a `POST /api/volume`. Fetch current volume + max on
  load to initialize.

## Power & wiring per mode

### Dev mode — laptop USB-C
- Board powered from the laptop's USB-C (flash + monitor over the same cable).
- Dev build caps volume → amp draw is negligible.
- Keep the **LED ring dim or off** while testing (it's the other big draw).
- Still add the bulk caps below — they don't hurt and you'll reuse them in prod.

### Prod mode — dedicated 5V supply
- **Source:** 5V / 4A DC adapter with a 2.1mm barrel jack (LED-strip type). 4A
  gives headroom over the ~2.5–3A worst case so the rail holds under amp peaks.
- **Distribution:** barrel-jack → screw-terminal adapter → a 5V bus + GND bus;
  fan out to each load. **All grounds common.**

```
 5V/4A adapter ──▶ barrel jack ──▶ [ 5V bus ]──┬── ESP32-C6 board  5V pin
                                  [ GND bus ]──┤   (+ GND)
                                               ├── SK6812 ring   5V (+ GND)   [470–1000µF here]
                                               ├── SD module     5V (+ GND)
                                               └── PAM8302A       Vin (+ GND)  [1000µF here]
        all grounds tied together on the GND bus ▲
```

- **Bulk caps:** 1000µF across the amp `Vin`/`GND`, 470–1000µF across the ring's
  5V/GND, mounted close to each — absorb current spikes (prevents brownouts + whine).
- **Don't dual-power:** wall brick *and* laptop USB both feeding 5V will fight. Use
  a **data-only USB cable** (VBUS cut) for the serial monitor while the brick
  powers everything, or flash → unplug → run on the brick.
- **Parts:** 5V/4A barrel adapter (~$8), barrel→screw-terminal adapter (~$1),
  terminal block/power rail, 1000µF + 470–1000µF caps (≥10V), optional ferrite
  bead in the amp's 5V branch if any residual noise.

### The switch is trivial
Going dev → prod changes only: (1) **power supply**, (2) **build flag**
(`-DPROD=1`). Same circuit, same RC filter, same wiring. No code edits.

## Verification
1. **Dev:** `idf.py build flash monitor` (default `build/` dir) on laptop USB. Max
   the web volume slider — confirm it stays quiet and the board doesn't reset/brown
   out with Wi-Fi + playback running.
2. Confirm the dev ceiling is real: log `audio_max_volume()` at boot and check the
   slider can't exceed it.
3. **Prod:** wire the 5V/4A supply per the diagram, build into its own dir with
   `idf.py -B build_prod -DPROD=1 build flash`, monitor via a **data-only** cable.
   Volume slider now reaches full; confirm clean audio at volume with no brownouts.
   A bare `idf.py build` afterward still targets `build/` (dev) — the two dirs stay
   independent.
4. Sanity-check the build banner — CMake prints `openHatch build: DEV/PROD` so you
   always know which mode you flashed.
