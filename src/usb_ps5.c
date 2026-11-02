/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "usb_ps5.h"
#include <dev/usb/usb.h>
#include <dev/usb/usb_ioctl.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
static int control(void *ctx,const char *name) {
    (void)ctx;char path[64];snprintf(path,sizeof path,"/dev/usb/%s.0",name);
    return open(path,O_RDONLY|O_NONBLOCK);
}
static int device(void *ctx,int fd,uint16_t *v,uint16_t *p,uint16_t *r) {
    (void)ctx;struct usb_device_descriptor dd;memset(&dd,0,sizeof dd);
    if (ioctl(fd,USB_GET_DEVICE_DESC,&dd)!=0) return -1;
    *v=UGETW(dd.idVendor);*p=UGETW(dd.idProduct);*r=UGETW(dd.bcdDevice);return 0;
}
static int config(void *ctx,int fd,uint8_t *data,size_t capacity) {
    (void)ctx;struct usb_gen_descriptor gd;memset(&gd,0,sizeof gd);
    gd.ugd_data=data;gd.ugd_maxlen=(uint16_t)capacity;gd.ugd_config_index=0xff;
    if (ioctl(fd,USB_GET_FULL_DESC,&gd)!=0) return -1;
    return gd.ugd_actlen;
}
static int input(void *ctx,const char *name,uint8_t ep) {
    (void)ctx;char path[64];snprintf(path,sizeof path,"/dev/usb/%s.%u",name,ep&15);
    /* No detach, claim, device mode change or output endpoint. */
    return open(path,O_RDONLY|O_NONBLOCK);
}
static int active(void *ctx,int fd,const GhTarget *target) {
    (void)ctx;struct usb_interface_descriptor iface;struct usb_endpoint_descriptor ep;
    memset(&iface,0,sizeof iface);memset(&ep,0,sizeof ep);
    if(ioctl(fd,USB_GET_RX_INTERFACE_DESC,&iface)!=0 || ioctl(fd,USB_GET_RX_ENDPOINT_DESC,&ep)!=0)return -1;
    if(iface.bLength!=sizeof iface || iface.bDescriptorType!=UDESC_INTERFACE ||
       iface.bInterfaceNumber!=target->interface_number || iface.bAlternateSetting!=0 ||
       iface.bInterfaceClass!=UICLASS_HID || iface.bInterfaceSubClass!=0 || iface.bInterfaceProtocol!=0 ||
       ep.bLength!=sizeof ep || ep.bDescriptorType!=UDESC_ENDPOINT ||
       ep.bEndpointAddress!=target->endpoint || (ep.bmAttributes&3)!=UE_INTERRUPT ||
       UGETW(ep.wMaxPacketSize)!=target->packet_size || ep.bInterval!=target->interval) {
        errno=ENODEV;return -1;
    }
    return 0;
}
static int receive_mode(void *ctx,int fd) {
    (void)ctx;int one=1;
    /* Host-side receive policy: accept 27-byte reports on a larger packet EP.
     * This does not send an output/feature/control report to the guitar. */
    return ioctl(fd,USB_SET_RX_SHORT_XFER,&one);
}
static int read_input(void *ctx,int fd,uint8_t *d,size_t n) {(void)ctx;return (int)read(fd,d,n);}
static void close_fd(void *ctx,int fd) {(void)ctx;close(fd);}
const GhUsbOps gh_ps5_usb_ops={control,device,config,input,active,receive_mode,read_input,close_fd};
int gh_ps5_scan(GhUsb *usb,unsigned *nodes,unsigned *errors,int *last_error) {
    DIR *dir=opendir("/dev");if(!dir){*last_error=errno;++*errors;return -1;}
    struct dirent *de;unsigned examined=0;int result=0;
    *nodes=0;
    while((de=readdir(dir)) && examined<64) {
        if(strncmp(de->d_name,"ugen",4)) continue;
        ++examined;++*nodes;
        int got=gh_usb_open(&gh_ps5_usb_ops,NULL,de->d_name+4,usb);
        if(got<0){++*errors;*last_error=errno;}
        if(got==1){result=1;break;}
    }
    closedir(dir);return result;
}
