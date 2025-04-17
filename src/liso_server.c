#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "parse.h"
#include "Apache_log.h"
#include "http_data.h"
#include "response.h"

#define ECHO_PORT 9999
#define SEND_BUFFER_SIZE 4096 
#define MAX_HEADER_SIZE 8192
#define LimitRequestLine 8192


int sock=-1,client_sock=-1;
char buf[BUF_SIZE];

int close_socket(int sock) {
    if(close(sock)) {
        fprintf(stderr,"Failed closing socket.\n");
        return 1;
    }
    return 0;
}
void handle_signal(const int sig) {
    if(sock!=-1) {
        fprintf(stderr,"\nReceived signal %d. Closing socket.\n",sig);
        close_socket(sock);
    }
    exit(0);
}
void handle_sigpipe(const int sig)
{
    if(sock!=-1) {
        return;
    }
    exit(0);
}

int read_request_header(int sock,char* header_buf);

int send_file(int client_sock,const char* filepath,Response* response);

void echo_service(Request* requst,int sock,char* buf);
int main(int argc,char* argv[]) {
    /* register signal handler */
    /* process termination signals */

    parse_format(access_log);
    parse_format(error_log);

    signal(SIGTERM,handle_signal);
    signal(SIGINT,handle_signal);
    signal(SIGSEGV,handle_signal);
    signal(SIGABRT,handle_signal);
    signal(SIGQUIT,handle_signal);
    signal(SIGTSTP,handle_signal);
    signal(SIGFPE,handle_signal);
    signal(SIGHUP,handle_signal);
    /* normal I/O event */
    signal(SIGPIPE,handle_sigpipe);
    socklen_t cli_size;
    struct sockaddr_in addr,cli_addr;
    fprintf(stdout,"----- Echo Server -----\n");

    /* all networked programs must create a socket */
    if((sock=socket(PF_INET,SOCK_STREAM,0))==-1) {
        fprintf(stderr,"Failed creating socket.\n");
        return EXIT_FAILURE;
    }
    /* set socket SO_REUSEADDR | SO_REUSEPORT */
    int opt=1;
    if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR|SO_REUSEPORT,&opt,sizeof(opt))) {
        fprintf(stderr,"Failed setting socket options.\n");
        return EXIT_FAILURE;
    }

    addr.sin_family=AF_INET; // ipv4
    addr.sin_port=htons(ECHO_PORT);
    addr.sin_addr.s_addr=INADDR_ANY;

    /* servers bind sockets to ports---notify the OS they accept connections */
    if(bind(sock,(struct sockaddr*)&addr,sizeof(addr))) {
        close_socket(sock);
        fprintf(stderr,"Failed binding socket.\n");
        return EXIT_FAILURE;
    }

    if(listen(sock,5)) {
        close_socket(sock);
        fprintf(stderr,"Error listening on socket.\n");
        return EXIT_FAILURE;
    }

    /* finally, loop waiting for input and then write it back */

    while(1) {
        /* listen for new connection */
        cli_size=sizeof(cli_addr);
        fprintf(stdout,"Waiting for connection...\n");
        client_sock=accept(sock,(struct sockaddr*)&cli_addr,&cli_size);
        if(client_sock==-1)
        {
            fprintf(stderr,"Error accepting connection.\n");
            close_socket(sock);
            return EXIT_FAILURE;
        }
        fprintf(stdout,"New connection from %s:%d\n",inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port));

        while(1){
            /* receive msg from client, and concatenate msg with "(echo back)" to send back */
            memset(buf,0,BUF_SIZE);
            /*--------------------old receive----------------*/
            // int readret=recv(client_sock,buf,BUF_SIZE-1,0);
            // buf[BUF_SIZE]='\0';
            /*--------------------old receive----------------*/

            /*--------------------new receive----------------*/
            int readret=read_request_header(client_sock,buf);
            buf[readret]='\0';
            /*--------------------new receive----------------*/

            if(readret<=0)break;

            fprintf(stdout,"Received (total %d bytes):%s \n",readret,buf);

            Request* request=parse(buf,strlen(buf),client_sock);

            Response* response=make_response(request,inet_ntoa(cli_addr.sin_addr));

            if(response==NULL) {
                fprintf(stderr,"Failed to create response\n");
                continue;
            }

            // if(response->http_status_code!=200) {
            //     strcat(response->http_msg,"\r\n");
            // }

            if(send(client_sock,response->http_msg,strlen(response->http_msg),0)<0) {
                fprintf(stderr,"Failed to send HTTP headers\n");
            }

            if(response->method==GET&&response->http_status_code==200) {
                if(send_file(client_sock,response->path,response)<0) {
                    fprintf(stderr,"Failed to send file: %s\n",response->path);
                }
            }
            if(response->http_status_code==200)
                write_log(request,response,access_log);
            else
                write_log(request,response,error_log);

            free_Response(response);

            fprintf(stdout,"\n---------send back-------\n");

        }
        /* client closes the connection. server free resources and listen again */
        if(close_socket(client_sock))
        {
            close_socket(sock);
            fprintf(stderr,"Error closing client socket.\n");
            return EXIT_FAILURE;
        }
        fprintf(stdout,"Closed connection from %s:%d\n",inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port));
    }

    /* close the socket */
    close_socket(sock);
    return EXIT_SUCCESS;
}


