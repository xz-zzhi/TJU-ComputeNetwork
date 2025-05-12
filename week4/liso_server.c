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

#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include "parse.h"
#include "Apache_log.h"
#include "http_data.h"
#include "response.h"

#define ECHO_PORT 9999
#define SEND_BUFFER_SIZE 4096
#define MAX_HEADER_SIZE 8192
#define LimitRequestLine 8192


int sock=-1,old_client_sock=-1;
char buf[FD_SETSIZE+5][BUF_SIZE];
int count_Time[FD_SETSIZE+5];
int vis_sock[FD_SETSIZE+5];
int sock_now_read[FD_SETSIZE+5];
int count_sock=0;
int all_sock[FD_SETSIZE+5];
int TIME=3;

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

int read_request_header(int sock,char* header_buf,struct sockaddr_in cli_addr,int client_sock,int toal_read);

int send_file(int client_sock,const char* filepath,Response* response);

void echo_service(Request* requst,int sock,char* buf);

char* find_tail(char* buf,char** start,char* end);

void solve(Request* request,Response* response,int client_sock);

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
    struct sockaddr_in all_cli_addr[2000];

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
    fd_set read_fds;
    fd_set write_fds;// ####### 要吧client sock的内容封装为局部变量
    struct timeval timeout;

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_SET(sock,&read_fds);
    all_cli_addr[sock]=addr;
    // all_sock[++count_sock]=sock;
    count_sock=1;
    int ccnt=1;
    int ii;
    for(ii=0;ii<=FD_SETSIZE+1;++ii){
        vis_sock[ii]=-1;
    }
    vis_sock[sock]=sock;

    while(true){
        timeout.tv_sec=0;
        timeout.tv_usec=500;
        FD_ZERO(&read_fds);
        int i;
        for(i=0;i<=FD_SETSIZE;++i){
            if(vis_sock[i]==-1){
                continue;
            }
            FD_SET(vis_sock[i],&read_fds);
        }
        int status=select(FD_SETSIZE+1,&read_fds,NULL,NULL,&timeout);
        fprintf(stderr,"==== %d ===%d \n",status,ccnt++);

        if(status==-1) {
            fprintf(stderr,"Error in select.\n");
            close_socket(sock);
            return EXIT_FAILURE;
        }
        if(status==0) {
            fprintf(stderr,"Timeout in select.\n");
            for(i=0;i<=FD_SETSIZE;++i){
                if(vis_sock[i]==-1||i==sock){
                    continue;
                }
                int now_sock=vis_sock[i];
                count_Time[now_sock]++;
                printf("count_Time[%d] = %d\n",now_sock,count_Time[now_sock]);
                if(count_Time[now_sock]>TIME){
                    if(close_socket(now_sock)){
                        close_socket(sock);
                        fprintf(stderr,"Error closing client socket.\n");
                        return EXIT_FAILURE;
                    }
                    fprintf(stdout,"Closed connection from %s:%d\n",inet_ntoa(all_cli_addr[now_sock].sin_addr),ntohs(all_cli_addr[now_sock].sin_port));
                    count_sock--;
                    FD_CLR(now_sock,&read_fds);
                    vis_sock[now_sock]=-1;
                    sock_now_read[now_sock]=0;
                    count_Time[now_sock]=0;
                }
            }
            continue;
        }
        // int i;
        // sleep(1);
        for(i=0;i<=FD_SETSIZE;++i){
            if(vis_sock[i]==-1){
                continue;
            }
            // fprintf(stderr,"neeeeeeeeeeee.\n");

            int now_sock=vis_sock[i];
            if(!FD_ISSET(now_sock,&read_fds)){
                count_Time[now_sock]++;
                printf("count_Time[%d] = %d\n",now_sock,count_Time[now_sock]);
                if(count_Time[now_sock]>TIME){
                    if(close_socket(now_sock)){
                        close_socket(sock);
                        fprintf(stderr,"Error closing client socket.\n");
                        return EXIT_FAILURE;
                    }
                    fprintf(stdout,"Closed connection from %s:%d\n",inet_ntoa(all_cli_addr[now_sock].sin_addr),ntohs(all_cli_addr[now_sock].sin_port));
                    /*-----------------close----------------*/
                    count_sock--;
                    FD_CLR(now_sock,&read_fds);
                    vis_sock[now_sock]=-1;
                    sock_now_read[now_sock]=0;
                    count_Time[now_sock]=0;
                }
                continue;
            }
            // fprintf(stderr,"n1.\n");

            if(now_sock==sock){
                // fprintf(stderr,"neeeeeeeeeeee.\n");
                // fprintf(stderr,"n2.\n");
                cli_size=sizeof(cli_addr);
                if(count_sock>1000){
                    fprintf(stderr,"Too many connections.\n");
                    continue;
                }
                int new_client=accept(sock,(struct sockaddr*)&cli_addr,&cli_size);
                if(new_client==-1){
                    fprintf(stderr,"Error accepting connection.\n");
                    close_socket(sock);
                    return EXIT_FAILURE;
                }
                fprintf(stdout,"New connection from %s:%d\n",inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port));
                all_cli_addr[new_client]=cli_addr;
                FD_SET(new_client,&read_fds);
                vis_sock[new_client]=new_client;
                memset(&buf[now_sock][0],0,BUF_SIZE);
                count_sock++;
                // fprintf(stderr,"n2end.\n");

                // FD_SET(new_client,&write_fds);//write 判段 是否断开连接
            }
            else{

                fprintf(stdout,"now is %s:%d\n",inet_ntoa(all_cli_addr[now_sock].sin_addr),ntohs(all_cli_addr[now_sock].sin_port));

                int status=read_request_header(now_sock,&buf[now_sock][0],all_cli_addr[now_sock],now_sock,sock_now_read[now_sock]);
                fprintf(stdout,"\n\n---------send back all-------\n");
                /*-----------------close----------------*/
                /* client closes the connection. server free resources and listen again */
                if(status==-1){
                    if(close_socket(now_sock)){
                        close_socket(sock);
                        fprintf(stderr,"Error closing client socket.\n");
                        return EXIT_FAILURE;
                    }
                    fprintf(stdout,"Closed connection from %s:%d\n",inet_ntoa(all_cli_addr[now_sock].sin_addr),ntohs(all_cli_addr[now_sock].sin_port));
                    /*-----------------close----------------*/
                    count_sock--;
                    FD_CLR(now_sock,&read_fds);
                    vis_sock[now_sock]=-1;
                    sock_now_read[now_sock]=0;
                    count_Time[now_sock]=0;
                }

                // fprintf(stderr,"status is %d\n",status);
                sock_now_read[now_sock]=status;
            }
        }
        FD_ZERO(&read_fds);
    }
    // while(1) {
    //     /* listen for new connection */
    //     cli_size=sizeof(cli_addr);
    //     fprintf(stdout,"Waiting for connection...\n");

    //     /*-----------get------------request*/
    //     // client_sock=accept(sock,(struct sockaddr*)&cli_addr,&cli_size);
    //     // if(client_sock==-1)
    //     // {
    //     //     fprintf(stderr,"Error accepting connection.\n");
    //     //     close_socket(sock);
    //     //     return EXIT_FAILURE;
    //     // }
    //     // fprintf(stdout,"New connection from %s:%d\n",inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port));
    //     /*-----------get------------request*/



    //     /*-----------solve()-----------*/
    //     // memset(buf,0,BUF_SIZE);
    //     // read_request_header(client_sock,buf,cli_addr);
    //     /*-----------solve()-----------*/

    //     fprintf(stdout,"\n\n---------send back all-------\n");


    //     /*-----------------close----------------*/
    //     /* client closes the connection. server free resources and listen again */
    //     // if(close_socket(client_sock))
    //     // {
    //     //     close_socket(sock);
    //     //     fprintf(stderr,"Error closing client socket.\n");
    //     //     return EXIT_FAILURE;
    //     // }
    //     // fprintf(stdout,"Closed connection from %s:%d\n",inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port));
    //     /*-----------------close----------------*/

    // }

    /* close the socket */
    close_socket(sock);
    return EXIT_SUCCESS;
}


