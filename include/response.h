#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <libgen.h>
#include "parse.h"
#include "Apache_log.h"
#include "http_data.h"

#define MAX_RESPONSE_HEADER 30
//响应头结构定义
typedef struct 
{
	char header_value[4096];
}Response_headers;

extern char *ret_head;
extern char *ret_head2;
typedef struct{
    char http_version[50];
    char* http_status_msg;
    int http_status_code_name;
    int http_status_code;
    int response_bytes;
    char http_msg[99999];
    char type;
    char path[BUF_SIZE];
}Response;

extern Response* make_response(Request* request);

extern void free_Response(Response * r);

extern int implementPOST(Response* r,Request* request);
extern int implementHEAD(Response* r,Request* request);
extern int implementGET(Response* r,Request* request);
extern int implementUKOWN(Response* r,Request* request);
