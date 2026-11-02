/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef GH_HTTP_H
#define GH_HTTP_H
#include <stddef.h>
/* 0 invalid/incomplete, 1 index, 2 state, 3 stop; paths require per-run token. */
int gh_route(const char *request,size_t len,const char *token);
#endif
