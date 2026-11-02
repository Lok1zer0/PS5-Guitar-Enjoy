/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "usb_guard.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
static int valid_name(const char *s) {
    unsigned a,b;int n=0;
    return s && strlen(s)<16 && sscanf(s,"%u.%u%n",&a,&b,&n)==2 &&
        !s[n] && a<=255 && b>0 && b<=127 &&
        strspn(s,"0123456789.")==strlen(s);
}
int gh_usb_open(const GhUsbOps *o, void *c, const char *n, GhUsb *u) {
    if (!o || !u || !valid_name(n) || u->control>=0 || u->input>=0) return 0;
    int ctl=o->control(c,n); if (ctl<0) return -1;
    uint16_t v=0,p=0,r=0;uint8_t cfg[1024];GhTarget target;
    int result=0, saved=0;
    if (o->device(c,ctl,&v,&p,&r)<0) { result=-1;goto done; }
    if (v!=0x12ba || p!=0x100 || r!=5) goto done;
    int len=o->config(c,ctl,cfg,sizeof cfg);
    if (len<0) { result=-1;goto done; }
    if ((size_t)len>sizeof cfg || !gh_select_config(v,p,r,cfg,(size_t)len,&target)) goto done;
    int in=o->input(c,n,target.endpoint);
    if (in<0) {result=-1;goto done;}
    /* A pathname can be reused after unplug. Verify the descriptor on the
     * opened input handle as well as its current RX interface/endpoint. */
    if(o->device(c,in,&v,&p,&r)<0) {result=-1;goto input_fail;}
    if(v!=0x12ba || p!=0x100 || r!=5) {errno=ENODEV;result=-1;goto input_fail;}
    len=o->config(c,in,cfg,sizeof cfg);
    if(len<0) {result=-1;goto input_fail;}
    GhTarget current;
    if((size_t)len>sizeof cfg || !gh_select_config(v,p,r,cfg,(size_t)len,&current) ||
       current.endpoint!=target.endpoint || current.packet_size!=target.packet_size ||
       current.interface_number!=target.interface_number || current.interval!=target.interval) {
        errno=ENODEV;result=-1;goto input_fail;
    }
    if(o->active(c,in,&target)<0 || o->receive_mode(c,in)<0) {result=-1;goto input_fail;}
    u->control=ctl;u->input=in;u->target=target;snprintf(u->name,sizeof u->name,"%s",n);
    return 1;
input_fail:
    saved=errno;o->close(c,in);errno=saved;
done:
    saved=errno;o->close(c,ctl);errno=saved;return result;
}
void gh_usb_close(const GhUsbOps *o, void *c, GhUsb *u) {
    if (u->input>=0) o->close(c,u->input);
    if (u->control>=0) o->close(c,u->control);
    u->input=-1;u->control=-1;
}
int gh_usb_read(const GhUsbOps *o, void *c, GhUsb *u, uint8_t *d, size_t n) {
    if (u->input<0) return -1;
    int got=o->read(c,u->input,d,n);
    if (got<0 && (errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR)) return 0;
    if (got<=0 || (size_t)got>n) {
        int saved=got==0?ENXIO:errno;gh_usb_close(o,c,u);errno=saved;return -1;
    }
    return got;
}
