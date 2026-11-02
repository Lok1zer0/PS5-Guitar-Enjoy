/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef GH_USB_PS5_H
#define GH_USB_PS5_H
#include "usb_guard.h"
extern const GhUsbOps gh_ps5_usb_ops;
int gh_ps5_scan(GhUsb *usb, unsigned *nodes, unsigned *errors, int *last_error);
#endif
