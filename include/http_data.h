#ifndef MYDATA_H    // 预防重复包含
#define MYDATA_H


#define BUF_SIZE 9999

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
    char* client_ip;
    int log_level;
    char* socket_buf;
} Request;


enum { code400, code404, code501, code200, code505 };
enum { GET=2, POST, HEAD, UKNOWN, ERROR };

// 如果有全局数据需要共享，使用 extern 声明

extern int http_code[];
extern char* error_msg[];
extern char *defaut_address;
#endif /* MYDATA_H */