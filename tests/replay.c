/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "guitar.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
/* Literal expectations from physical capture sequence (2026-11-02).
 * Phase 1 repeats four frets; phase 2 ended with green+yellow held;
 * phase 3 includes transitional chords and two PS presses. */
int main(int argc,char **argv) {
    static const unsigned counts[3]={5975,5982,4477};
    static const unsigned presses[3][10]={{2,2,2,2,1,1,1,1,1,0},{1,0,1,0,0,0,0,0,0,0},{3,3,1,1,1,0,0,0,0,2}};
    if(argc!=3)return 2;
    int phase=atoi(argv[2])-1;if(phase<0||phase>2)return 2;
    FILE *file=fopen(argv[1],"rb");if(!file)return 2;
    GhTracker t={0};uint8_t data[27];size_t n;int failed=0;
    while((n=fread(data,1,27,file))) {
        if(n!=27 || !gh_accept(&t,data,n,t.reports*10000)){failed=1;break;}
    }
    if(ferror(file))failed=1;
    fclose(file);
    if(t.reports!=counts[phase])failed=1;
    for(unsigned i=0;i<10;i++)if(t.presses[i]!=presses[phase][i])failed=1;
    printf("capture %d: %" PRIu64 " reports, %s\n",phase+1,t.reports,failed?"FAIL":"PASS");
    return failed;
}
