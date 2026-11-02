/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "usb_guard.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
static unsigned checks, failed;
#define CHECK(x) do {++checks;if(!(x)){++failed;fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);}}while(0)
static const uint8_t cfg[]={9,2,34,0,1,1,0,128,50,9,4,0,0,1,3,0,0,0,9,33,17,1,0,1,34,137,0,7,5,129,3,32,0,10};
typedef struct { uint16_t vid; int rev,config_fail,input_fail,read_mode,closed,opened;
    int swapped,active_fail,receive_fail,configured,reads;
} Fake;
static int ctl(void *c,const char *n){(void)c;(void)n;return 10;}
static int dd(void *c,int f,uint16_t *v,uint16_t *p,uint16_t *r){
    Fake *x=c;*v=x->swapped&&f==11?0x054c:x->vid;*p=256;*r=(uint16_t)x->rev;return 0;
}
static int conf(void *c,int f,uint8_t *d,size_t n){
    (void)f;(void)n;if(((Fake*)c)->config_fail){errno=ENOTTY;return -1;}
    memcpy(d,cfg,sizeof cfg);return sizeof cfg;
}
static int inp(void *c,const char *n,uint8_t e){
    (void)n;Fake*x=c;CHECK(e==129);x->opened++;if(x->input_fail){errno=EBUSY;return -1;}return 11;
}
static int rd(void*c,int f,uint8_t*d,size_t n){
    (void)f;(void)n;Fake*x=c;++x->reads;
    if(x->read_mode==1){errno=EAGAIN;return -1;}
    if(x->read_mode==2){errno=ENXIO;return -1;}
    if(x->read_mode==3)return 0;
    memset(d,0,27);d[2]=15;return 27;
}
static void cl(void*c,int f){CHECK(f==10||f==11);((Fake*)c)->closed++;}
static int active(void*c,int f,const GhTarget*t){
    (void)f;CHECK(t->endpoint==129);if(((Fake*)c)->active_fail){errno=EINVAL;return -1;}return 0;
}
static int receive_mode(void*c,int f){
    (void)f;Fake*x=c;++x->configured;if(x->receive_fail){errno=ENOTTY;return -1;}return 0;
}
static const GhUsbOps ops={ctl,dd,conf,inp,active,receive_mode,rd,cl};
int main(void){
    Fake f={0};GhUsb u={.control=-1,.input=-1};uint8_t data[64];
    f.vid=0x1430;f.rev=5;
    CHECK(gh_usb_open(&ops,&f,"0.4",&u)==0);CHECK(f.opened==0&&f.closed==1);
    f.vid=0x12ba;f.rev=6;f.closed=0;
    CHECK(gh_usb_open(&ops,&f,"0.4",&u)==0);CHECK(f.opened==0&&f.closed==1);
    f.rev=5;f.config_fail=1;f.closed=0;
    CHECK(gh_usb_open(&ops,&f,"0.4",&u)==-1);CHECK(errno==ENOTTY&&f.opened==0&&f.closed==1);
    f.config_fail=0;f.input_fail=1;f.closed=0;
    CHECK(gh_usb_open(&ops,&f,"0.4",&u)==-1);CHECK(errno==EBUSY&&f.closed==1&&u.input==-1);
    f.input_fail=0;f.closed=0;
    CHECK(gh_usb_open(&ops,&f,"0.4",&u)==1);CHECK(u.input==11&&u.control==10);
    f.read_mode=1;CHECK(gh_usb_read(&ops,&f,&u,data,sizeof data)==0);CHECK(u.input==11&&f.closed==0);
    f.read_mode=0;CHECK(gh_usb_read(&ops,&f,&u,data,sizeof data)==27);
    f.read_mode=2;CHECK(gh_usb_read(&ops,&f,&u,data,sizeof data)==-1);CHECK(u.input==-1&&u.control==-1&&f.closed==2);
    gh_usb_close(&ops,&f,&u);CHECK(f.closed==2);
    CHECK(gh_usb_open(&ops,&f,"0.4",&u)==1);f.closed=0;f.read_mode=3;
    CHECK(gh_usb_read(&ops,&f,&u,data,sizeof data)==-1);CHECK(f.closed==2);
    CHECK(gh_usb_open(&ops,&f,"../bad",&u)==0);
    f.closed=0;f.configured=0;f.reads=0;f.swapped=1;
    CHECK(gh_usb_open(&ops,&f,"0.4",&u)==-1);
    CHECK(f.closed==2 && f.configured==0 && f.reads==0 && u.input==-1);
    gh_usb_close(&ops,&f,&u);
    f.swapped=0;f.active_fail=1;f.closed=0;
    CHECK(gh_usb_open(&ops,&f,"0.4",&u)==-1);
    CHECK(f.closed==2 && f.configured==0 && f.reads==0 && u.input==-1);
    gh_usb_close(&ops,&f,&u);
    f.active_fail=0;f.receive_fail=1;f.closed=0;
    CHECK(gh_usb_open(&ops,&f,"0.4",&u)==-1);
    CHECK(errno==ENOTTY && f.closed==2 && f.configured==1 && u.input==-1);
    printf("usb guard: %u checks, %u failures\n",checks,failed);return failed?1:0;
}
