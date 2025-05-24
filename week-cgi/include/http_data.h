#ifndef MYDATA_H    // 预防重复包含
#define MYDATA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUF_SIZE 9999
#define MAX_RESPONSE_HEADER 30
#define HTTP_MSG_SIZE 8192
#define PATH_SIZE 1024
#define HTTP_VERSION_SIZE 16
#define MAX_ENV_VARS 128

// typedef 定义类型
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
    int response_status;
    int response_bytes;
    char *error_message;
    char client_ip[100];
    int log_level;
    char* socket_buf;
} Request;

typedef struct 
{
	char header_value[4096];
}Response_headers;


typedef struct{
    char* http_status_msg;
    int http_status_code_name;
    int http_status_code;
    int content_bytes;
    int msg_bytes;
    char client_ip[100];
    char type;
    int method;
    int log_level;
    char http_msg[HTTP_MSG_SIZE];
    char path[PATH_SIZE];
    char http_version[HTTP_VERSION_SIZE];
}Response;

enum { code400, code404, code501, code200, code505 };

enum { GET=2, POST, HEAD, UKNOWN, ERROR };

// 如果有全局数据需要共享，使用 extern 声明

extern int http_code[];
extern char* error_msg[];
extern char *defaut_address;
#endif /* MYDATA_H */