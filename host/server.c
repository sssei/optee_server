#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "server.h"

static int transfer(int sock) {

    int recv_size, send_size;
    char recv_buf[BUF_SIZE], send_buf;

    while (1) {

        /* クライアントから文字列を受信 */
        recv_size = recv(sock, recv_buf, BUF_SIZE, 0);
        if (recv_size == -1) {
            printf("recv error\n");
            break;
        }
        if (recv_size == 0) {
            /* 受信サイズが0の場合は相手が接続閉じていると判断 */
            printf("connection ended\n");
            break;
        }
        
        /* 受信した文字列を表示 */
        printf("%s\n", recv_buf);
        
        /* 文字列が"finish"ならクライアントとの接続終了 */
        if (strcmp(recv_buf, "finish") == 0) {

            /* 接続終了を表す0を送信 */
            send_buf = 0;
            send_size = send(sock, &send_buf, 1, 0);
            if (send_size == -1) {
                printf("send error\n");
                break;
            }
            break;
        } else {
            /* "finish"以外の場合はクライアントとの接続を継続 */
            send_buf = 1;
            send_size = send(sock, &send_buf, 1, 0);
            if (send_size == -1) {
                printf("send error\n");
                break;
            }
        }
    }

    return 0;
}

void *sock_server(void *arg) {
    int w_addr, c_sock;
    struct sockaddr_in a_addr;
    struct server_info *si = (struct server_info *)arg;

    /* ソケットを作成 */
    w_addr = socket(AF_INET, SOCK_STREAM, 0);
    if (w_addr == -1) {
        printf("socket error\n");
        exit(1);
    }

    /* 構造体を全て0にセット */
    memset(&a_addr, 0, sizeof(struct sockaddr_in));

    /* サーバーのIPアドレスとポートの情報を設定 */
    a_addr.sin_family = AF_INET;
    a_addr.sin_port = htons((unsigned short)si->port);
    a_addr.sin_addr.s_addr = inet_addr(si->addr);

    /* ソケットに情報を設定 */
    if (bind(w_addr, (const struct sockaddr *)&a_addr, sizeof(a_addr)) == -1) {
        printf("bind error\n");
        close(w_addr);
        exit(1);
    }

    /* ソケットを接続待ちに設定 */
    if (listen(w_addr, 3) == -1) {
        printf("listen error\n");
        close(w_addr);
        exit(1);
    }

    /* 接続要求の受け付け（接続要求くるまで待ち） */
    printf("Waiting connect...\n");
    c_sock = accept(w_addr, NULL, NULL);
    if (c_sock == -1) {
        printf("accept error\n");
        close(w_addr);
        exit(1);
    }
    printf("Connected!!\n");
    /* 接続済のソケットでデータのやり取り */
    transfer(c_sock);
    /* ソケット通信をクローズ */
    close(c_sock);


    /* 接続待ちソケットをクローズ */
    close(w_addr);
}

int server_init(struct server_info *si){
    int ret;

    ret = pthread_create(&si->tid, NULL, sock_server, si);
    if(ret){
        perror("pthread_create");
        return 1;
    }
    return 0;
}

int server_uinit(struct server_info *si){
    int ret;
    ret = pthread_cancel(si->tid);
    if(ret){
        perror("pthread_join");
        return 1;
    }
    return 0;
}