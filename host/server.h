#ifndef _SERVER_H_
#define _SERVER_H_

#include <pthread.h>

#define SERVER_ADDR "127.0.0.1"
#define BUF_SIZE 1024

struct server_info{
    char addr[255];
    int port;
    pthread_t tid;
};

int server_init(struct server_info*);
int server_uinit(struct server_info*);

#endif 