void echo_service(Request* rq,int sock,char* buf){
    if(merge_request(rq,buf))
        send(client_sock,buf,strlen(buf),0);
    else{
        send(client_sock,error_msg[code501],strlen(error_msg[code501]),0);
    }
}

int send_file(int client_sock,const char* filepath,Response* response) {
    fprintf(stdout,"Opening file: %s\n",filepath);

    FILE* fp=fopen(filepath,"rb");
    if(fp==NULL) {
        fprintf(stderr,"Failed to open file: %s, errno: %d\n",filepath,errno);
        return -1;
    }

    fseek(fp,0,SEEK_END);
    long file_size=ftell(fp);
    rewind(fp);

    fprintf(stdout,"File size: %ld bytes\n",file_size);

    char buffer[SEND_BUFFER_SIZE];
    size_t total_sent=0;
    size_t bytes_read;

    while((bytes_read=fread(buffer,1,sizeof(buffer),fp))>0) {
        size_t bytes_sent=0;
        while(bytes_sent<bytes_read) {
            ssize_t ret=send(client_sock,buffer+bytes_sent,
                bytes_read-bytes_sent,0);
            if(ret<=0) {
                fprintf(stderr,"Failed to send file data, errno: %d\n",errno);
                fclose(fp);
                return -1;
            }
            bytes_sent+=ret;
            total_sent+=ret;
        }
    }

    fprintf(stdout,"Total bytes sent: %zu\n",total_sent);
    fclose(fp);
    return 0;
}

int read_request_header(int sock,char* header_buf) {
    int total_read=0;
    int bytes_read;
    while(1) {
        bytes_read=recv(sock,header_buf+total_read,1024,0); // 每次读取 1024 字节
        if(bytes_read<0) {
            perror("recv error");
            return -1;
        }
        if(bytes_read==0) {
            // 客户端关闭连接
            break;
        }
        total_read+=bytes_read;
        header_buf[total_read]='\0';  // 保证字符串结尾

        // 判断是否超过 header 限制（注意加上结束符号所占空间）
        if(total_read>MAX_HEADER_SIZE) {
            // 超过头部长度限制，返回错误
            fprintf(stderr,"Request header too large: %d bytes\n",total_read);
            return -1;
        }

        // 检查是否完整收到 header（以 "\r\n\r\n" 结束）
        if(strstr(header_buf,"\r\n\r\n")!=NULL) {
            break;
        }
    }
    return total_read;
}
