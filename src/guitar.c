/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "guitar.h"
#include <string.h>
static uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1]<<8)); }
int gh_decode(const uint8_t *d, size_t n, GhState *o) {
    if (!d || !o || n!=GH_REPORT_SIZE) return 0;
    GhState s={0};
    s.buttons=(uint16_t)(((d[0]&2)>>1) | ((d[0]&4)>>1) |
                         ((d[0]&1)<<2) | (d[0]&0x18));
    s.hat=d[2]&15;
    if (s.hat==0) s.buttons|=1u<<5;
    if (s.hat==4) s.buttons|=1u<<6;
    if (d[1]&2) s.buttons|=1u<<7;
    if (d[1]&1) s.buttons|=1u<<8;
    if (d[1]&16) s.buttons|=1u<<9;
    s.whammy=d[5]; s.slider=d[6];
    for (unsigned i=0;i<3;i++) s.accel[i]=le16(d+19+2*i);
    *o=s; return 1;
}
int gh_select_config(uint16_t v, uint16_t p, uint16_t r,
                     const uint8_t *d, size_t n, GhTarget *o) {
    if (v!=0x12ba || p!=0x100 || r!=5 || !d || !o || n<9 ||
        d[0]!=9 || d[1]!=2 || le16(d+2)!=n || !d[4] || !d[5]) return 0;
    GhTarget candidate={0}; int active=0, hid=0, found=0, selected=0;
    unsigned declared=0, seen=0;
    for (size_t pos=9;pos<n;) {
        size_t len=d[pos];
        if (len<2 || len>n-pos) return 0;
        const uint8_t *b=d+pos;
        if (b[1]==4) {
            if (active && (seen!=declared || !hid)) return 0;
            if (len!=9) return 0;
            active=b[2]==0 && b[3]==0 && b[5]==3 && b[6]==0 && b[7]==0;
            hid=0; seen=0; declared=b[4];
            if (active && ++selected!=1) return 0;
        } else if (active && b[1]==0x21) {
            if (len<9 || b[5]!=1 || b[6]!=0x22 || !le16(b+7)) return 0;
            if (++hid!=1) return 0;
        } else if (active && b[1]==5) {
            if (len<7) return 0;
            ++seen;
            if ((b[2]&0x80) && (b[3]&3)==3) {
                uint16_t mps=le16(b+4);
                if (!(b[2]&15) || (b[2]&0x70) || mps<27 || mps>64 || !b[6] || ++found!=1) return 0;
                candidate.interface_number=0; candidate.endpoint=b[2];
                candidate.packet_size=mps; candidate.interval=b[6];
            }
        }
        pos+=len;
    }
    if (active && (seen!=declared || !hid)) return 0;
    if (found!=1 || selected!=1) return 0;
    *o=candidate; return 1;
}
int gh_accept(GhTracker *t, const uint8_t *d, size_t n, uint64_t u) {
    if (!t) return 0;
    GhState next;
    if (!gh_decode(d,n,&next)) { ++t->invalid; return 0; }
    uint16_t rising=(uint16_t)(next.buttons & ~t->state.buttons);
    for (unsigned i=0;i<GH_BUTTONS;i++) if (rising&(1u<<i)) ++t->presses[i];
    if (t->has_report) {
        if (memcmp(d,t->raw,GH_REPORT_SIZE)) ++t->changes;
        if (u>=t->last_us && u-t->last_us>t->max_gap_us) t->max_gap_us=u-t->last_us;
    } else if (!t->reports) t->first_us=u;
    ++t->reports; t->last_us=u; t->state=next; t->has_report=1;memcpy(t->raw,d,GH_REPORT_SIZE);
    return 1;
}
void gh_session_reset(GhTracker *t) {
    if(!t)return;
    memset(&t->state,0,sizeof t->state);memset(t->raw,0,sizeof t->raw);
    t->has_report=0;t->last_us=0;
}
