#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include "parse.h"

#define ECHO_PORT 9999
#define BUF_SIZE 4096

int sock = -1, client_sock = -1;
//sock服务器的监听套接字（监听客户端连接的套接字）。
char buf[BUF_SIZE];

//接收一个套接字描述符 sock 作为参数，尝试关闭该套接字
int close_socket(int sock) {
    if (close(sock)) {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}
//信号处理函数，用于关闭套接字
void handle_signal(const int sig) {
    if (sock != -1) {
        fprintf(stderr, "\nReceived signal %d. Closing socket.\n", sig);
        close_socket(sock);
    }
    exit(0);
}
//sigpipe信号处理
void handle_sigpipe(const int sig) 
{
    if (sock != -1) {
        return;
    }
    exit(0);
}
//http请求处理
/*
    request指向客户端发送的http请求的字符串
    method GET post ……
    path 请求路径
    http_version
*/
int parse_http_request(const char *buf,char *method, char *path, char *http_version){

    printf("buf:%s\n",buf);
    printf("strlen(buf):%d\n",strlen(buf));

    Request *request = parse(buf, strlen(buf), -1);

    fprintf(stdout,"Received (total %d bytes):%s \n",strlen(buf),buf);

    if (request == NULL) {
        return 0;//不合规范
    }

    method = request->http_method;
    path = request->http_uri;
    http_version = request->http_version;

    //strcmp相等返回0
    if(strcmp(method,"GET")!=0&&strcmp(method,"POST")!=0&&strcmp(method,"HEAD")!=0){
        return -1;//未实现方法
    }
    return 1;
}
int main(int argc, char *argv[]) {
    /* register signal handler */
    /* process termination signals */
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);
    signal(SIGSEGV, handle_signal);
    signal(SIGABRT, handle_signal);
    signal(SIGQUIT, handle_signal);
    signal(SIGTSTP, handle_signal);
    signal(SIGFPE, handle_signal);
    signal(SIGHUP, handle_signal);
    /* normal I/O event */
    signal(SIGPIPE, handle_sigpipe);
    socklen_t cli_size;//存储客户端地址结构的大小
    struct sockaddr_in addr, cli_addr;//ipv4地址网络
    fprintf(stdout, "----- Echo Server -----\n");
    
    /* all networked programs must create a socket */
    //创建ipv4协议 TCP协议的连接
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }
    /* set socket SO_REUSEADDR | SO_REUSEPORT */
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR |SO_REUSEPORT, &opt, sizeof(opt))) {
        fprintf(stderr, "Failed setting socket options.\n");
        return EXIT_FAILURE;
    }

    addr.sin_family = AF_INET; // ipv4
    addr.sin_port = htons(ECHO_PORT);//监听端口号9999
    addr.sin_addr.s_addr = INADDR_ANY;//将服务器的 IP 地址设置为 0.0.0.0，从而让服务器监听所有可用的网络接口

    /* servers bind sockets to ports---notify the OS they accept connections */
    //socket绑定ip port 协议族
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr))) {
        close_socket(sock);
        fprintf(stderr, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }
    //最大的连接数量为5
    if (listen(sock, 5)) {
        close_socket(sock);
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }

    /* finally, loop waiting for input and then write it back */
    //初始化结束
    /*
        初始化流程:
        1 create socket
        2 bind
        3 listen
    */
    while (1) {
        /* listen for new connection */
        cli_size = sizeof(cli_addr);
        fprintf(stdout,"Waiting for connection...\n");
        //阻塞直到收到连接
        client_sock = accept(sock, (struct sockaddr *) &cli_addr, &cli_size);
        if (client_sock == -1)
        {
            fprintf(stderr, "Error accepting connection.\n");
            close_socket(sock);
            return EXIT_FAILURE;
        }
        //打印客户端的ip port
        fprintf(stdout,"New connection from %s:%d\n",inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port));
        while(1){
             /* receive msg from client, and concatenate msg with "(echo back)" to send back */
            memset(buf, 0, BUF_SIZE);
            //读取客户端传入的数据，写入buf中
            int readret = recv(client_sock, buf, BUF_SIZE, 0);
            if (readret <=0)break;
            fprintf(stdout,"Received (total %d bytes):%s \n",readret,buf); 
            
            //开始处理请求
            char method[10],path[100],http_version[20];
            int parse_result = parse_http_request(buf,method,path,http_version);
            if(parse_result==1){
                // if(send(client_sock,buf,strlen(buf),0)<0){
                //     break;
                // }
                send(client_sock, buf, strlen(buf), 0);
                fprintf(stdout,"Send back\n");
            }else if(parse_result==-1){
                char *response = "HTTP/1.1 501 Not Implemented\r\n\r\n";
                send(client_sock,response,strlen(response),0);
            }else if(parse_result == 0){
                char *response = "HTTP/1.1 400 Bad request\r\n\r\n";
                send(client_sock,response,strlen(response),0);
            }
            /* when client is closing the connection：
                FIN of client carrys empty，so recv() return 0
                ACK of server only carrys"(echo back)", so send() return 11
                ACK of client carrys empty, so recv() return 0
                Then server finishes closing the connection, recv() and send() return -1 */
        }
        /* client closes the connection. server free resources and listen again */
        //关闭client套接子，成功返回0 失败返回1
        if (close_socket(client_sock))
        {
            close_socket(sock);
            fprintf(stderr, "Error closing client socket.\n");
            return EXIT_FAILURE;
        }
        //防止内存泄漏
        fprintf(stdout,"Closed connection from %s:%d\n",inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port));
    }
    //正常关闭socket
    close_socket(sock);
    return EXIT_SUCCESS;
}
