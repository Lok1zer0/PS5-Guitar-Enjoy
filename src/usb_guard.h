/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef GH_USB_GUARD_H
#define GH_USB_GUARD_H
#include "guitar.h"
typedef struct {
    int (*control)(void *ctx, const char *name);
    int (*device)(void *ctx, int fd, uint16_t *v, uint16_t *p, uint16_t *r);
    int (*config)(void *ctx, int fd, uint8_t *data, size_t capacity);
    int (*input)(void *ctx, const char *name, uint8_t endpoint);
    int (*active)(void *ctx, int fd, const GhTarget *target);
    int (*receive_mode)(void *ctx, int fd);
    int (*read)(void *ctx, int fd, uint8_t *data, size_t capacity);
    void (*close)(void *ctx, int fd);
} GhUsbOps;
typedef struct { int control, input; GhTarget target; char name[16]; } GhUsb;
/* 1 open; 0 unrecognized/unsupported; -1 OS failure (errno retained). */
int gh_usb_open(const GhUsbOps *ops, void *ctx, const char *name, GhUsb *out);
void gh_usb_close(const GhUsbOps *ops, void *ctx, GhUsb *usb);
/* >0 bytes, 0 would block, -1 disconnected/error. */
int gh_usb_read(const GhUsbOps *ops, void *ctx, GhUsb *usb, uint8_t *data, size_t len);
#endif
