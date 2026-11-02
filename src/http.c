/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "http.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
static int equal_ci(const char *a,const char *b) {
    while(*a && *b) if(tolower((unsigned char)*a++)!=tolower((unsigned char)*b++))return 0;
    return !*a && !*b;
}
int gh_route(const char*r,size_t n,const char*t){
    if(!r||!t||n<4||n>=1024||strlen(t)!=16||memchr(r,0,n)||memcmp(r+n-4,"\r\n\r\n",4))return 0;
    char b[1024],method[8],path[128],version[16],extra;memcpy(b,r,n);b[n]=0;
    char *end=strstr(b,"\r\n");if(!end)return 0;*end=0;
    if(sscanf(b,"%7s %127s %15s %c",method,path,version,&extra)!=3 ||
       (strcmp(version,"HTTP/1.1")&&strcmp(version,"HTTP/1.0")))return 0;
    char expected[128];int route=0;
    snprintf(expected,sizeof expected,"/%s/",t);
    if(!strcmp(method,"GET")&&!strcmp(path,expected))route=1;
    snprintf(expected,sizeof expected,"/%s/state",t);
    if(!strcmp(method,"GET")&&!strcmp(path,expected))route=2;
    snprintf(expected,sizeof expected,"/%s/stop",t);
    if(!strcmp(method,"POST")&&!strcmp(path,expected))route=3;
    int length_seen=0;
    for(char *h=end+2;*h;) {
        end=strstr(h,"\r\n");if(!end)return 0;*end=0;if(!*h)break;
        char *colon=strchr(h,':');if(!colon)return 0;*colon++=0;
        while(*colon==' '||*colon=='\t')++colon;
        if(equal_ci(h,"Origin")&&strcmp(colon,"http://127.0.0.1:18765")&&strcmp(colon,"http://localhost:18765"))return 0;
        if(equal_ci(h,"Transfer-Encoding"))return 0;
        if(equal_ci(h,"Content-Length")) {
            if(length_seen++ || strcmp(colon,"0"))return 0;
        }
        h=end+2;
    }
    if(route==3&&!length_seen)return 0;
    return route;
}
