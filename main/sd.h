#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/**
 * Filesystem mount point. Use this to build absolute paths when calling into
 * POSIX I/O directly (e.g. `fopen(SD_MOUNT_POINT "/music.mp3", "rb")`).
 */
#define SD_MOUNT_POINT "/sdcard"

/**
 * Initialize the SPI bus and mount the SD card at SD_MOUNT_POINT. Fail-soft:
 * logs and returns if no card is present or mount fails. Call once at
 * startup. Subsequent helper calls will fail gracefully if the card never
 * mounted.
 */
void sd_init(void);

/**
 * True if `path` (relative to SD_MOUNT_POINT) exists.
 */
bool sd_exists(const char *path);

/**
 * Create the directory at `path` (relative to SD_MOUNT_POINT). Returns 0 on
 * success or if the directory already exists; -1 with errno set on other
 * errors.
 */
int sd_mkdir(const char *path);

/**
 * Open the file at `path` (relative to SD_MOUNT_POINT) with the given mode
 * (e.g. "rb", "wb", "ab"). Returns a FILE* the caller must close with
 * fclose(), or NULL on error (errno set).
 */
FILE *sd_fopen(const char *path, const char *mode);

/**
 * Callback invoked once per entry by sd_list_dir.
 * @param name   Entry name only — no path prefix.
 * @param is_dir True if the entry is a subdirectory.
 * @param user   Opaque pointer forwarded from sd_list_dir.
 */
typedef void (*sd_dir_cb)(const char *name, bool is_dir, void *user);

/**
 * List entries at `path` (relative to SD_MOUNT_POINT). Calls `cb` once per
 * entry. Returns 0 on success, -1 on error (errno set: ENOENT if path doesn't
 * exist, EACCES, etc.).
 */
int sd_list_dir(const char *path, sd_dir_cb cb, void *user);

/**
 * Delete the file at `path` (relative to SD_MOUNT_POINT).
 * Returns 0 on success, -1 on error.
 */
int sd_delete(const char *path);

/**
 * Overwrite-write `len` bytes from `buf` to `path` (relative to
 * SD_MOUNT_POINT). Truncates any existing file. Suitable for small payloads
 * (config, settings). For large/streaming writes use fopen on
 * SD_MOUNT_POINT "/..." directly.
 *
 * Returns the number of bytes written on success, or -1 on error.
 */
int sd_write_bytes(const char *path, const void *buf, size_t len);

/**
 * Read up to `maxlen` bytes from `path` (relative to SD_MOUNT_POINT) into
 * `buf`. Suitable for small payloads. For large/streaming reads, use fopen
 * on SD_MOUNT_POINT "/..." directly.
 *
 * Returns the number of bytes actually read on success, or -1 on error.
 */
int sd_read_bytes(const char *path, void *buf, size_t maxlen);
