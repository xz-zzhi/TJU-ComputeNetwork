#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "parse.h"
#include "Apache_log.h"
#include "http_data.h"
#include "response.h"

#define ECHO_PORT 9999

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
void echo_service(Request* requst,int sock,char* buf);
int main(int argc,char* argv[]) {
    /* register signal handler */
    /* process termination signals */
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

            int readret=recv(client_sock,buf,BUF_SIZE,0);
            if(readret<=0)break;
            fprintf(stdout,"Received (total %d bytes):%s \n",readret,buf);

            Request* request=parse(buf,strlen(buf),client_sock);

            Response* response=make_response(request);
            if(response->http_status_code!=200){
                strcat(response->http_msg,"\r\n");
            }
            if(send(client_sock,response->http_msg,strlen(response->http_msg),0)<0){
                fprintf(stderr,"can't send???\n");

            }

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
