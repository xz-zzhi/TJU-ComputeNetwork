#pragma once
#include <string.h>
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
#include <unistd.h>
#include <stdbool.h>

#define SUCCESS 0

//Header field
typedef struct {
    char header_name[4096];
    char header_value[4096];
} Request_header;

//HTTP Request Header
typedef struct {
    char http_version[50];
    char http_method[50];
    char http_uri[4096];
    Request_header* headers;
    int header_count;
} Request;

Request* parse(const char* buffer,int size,int socketFd);

bool merge_request(Request* rq,char* orin);

// functions decalred in parser.y
extern int yyparse();


extern void set_parsing_options(char* buf,size_t i,Request* request);
