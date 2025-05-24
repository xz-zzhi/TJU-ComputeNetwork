#ifndef APACHE_L
#define APACHE_L

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "parse.h"
#include "http_data.h"

#define MAX_TOKEN 50
#define LOG_BUFFER_SIZE 4900

/* 示例 Request 结构体定义，根据实际情况调整 */
// typedef struct {
//     char http_version[50];
//     char http_method[50];
//     char http_uri[4096];
//     Request_header* headers;
//     int header_count;
//     int response_status;
//     int response_bytes;
//     char *error_message;
//     char* client_ip;
//     int log_level;
// } Request;

typedef void (*hook_function)(Request* r);
typedef void (*solve_log_msg)(Request* r, Response* ret, char* buf, int bufsize);

/* 枚举定义 */
enum Element_type { ELEMENT_TEXT, ELEMENT_FORMAT };
enum log { error_log, access_log };
// enum { code400, code404, code501, other };
// static int http_code[]={
//     [code400] 400,
//     [code404] 404,
//     [code501] 501,
//     [other] 200,
// };
/* 用户自定义日志格式 */
extern char* user_log_format[];

/* 日志格式结构体（拆分格式化字符串为多个 token） */
typedef struct {
    int count;
    int type[MAX_TOKEN];
    char* text[MAX_TOKEN];
    solve_log_msg make_log[MAX_TOKEN];
} Log_Format;

extern Log_Format log_fromat[5];

/* 日志相关函数原型 */
extern void parse_format(int opr);
extern void write_log(Request* r,Response* ret,int opr);
extern void get_other_msg(Request *r, int response_status, int response_bytes, char *error_message, char* client_ip);
 
/* 各种格式化日志元素的函数原型 */
extern void format_time(Request* r, Response* ret, char* buf, int bufsize);
extern void format_log_level(Request* r, Response* ret, char* buf, int bufsize);
extern void format_pid(Request* r, Response* ret, char* buf, int bufsize);
extern void format_file(Request* r, Response* ret, char* buf, int bufsize);
extern void format_error(Request* r, Response* ret, char* buf, int bufsize);
extern void format_client_ip(Request* r, Response* ret, char* buf, int bufsize);
extern void format_message(Request* r, Response* ret, char* buf, int bufsize);
extern void format_request_line(Request* r, Response* ret, char* buf, int bufsize);
extern void format_status(Request* r, Response* ret, char* buf, int bufsize);
extern void format_bytes(Request* r, Response* ret, char* buf, int bufsize);

/* 钩子函数相关 */
typedef struct {
    int count;
    hook_function hook[MAX_TOKEN];
} hook_group;

extern hook_group hook_zzhi;

extern void hook_register(hook_function h);
extern void run_hook(Request* r);

#endif // PARSE_H
