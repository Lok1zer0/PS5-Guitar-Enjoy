/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "http.h"
#include <stdio.h>
#include <string.h>
static unsigned checks,failed;
#define CHECK(x) do {++checks;if(!(x)){++failed;fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);}}while(0)
int main(void){
    const char *t="0123456789abcdef";
    const char *index="GET /0123456789abcdef/ HTTP/1.1\r\nHost: localhost\r\n\r\n";
    const char *state="GET /0123456789abcdef/state HTTP/1.1\r\nHost: localhost\r\n\r\n";
    const char *stop="POST /0123456789abcdef/stop HTTP/1.1\r\nContent-Length: 0\r\n\r\n";
    CHECK(gh_route(index,strlen(index),t)==1);
    CHECK(gh_route(state,strlen(state),t)==2);
    CHECK(gh_route(stop,strlen(stop),t)==3);
    for(size_t i=0;i<strlen(index);i++)CHECK(gh_route(index,i,t)==0);
    CHECK(gh_route(index,strlen(index),"wrong")==0);
    const char *bad[]={"GET /0123456789abcdef/stop HTTP/1.1\r\n\r\n",
      "GET /0123456789abcdef/../state HTTP/1.1\r\n\r\n",
      "GET /0123456789abcdef/ HTTP/9.9\r\n\r\n",
      "POST /0123456789abcdef/stop HTTP/1.1\r\nContent-Length: 99\r\n\r\n",
      "POST /0123456789abcdef/stop HTTP/1.1\r\nOrigin: https://evil.example\r\n\r\n"};
    for(unsigned i=0;i<sizeof bad/sizeof *bad;i++)CHECK(gh_route(bad[i],strlen(bad[i]),t)==0);
    CHECK(!gh_route(NULL,0,t));
    printf("http: %u checks, %u failures\n",checks,failed);return failed?1:0;
}
