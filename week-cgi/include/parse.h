#ifndef prase
#define prase 

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
#include "http_data.h"

#define SUCCESS 0

//Header field


extern Request* parse(const char* buffer,int size,int socketFd);

extern bool merge_request(Request* rq,char* orin);

// functions decalred in parser.y
extern int yyparse();


extern void set_parsing_options(char* buf,size_t i,Request* request);


#endif