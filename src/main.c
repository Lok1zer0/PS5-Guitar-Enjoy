/* SPDX-License-Identifier: GPL-3.0-or-later
 * Browser launch sequence adapted from ps5-payload-sdk samples/browser/main.c,
 * Copyright (C) 2023 John Törnblom. See THIRD_PARTY.md.
 */
#include "guitar.h"
#include "http.h"
#include "usb_ps5.h"
#include "ui.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define PORT 18765
#define CLIENTS 4
#define RUN_US (180ull*1000000)
#define U(x) ((unsigned long long)(x))
/* Same signatures as SDK v0.45 samples/browser/main.c. */
int sceUserServiceInitialize(void *);
int sceUserServiceTerminate(void);
int sceSystemServiceLaunchWebBrowser(const char *,void *);
typedef struct { char reserved[45];char message[3075]; } NotifyRequest;
int sceKernelSendNotificationRequest(int,NotifyRequest *,unsigned long,int);
static volatile sig_atomic_t stopping;
static void stop_signal(int sig) {(void)sig;stopping=1;}
static uint64_t now_us(void) {
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC,&ts)!=0) {stopping=1;return 0;}
    return (uint64_t)ts.tv_sec*1000000+(uint64_t)ts.tv_nsec/1000;
}
static int nonblock(int fd) {
    int f=fcntl(fd,F_GETFL,0);return f<0?-1:fcntl(fd,F_SETFL,f|O_NONBLOCK);
}
static void toast(const char *text) {
    NotifyRequest n;memset(&n,0,sizeof n);snprintf(n.message,sizeof n.message,"%s",text);
    sceKernelSendNotificationRequest(0,&n,sizeof n,0);
}
typedef struct {
    int fd;size_t used,sent,total;uint64_t deadline;
    char request[1024],response[16384];
} Client;
static void drop(Client *c) {if(c->fd>=0)close(c->fd);c->fd=-1;}
static void response(Client *c,int code,const char *type,const char *body) {
    int n=snprintf(c->response,sizeof c->response,
        "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
        "Cache-Control: no-store\r\nConnection: close\r\nX-Content-Type-Options: nosniff\r\n"
        "Referrer-Policy: no-referrer\r\n\r\n%s",code,code==200?"OK":"Not Found",type,strlen(body),body);
    if(n<0 || (size_t)n>=sizeof c->response) {drop(c);return;}
    c->total=(size_t)n;c->sent=0;
}
static void state_json(char *out,size_t cap,const GhTracker *t,const GhUsb *usb,
                       uint64_t now,uint64_t end,unsigned nodes,unsigned errors,int error) {
    char raw[GH_REPORT_SIZE*3+1]={0},counts[512]={0};size_t pos=0;
    for(unsigned i=0;i<GH_REPORT_SIZE;i++)snprintf(raw+i*3,sizeof raw-i*3,"%02X%s",t->raw[i],i==26?"":" ");
    for(unsigned i=0;i<GH_BUTTONS;i++) {
        int n=snprintf(counts+pos,sizeof counts-pos,"%s%llu",i?",":"",U(t->presses[i]));
        if(n<0 || (size_t)n>=sizeof counts-pos) {out[0]=0;return;}pos+=(size_t)n;
    }
    uint64_t age=t->has_report && now>=t->last_us?(now-t->last_us)/1000:0;
    int n=snprintf(out,cap,
      "{\"version\":\"0.1.0\",\"connected\":%s,\"has_report\":%s,\"node\":\"%s\",\"endpoint\":%u,\"packet_size\":%u,"
      "\"remaining_s\":%llu,\"buttons\":%u,\"whammy\":%u,\"slider\":%u,\"accel\":[%u,%u,%u],"
      "\"reports\":%llu,\"invalid\":%llu,\"changes\":%llu,\"max_gap_us\":%llu,\"age_ms\":%llu,"
      "\"presses\":[%s],\"nodes\":%u,\"errors\":%u,\"errno\":%d,\"raw\":\"%s\"}",
      usb->input>=0?"true":"false",t->has_report?"true":"false",usb->name,usb->target.endpoint,usb->target.packet_size,
      U(end>now?(end-now+999999)/1000000:0),t->state.buttons,t->state.whammy,t->state.slider,
      t->state.accel[0],t->state.accel[1],t->state.accel[2],U(t->reports),U(t->invalid),U(t->changes),
      U(t->max_gap_us),U(age),counts,nodes,errors,error,t->has_report?raw:"");
    if(n<0 || (size_t)n>=cap)out[0]=0;
}
int main(void) {
    GhUsb usb={.control=-1,.input=-1};GhTracker tracker={0};
    static Client clients[CLIENTS];for(unsigned i=0;i<CLIENTS;i++)clients[i].fd=-1;
    unsigned nodes=0,errors=0;int last_error=0,exit_code=0;
    signal(SIGTERM,stop_signal);signal(SIGINT,stop_signal);signal(SIGPIPE,SIG_IGN);
    int server=socket(AF_INET,SOCK_STREAM,0);
    if(server<0)return 1;
    struct sockaddr_in addr;memset(&addr,0,sizeof addr);
    addr.sin_len=sizeof addr;addr.sin_family=AF_INET;addr.sin_port=htons(PORT);
    addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if(nonblock(server)<0 || bind(server,(struct sockaddr*)&addr,sizeof addr)<0 || listen(server,CLIENTS)<0) {
        printf("[GuitarEnjoy] localhost:%d indisponivel errno=%d; sem acesso USB.\n",PORT,errno);
        close(server);return 1;
    }
    uint8_t random[8];char token[17],url[96];arc4random_buf(random,sizeof random);
    for(unsigned i=0;i<8;i++)snprintf(token+i*2,3,"%02x",random[i]);
    snprintf(url,sizeof url,"http://127.0.0.1:%d/%s/",PORT,token);
    uint64_t start=now_us(),end=start+RUN_US,next_scan=start;
    if(stopping){close(server);return 1;}
    printf("[GuitarEnjoy] v0.1 diagnostico 180s: %s\n",url);
    int user_service=sceUserServiceInitialize(NULL);
    int browser=sceSystemServiceLaunchWebBrowser(url,NULL);
    printf("[GuitarEnjoy] UserService=%#x browser=%#x\n",user_service,browser);
    if(browser)toast("Guitar Enjoy: navegador nao abriu. Consulte a URL no log do loader. Limite: 3 minutos.");
    while(!stopping) {
        uint64_t now=now_us();if(stopping || now>=end)break;
        if(usb.input<0 && now>=next_scan) {
            int opened=gh_ps5_scan(&usb,&nodes,&errors,&last_error);
            next_scan=now_us()+3000000;
            if(opened==1) {
                /* New session: keep totals, clear state and timing to exclude unplug gaps. */
                gh_session_reset(&tracker);
                printf("[GuitarEnjoy] USB %s 12ba:0100 rev0005 iface=%u IN=%#x mps=%u bInterval=%u\n",
                    usb.name,usb.target.interface_number,usb.target.endpoint,usb.target.packet_size,usb.target.interval);
            }
        }
        struct pollfd fds[CLIENTS+2];
        fds[0]=(struct pollfd){.fd=server,.events=POLLIN};
        fds[1]=(struct pollfd){.fd=usb.input,.events=POLLIN};
        for(unsigned i=0;i<CLIENTS;i++)fds[i+2]=(struct pollfd){.fd=clients[i].fd,.events=clients[i].total?POLLOUT:POLLIN};
        int rc=poll(fds,CLIENTS+2,20);
        if(rc<0 && errno!=EINTR){exit_code=1;break;}
        now=now_us();if(stopping||now>=end)break;
        if(usb.input>=0 && (fds[1].revents&(POLLIN|POLLERR|POLLHUP|POLLNVAL))) {
            uint8_t report[64];
            for(unsigned i=0;i<32;i++) {
                int n=gh_usb_read(&gh_ps5_usb_ops,NULL,&usb,report,sizeof report);
                if(n<0) {
                    last_error=errno;++errors;gh_session_reset(&tracker);
                    printf("[GuitarEnjoy] desconectado errno=%d\n",last_error);break;
                }
                if(!n)break;
                gh_accept(&tracker,report,(size_t)n,now_us());
            }
            if(usb.input>=0 && (fds[1].revents&(POLLERR|POLLHUP|POLLNVAL))) {
                gh_usb_close(&gh_ps5_usb_ops,NULL,&usb);gh_session_reset(&tracker);
            }
        }
        if(fds[0].revents&POLLIN) {
            int fd=accept(server,NULL,NULL);
            if(fd>=0) {
                unsigned i;for(i=0;i<CLIENTS;i++)if(clients[i].fd<0)break;
                if(i==CLIENTS || nonblock(fd)<0)close(fd);
                else {memset(&clients[i],0,sizeof clients[i]);clients[i].fd=fd;clients[i].deadline=now+1000000;}
            }
        }
        for(unsigned i=0;i<CLIENTS;i++) {
            Client *c=&clients[i];if(c->fd<0)continue;
            if(now>=c->deadline){drop(c);continue;}
            if(!c->total && (fds[i+2].revents&POLLIN)) {
                ssize_t got=recv(c->fd,c->request+c->used,sizeof c->request-1-c->used,0);
                if(got>0) {
                    c->used+=(size_t)got;c->request[c->used]=0;
                    if(strstr(c->request,"\r\n\r\n")) {
                        int route=gh_route(c->request,c->used,token);
                        if(route==1)response(c,200,"text/html; charset=utf-8",GH_UI);
                        else if(route==2) {
                            char json[2048];state_json(json,sizeof json,&tracker,&usb,now,end,nodes,errors,last_error);
                            response(c,200,"application/json",json);
                        } else if(route==3) {
                            /* Release USB before acknowledging stop. */
                            gh_usb_close(&gh_ps5_usb_ops,NULL,&usb);
                            response(c,200,"application/json","{\"stopped\":true}");
                            send(c->fd,c->response,c->total,0);stopping=1;
                        } else response(c,404,"text/plain","Rota indisponivel");
                    } else if(c->used==sizeof c->request-1)drop(c);
                } else if(got==0 || (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR))drop(c);
            }
            if(c->fd>=0 && c->total && (fds[i+2].revents&POLLOUT)) {
                ssize_t sent=send(c->fd,c->response+c->sent,c->total-c->sent,0);
                if(sent>0){c->sent+=(size_t)sent;if(c->sent==c->total)drop(c);}
                else if(sent==0 || (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR))drop(c);
            }
            if(c->fd>=0 && (fds[i+2].revents&(POLLERR|POLLHUP|POLLNVAL)))drop(c);
        }
    }
    gh_usb_close(&gh_ps5_usb_ops,NULL,&usb);
    for(unsigned i=0;i<CLIENTS;i++)drop(&clients[i]);
    close(server);
    if(!user_service)sceUserServiceTerminate();
    printf("[GuitarEnjoy] encerrado. validos=%llu invalidos=%llu mudancas=%llu falhas=%u ultimo_errno=%d\n",
           U(tracker.reports),U(tracker.invalid),U(tracker.changes),errors,last_error);
    return exit_code;
}
