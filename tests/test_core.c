/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "guitar.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, failed;
#define CHECK(x) do { ++checks; if (!(x)) { ++failed; \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); } } while (0)
/* Synthetic USB configuration: one HID interrupt IN. Not a captured descriptor. */
static const uint8_t config[] = {
    9,2,34,0,1,1,0,0x80,50, 9,4,0,0,1,3,0,0,0,
    9,0x21,0x11,1,0,1,0x22,137,0, 7,5,0x81,3,32,0,10
};
static void parser(void) {
    uint8_t r[28] = {0}; GhState s={0}, before;
    r[2]=15; r[3]=128; r[4]=128; r[5]=128;
    r[19]=0x70; r[20]=2; r[22]=2; r[24]=2;
    CHECK(gh_decode(r,27,&s)); CHECK(s.buttons==0);
    CHECK(s.whammy==128 && s.accel[0]==624 && s.accel[1]==512 && s.accel[2]==512);
    const uint8_t physical[] = {2,4,1,8,16};
    for (unsigned i=0;i<5;i++) {
        r[0]=physical[i]; CHECK(gh_decode(r,27,&s)); CHECK(s.buttons==(1u<<i));
    }
    r[0]=6; CHECK(gh_decode(r,27,&s)); CHECK(s.buttons==3);
    r[0]=31; CHECK(gh_decode(r,27,&s)); CHECK(s.buttons==31);
    r[0]=0; r[1]=0x10; CHECK(gh_decode(r,27,&s)); CHECK(s.buttons==512);
    r[1]=3; CHECK(gh_decode(r,27,&s)); CHECK(s.buttons==384);
    r[1]=0; r[2]=0; CHECK(gh_decode(r,27,&s)); CHECK(s.buttons==32);
    r[2]=4; CHECK(gh_decode(r,27,&s)); CHECK(s.buttons==64);
    r[2]=15; r[5]=255; r[6]=0x15; r[7]=255; r[8]=255;
    CHECK(gh_decode(r,27,&s)); CHECK(s.buttons==0 && s.whammy==255 && s.slider==21);
    before=s;
    for (size_t i=0;i<27;i++) CHECK(!gh_decode(r,i,&s));
    CHECK(!gh_decode(r,28,&s)); CHECK(!memcmp(&s,&before,sizeof s));
    CHECK(!gh_decode(NULL,27,&s)); CHECK(!gh_decode(r,27,NULL));
}
static void descriptors(void) {
    GhTarget t={0}, before; uint8_t b[sizeof config];
    CHECK(gh_select_config(0x12ba,0x100,5,config,sizeof config,&t));
    CHECK(t.endpoint==0x81 && t.packet_size==32 && t.interface_number==0 && t.interval==10);
    before=t;
    CHECK(!gh_select_config(0x1430,0x4734,0x5900,config,sizeof config,&t));
    CHECK(!gh_select_config(0x054c,0x0ce6,5,config,sizeof config,&t));
    CHECK(!gh_select_config(0x12ba,0x100,6,config,sizeof config,&t));
    for(size_t i=0;i<sizeof config;i++) CHECK(!gh_select_config(0x12ba,0x100,5,config,i,&t));
    memcpy(b,config,sizeof b); b[9]=0; CHECK(!gh_select_config(0x12ba,0x100,5,b,sizeof b,&t));
    memcpy(b,config,sizeof b); b[14]=8; CHECK(!gh_select_config(0x12ba,0x100,5,b,sizeof b,&t));
    memcpy(b,config,sizeof b); b[12]=1; CHECK(!gh_select_config(0x12ba,0x100,5,b,sizeof b,&t));
    memcpy(b,config,sizeof b); b[29]=1; CHECK(!gh_select_config(0x12ba,0x100,5,b,sizeof b,&t));
    memcpy(b,config,sizeof b); b[30]=2; CHECK(!gh_select_config(0x12ba,0x100,5,b,sizeof b,&t));
    memcpy(b,config,sizeof b); b[31]=26; CHECK(!gh_select_config(0x12ba,0x100,5,b,sizeof b,&t));
    memcpy(b,config,sizeof b); b[31]=65; CHECK(!gh_select_config(0x12ba,0x100,5,b,sizeof b,&t));
    CHECK(!memcmp(&t,&before,sizeof t));
}
static void transitions(void) {
    GhTracker t={0}; uint8_t r[27]={0}; r[2]=15;
    CHECK(gh_accept(&t,r,27,1000)); r[0]=2;
    CHECK(gh_accept(&t,r,27,2000)); CHECK(gh_accept(&t,r,27,3000));
    r[0]=0; CHECK(gh_accept(&t,r,27,4000)); r[0]=2;
    CHECK(gh_accept(&t,r,27,5000));
    CHECK(t.presses[0]==2 && t.changes==3 && t.reports==5 && t.max_gap_us==1000);
    CHECK(!gh_accept(&t,r,26,6000)); CHECK(t.invalid==1 && t.reports==5 && t.state.buttons==1);
    r[0]=0; r[2]=0; CHECK(gh_accept(&t,r,27,7000)); r[2]=15;
    CHECK(gh_accept(&t,r,27,8000)); CHECK(t.presses[5]==1 && t.state.buttons==0);
    CHECK(t.has_report==1);
    gh_session_reset(&t);
    CHECK(t.has_report==0 && t.state.buttons==0 && t.last_us==0 && t.reports==7 && t.raw[2]==0);
    CHECK(!gh_accept(&t,r,26,9000));CHECK(t.has_report==0);
    r[0]=2;CHECK(gh_accept(&t,r,27,90000000));
    CHECK(t.has_report==1 && t.state.buttons==1 && t.presses[0]==3 && t.max_gap_us==2000);
}
int main(void) {
    parser(); descriptors(); transitions();
    printf("core: %u checks, %u failures\n",checks,failed); return failed?1:0;
}
