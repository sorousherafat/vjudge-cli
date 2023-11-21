#ifndef LIBVCD_LIBVCD_H
#define LIBVCD_LIBVCD_H

#include <stdint.h>
#include <stdio.h>

#define LIBVCD_SIGNAL_COUNT 32
#define LIBVCD_VALUE_CHANGE_COUNT 4096
#define LIBVCD_SIGNAL_SIZE 64
#define LIBVCD_NAME_SIZE 32
#define LIBVCD_TIME_UNIT_SIZE 8
#define LIBVCD_VERSION_SIZE 64
#define LIBVCD_DATE_SIZE 64

typedef uint32_t timestamp_t;

typedef struct {
    timestamp_t timestamp;
    char value[LIBVCD_SIGNAL_SIZE];
} value_change_t;

typedef struct {
    char name[LIBVCD_NAME_SIZE];
    size_t size;
    value_change_t value_changes[LIBVCD_VALUE_CHANGE_COUNT];
    size_t changes_count;
} signal_t;

typedef struct {
    char unit[LIBVCD_TIME_UNIT_SIZE];
    size_t scale;
} timescale_t;

typedef struct {
    size_t signals_count;
    signal_t signals[LIBVCD_SIGNAL_COUNT];
    char date[LIBVCD_DATE_SIZE];
    char version[LIBVCD_VERSION_SIZE];
    timescale_t timescale;
} vcd_t;

vcd_t *libvcd_read_vcd_from_path(char *path);

char *libvcd_get_signal_value(vcd_t *vcd, char *signal_name, timestamp_t timestamp);

#endif //LIBVCD_LIBVCD_H