// void echo_service(Request* rq,int sock,char* buf){
//     if(merge_request(rq,buf))
//         send(client_sock,buf,strlen(buf),0);
//     else{
//         send(client_sock,error_msg[code501],strlen(error_msg[code501]),0);
//     }
// }

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

char* find_tail(char* buf,char** start,char* end){
    char* old=*start;
    char* buf_tem[8888];
    memset(buf_tem,0,sizeof(buf_tem));
    while((*start)+3<=end){
        if(*(*start)=='\r'&&*((*start)+1)=='\n'&&*((*start)+2)=='\r'&&*((*start)+3)=='\n'){
            (*start)=(*start)+4;
            // fprintf(stderr,"************%d\n",(int)(end-(*start)));
            int len=(*start)-old;
            memcpy(buf_tem,old,len);
            fprintf(stdout,"\n++++++++++\nnow request: (total %d bytes):\n%s++++++++++\n",(int)(*start-old),buf_tem);
            return (*start);
        }
        (*start)++;
    }
    return NULL;
}

int read_request_header(int sock,char* header_buf,struct sockaddr_in cli_addr,int client_sock,int total_read) {
    // int total_read=0;
    int bytes_read=0;
    struct timeval recv_timeout;
    // while(1) {
    recv_timeout.tv_sec=0;
    recv_timeout.tv_usec=100;
    if(setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&recv_timeout,sizeof(recv_timeout))<0) {
        perror("setsockopt SO_RCVTIMEO failed");
    }

    int old_bufsize=bytes_read;
    bytes_read=recv(sock,header_buf+total_read,1024,0); // 每次读取 1024 字节

    if(bytes_read<0) {
        perror("recv error");
        return -1;
    }
    if(bytes_read==0) {            // 客户端关闭连接
        return -1;
    }
    fprintf(stdout,"\n-----------\nReceived (total %d bytes):%s \n-----------\n",bytes_read,buf);

    total_read+=bytes_read;

    if(total_read) {
        char* start=header_buf;
        char* old_start=header_buf;
        char* end;
        while(find_tail(header_buf,&start,header_buf+total_read)!=NULL) {
            int len=start-old_start;
            Request* request=parse(old_start,len,client_sock);
            Response* response=make_response(request,inet_ntoa(cli_addr.sin_addr));
            solve(request,response,client_sock);
            old_start=start;
            fprintf(stdout,"\n---------send back-------\n");
        }
        if(old_start!=header_buf) {
            int len=total_read-(old_start-header_buf);
            char* buf_tem[BUF_SIZE];
            memset(buf_tem,0,sizeof(buf_tem));
            memcpy(buf_tem,old_start,len);
            memset(header_buf,0,sizeof(header_buf));
            strcpy(header_buf,buf_tem);
            total_read=len;
        }
        else if(total_read>MAX_HEADER_SIZE){
            total_read=0;
            memset(header_buf,0,sizeof(header_buf));
        }
    }
    else{
        return;//实际上不会进入这一行
    }

    // // 检查是否完整收到 header（以 "\r\n\r\n" 结束）
    // if(strstr(header_buf,"\r\n\r\n")!=NULL) {
    //     break;
    // }
// }
    return total_read;
}

void solve(Request* request,Response* response,int client_sock) {
    if(response==NULL) {
        fprintf(stderr,"Failed to create response\n");
        return;
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
}