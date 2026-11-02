/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef GH_GUITAR_H
#define GH_GUITAR_H
#include <stddef.h>
#include <stdint.h>
#define GH_REPORT_SIZE 27
#define GH_BUTTONS 10
typedef struct {
    uint16_t buttons;
    uint8_t whammy, slider, hat;
    uint16_t accel[3];
} GhState;
typedef struct {
    uint8_t interface_number, endpoint, interval;
    uint16_t packet_size;
} GhTarget;
typedef struct {
    GhState state;
    uint8_t raw[GH_REPORT_SIZE];
    uint64_t reports, invalid, changes, presses[GH_BUTTONS];
    uint64_t first_us, last_us, max_gap_us;
    int has_report;
} GhTracker;
int gh_decode(const uint8_t *data, size_t len, GhState *out);
int gh_select_config(uint16_t vid, uint16_t pid, uint16_t rev,
                     const uint8_t *data, size_t len, GhTarget *out);
int gh_accept(GhTracker *tracker, const uint8_t *data, size_t len, uint64_t now_us);
void gh_session_reset(GhTracker *tracker);
#endif
