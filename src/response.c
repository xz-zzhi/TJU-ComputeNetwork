#include "parse.h"
#include "Apache_log.h"
#include "http_data.h"
#include "response.h"

char* ret_head="Server: Liso/1.0\r\nDate: Mon, 23 May 2025 0:47:44 CST\r\n";
char* ret_head2="Last-modified: Mon, 23 May 2025 10:20:31 GMT\r\nConnection: keep-alive\r\n";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/select.h>
#include <fcntl.h>
#include <time.h>

#define MAX_ENV_VARS_INITIAL 16

enum{
    TEXT=0,
    IMAGE,
};
int ident_type(char* suffix){
    if(!strcmp(suffix,"png")||!strcmp(suffix,"jpeg")||!strcmp(suffix,"jpg")||!strcmp(suffix,"gif")||!strcmp(suffix,"bmp"))
        return IMAGE;
    return TEXT;
}

int determineRequestType(char str[])
{
    if(strcasecmp(str,"GET")==0) return GET;
    if(strcasecmp(str,"POST")==0) return POST;
    if(strcasecmp(str,"HEAD")==0) return HEAD;
    return UKNOWN;
}

int implementUKOWN(Response* r,Request* request){
    r->http_status_code_name=code501;
    r->http_status_code=http_code[r->http_status_code_name];
    r->http_status_msg=error_msg[r->http_status_code_name];
    strcpy(r->http_msg,r->http_status_msg);
    return 1;
}
//POST方法实现
int implementPOST(Response* r,Request* request)
{
    strcpy(r->http_msg,request->socket_buf);
    fprintf(stderr,"?\n");
    return 1;
}

//HEAD方法实现
int implementHEAD(Response* r,Request* request)
{
    char head[4096];
    memset(head,0,sizeof(head));

    strcpy(r->path,defaut_address);
    if(strcmp(request->http_uri,"/")==0){
        strcat(r->path,"/index.html");
        r->http_status_code=200;
        r->http_status_code_name=code200;
        r->http_status_msg=error_msg[code200];
        strcpy(r->http_msg,r->http_status_msg);
        strcat(r->http_msg,ret_head);
    }
    else{
        strcat(r->path,request->http_uri);
        struct stat buf;
        if(stat(r->path,&buf)!=0) {
            r->http_status_code=404;
            r->http_status_code_name=code404;
            r->http_status_msg=error_msg[code404];
            strcpy(r->http_msg,r->http_status_msg);
            return 1;
        }
        if(S_ISDIR(buf.st_mode)) {
            strcat(r->path,"/index.html");
            if(stat(r->path,&buf)!=0) {
                r->http_status_code=404;
                r->http_status_code_name=code404;
                r->http_status_msg=error_msg[code404];
                strcpy(r->http_msg,r->http_status_msg);
                return 1;
            }
        }
    }
    struct stat buf;
    if(stat(r->path,&buf)!=0) {
        r->http_status_code=404;
        r->http_status_code_name=code404;
        r->http_status_msg=error_msg[code404];
        strcpy(r->http_msg,r->http_status_msg);
        return 1;
    }
    r->type=(S_ISCHR(buf.st_mode))?'r':'b';
    r->content_bytes=buf.st_size;
    r->http_status_code=200;
    r->http_status_code_name=code200;
    r->http_status_msg=error_msg[code200];
    strcpy(r->http_msg,r->http_status_msg);
    strcat(r->http_msg,ret_head);


    sprintf(head,"Content-Length: %ld\r\n",buf.st_size);
    strcat(r->http_msg,head);
    memset(head,0,sizeof(head));

    //获取文件后缀名
    char* dot=strrchr(r->path,'.');
    char suffix[40]={0};
    if(dot) {
        strncpy(suffix,dot+1,sizeof(suffix)-1);
    }
    char type[20]="text";
    if(r->type!='r'){
        if(ident_type(suffix)==IMAGE)
            strcpy(type,"image");
    }
    sprintf(head,"Content-Type: %s/%s\r\n",type,(suffix[0]!='\0')?suffix:"html");
    strcat(r->http_msg,head);
    memset(head,0,sizeof(head));
    strcat(r->http_msg,ret_head2);
    strcat(r->http_msg,"\r\n");


    return 1;
}

int implementGET(Response* r,Request* request)
{
    implementHEAD(r,request);

    // if(r->http_status_code==200){
    //     char* fbuff=(char*)malloc(sizeof(char)*BUF_SIZE);
    //     if(fbuff==NULL) {
    //         return 1;  // 内存分配失败处理
    //     }

    //     FILE* fp=fopen(r->path,"r");
    //     if(fp==NULL) {
    //         free(fbuff);  // 释放已分配的内存
    //         return 1;
    //     }

    //     fseek(fp,0,SEEK_END);
    //     long lSize=ftell(fp);
    //     rewind(fp);
    //     size_t read_size=fread(fbuff,sizeof(char),lSize,fp);
    //     fbuff[read_size]='\0';
    //     fclose(fp);
    //     strcat(r->http_msg,fbuff);
    //     free(fbuff);  // 释放内存
    //     return 1;
    // }
    return 1;
}

Response* make_response(Request* request,char addr[]){
    Response* r=(Response*)malloc(sizeof(Response));
    // r->http_msg=NULL;
    strcpy(r->client_ip,addr);
    fprintf(stderr,"!!!\n");
    strcpy(request->client_ip,addr);
    fprintf(stderr,"!!!\n");

    r->content_bytes=0;
    r->log_level=100;
    if(r==NULL) {
        return NULL;
    }
    memset(r->http_msg,0,sizeof(r->http_msg));
    memset(r->http_version,0,sizeof(r->http_version));
    memset(r->path,0,sizeof(r->path));
    fprintf(stderr,"?\n");

    if(request==NULL){
        r->http_status_code_name=code400;
        r->http_status_code=http_code[r->http_status_code_name];
        r->http_status_msg=error_msg[r->http_status_code_name];
        strcpy(r->http_msg,r->http_status_msg);
        fprintf(stderr,"?\n");
        return r;
    }
    // if(request->http_method)
    if(strcmp(request->http_version,"HTTP/1.1")!=0){
        r->http_status_code_name=code505;
        r->http_status_code=http_code[r->http_status_code_name];
        r->http_status_msg=error_msg[r->http_status_code_name];
        strcpy(r->http_msg,r->http_status_msg);

        return r;
    }

    int method=determineRequestType(request->http_method);
    r->method=method;
    switch(method){
    case (GET):
        implementGET(r,request);
        break;
    case (POST):
        implementPOST(r,request);
        break;
    case (UKNOWN):
        implementUKOWN(r,request);
        break;
    case (HEAD):
        implementHEAD(r,request);
        break;
    default:
        implementUKOWN(r,request);
        break;
    }
    fprintf(stderr,"???\n");

    r->msg_bytes=strlen(r->http_msg)+(r->content_bytes);
    fprintf(stderr,"???\n");

    return r;
}


void free_Response(Response* r){
    if(r==NULL){
        return;
    }
    fprintf(stderr,"?\n");

    free(r);
}


// Dynamic array for environment variables
static char** envp=NULL;
static size_t envc=0,env_cap=0;

// Ensure capacity for new entries
static void ensure_env_capacity() {
    if(envc+1>=env_cap) {
        env_cap=env_cap?env_cap*2:MAX_ENV_VARS_INITIAL;
        envp=realloc(envp,env_cap*sizeof(char*));
        if(!envp) {
            perror("realloc envp");
            exit(EXIT_FAILURE);
        }
    }
}

// Add an environment variable formatted as KEY=VALUE
static void add_env(const char* key,const char* value) {
    if(!key||!value) {
        fprintf(stderr,"%s\n",key);
        fprintf(stderr,"Invalid environment variable: key or value is NULL\n");
        return;
    }
    ensure_env_capacity();
    size_t len=strlen(key)+strlen(value)+2;
    char* buf=malloc(len);
    if(!buf) {
        perror("malloc buf");
        exit(EXIT_FAILURE);
    }
    snprintf(buf,len,"%s=%s",key,value);
    envp[envc++]=buf;
}

// Convert HTTP header name to CGI variable name, e.g. "User-Agent" -> "HTTP_USER_AGENT"
static char* make_http_env_name(const char* header) {
    size_t len=strlen(header)+6;
    char* name=malloc(len);
    char* p=name;
    strcpy(p,"HTTP_"); p+=5;
    const char* c;
    for(c=header; *c; ++c) {
        *p++=(*c=='-')?'_':toupper((unsigned char)*c);
    }
    *p='\0';
    return name;
}

char** build_cgi_envp(const Request* req) {
    envc=0;
    env_cap=0;
    envp=NULL;

    // 1. Standard CGI variables
    add_env("CONTENT_LENGTH","");
    add_env("CONTENT_TYPE","");

    fprintf(stderr,"build_cgi_envp 1\n");
    // Fill CONTENT_* from headers
    int i;
    for(i=0; i<req->header_count; ++i) {
        if(strcasecmp(req->headers[i].header_name,"Content-Length")==0) {
            add_env("CONTENT_LENGTH",req->headers[i].header_value);
        }
        if(strcasecmp(req->headers[i].header_name,"Content-Type")==0) {
            add_env("CONTENT_TYPE",req->headers[i].header_value);
        }
    }


    add_env("GATEWAY_INTERFACE","CGI/1.1");
    add_env("REQUEST_METHOD",req->http_method);
    add_env("REQUEST_URI",req->http_uri);
    add_env("REMOTE_ADDR",req->client_ip);

    // 3) Split URI for SCRIPT_NAME, PATH_INFO, QUERY_STRING
    char script[4096]={0};
    char path_info[4096]={0};
    char query[4096]={0};

    fprintf(stderr,"build_cgi_envp 2\n");

    // 3.1 保护性读取
    const char* uri=req->http_uri?req->http_uri:"";
    // 找到 '?' 分割点
    const char* q=strchr(uri,'?');

    // 3.2 计算脚本名长度并裁剪
    size_t max_script=sizeof(script)-1;
    size_t uri_len=q?(size_t)(q-uri):strlen(uri);
    if(uri_len>max_script) uri_len=max_script;

    // 3.3 安全复制
    memcpy(script,uri,uri_len);
    script[uri_len]='\0';

    // 3.4 如果有 query，复制并裁剪
    if(q) {
        size_t qlen=strlen(q+1);
        size_t max_query=sizeof(query)-1;
        if(qlen>max_query) qlen=max_query;
        memcpy(query,q+1,qlen);
        query[qlen]='\0';
    }

    fprintf(stderr,"build_cgi_envp 3: script=\"%s\", query=\"%s\"\n",script,query);

    // 4) Extract PATH_INFO if 脚本名后还有 “/foo/bar”
    char* pi=strchr(script+1,'/');
    if(pi) {
        // 复制 "/foo/bar"
        strncpy(path_info,pi,sizeof(path_info)-1);
        path_info[sizeof(path_info)-1]='\0';
        *pi='\0';  // script 留下 "/cgi-bin/foo.py"
    }

    add_env("SCRIPT_NAME",script);
    add_env("PATH_INFO",path_info);
    add_env("QUERY_STRING",query);
    add_env("SERVER_PROTOCOL",req->http_version);

    // SERVER_PORT and SERVER_SOFTWARE
    char port_str[16];
    // Assume port unknown, use placeholder or fetch from socket layer
    snprintf(port_str,sizeof(port_str),"%d",80); // adjust as needed
    add_env("SERVER_PORT",port_str);
    add_env("SERVER_SOFTWARE","Liso/1.0");

    fprintf(stderr,"build_cgi_envp 4\n");

    // 2. Required HTTP_* headers
    const char* wanted[]={
        "Accept", "Referer", "Accept-Encoding", "Accept-Language",
        "Accept-Charset", "Host", "Cookie", "User-Agent", "Connection"
    };
    int j;
    for(j=0; j<(int)(sizeof(wanted)/sizeof(*wanted)); ++j) {
        for(i=0; i<req->header_count; ++i) {
            if(strcasecmp(req->headers[i].header_name,wanted[j])==0) {
                char* envname=make_http_env_name(wanted[j]);
                add_env(envname,req->headers[i].header_value);
                free(envname);
            }
        }
    }
    fprintf(stderr,"build_cgi_envp 5\n");

    // Null-terminate
    ensure_env_capacity();
    envp[envc]=NULL;
    return envp;
}

int invoke_cgi(const char* script_path,const Request* req,int client_sock){
    // snprintf(script_path, sizeof(script_path), ".%s", script_path); // 确保脚本路径正确
    char fs_path[512];
    snprintf(fs_path,sizeof(fs_path),".%s",script_path);

    fprintf(stderr,"execve: %s\n",fs_path);

    int pipe_in[2],pipe_out[2];
    pid_t pid;
    char** envp=build_cgi_envp(req);
    fprintf(stderr,"invoke 1\n");
    if(!envp) return -1;

    // 1) 创建管道
    if(pipe(pipe_in)<0||pipe(pipe_out)<0) {
        perror("pipe");
        goto err_free_env;
    }

    // 2) fork 子进程
    pid=fork();
    if(pid<0) {
        perror("fork");
        goto err_close_pipes;
    }

    if(pid==0) {
        // —— 子进程 ——
        // 关闭不使用的端
        close(pipe_in[1]);
        close(pipe_out[0]);

        // 重定向 stdin/stdout
        if(dup2(pipe_in[0],STDIN_FILENO)<0||
            dup2(pipe_out[1],STDOUT_FILENO)<0) {
            perror("dup2");
            _exit(1);
        }
        // 可选：dup2(pipe_out[1], STDERR_FILENO);

        // 执行 CGI 脚本
        // char* const argv[]={(char*)fs_path, NULL};
        char* const argv[] = { "/usr/bin/python3", "-u", (char*)fs_path, NULL };
        // execve(fs_path,argv,envp);
        execve("/usr/bin/python3", argv, envp);

        // execve 失败
        perror("execve");
        _exit(1);
    }

    fprintf(stderr,"invoke 2 fork\n");

    // —— 父进程 ——
    // 关闭子进程不使用端
    close(pipe_in[0]);
    close(pipe_out[1]);

    // 3) 如果是 POST，就向子进程 stdin 写入请求体
    if(strcasecmp(req->http_method,"POST")==0&&req->socket_buf) {
        size_t len=strlen(req->socket_buf);
        if(write(pipe_in[1],req->socket_buf,len)!=(ssize_t)len) {
            perror("write to cgi stdin");
            // 但继续读 stdout，把错误信息也回传
        }
    }
    // 写完后关闭 stdin 写端
    close(pipe_in[1]);

    // 1) 先发 HTTP 头，让浏览器知道连接是活的
    const char *hdr =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "\r\n";
    send(client_sock, hdr, strlen(hdr), 0);

    // 2) 使用 select() 对 CGI stdout 加超时
    fd_set rfds;
    struct timeval tv;
    int maxfd = pipe_out[0];
    time_t start = time(NULL);
    int timed_out = 0;

    while (1) {
        FD_ZERO(&rfds);
        FD_SET(pipe_out[0], &rfds);

        // 每次等待 1 秒，累计到 5 秒就退出
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int ret = select(maxfd+1, &rfds, NULL, NULL, &tv);
        if (ret < 0) {
            perror("select");
            break;
        } else if (ret == 0) {
            // 超时 1 秒，检查总时长
            if (time(NULL) - start >= 5) {
                fprintf(stderr, "invoke_cgi: timeout reached\n");
                timed_out=1;
                break;
            }
            // 否则继续等待
            continue;
        }

        if (FD_ISSET(pipe_out[0], &rfds)) {
            char buf[4096];
            ssize_t n = read(pipe_out[0], buf, sizeof(buf));
            if (n < 0) {
                perror("read from cgi");
                break;
            } else if (n == 0) {
                // EOF，脚本结束
                break;
            }
            fprintf(stderr,"******\n");
            // 将脚本输出直接发给浏览器
            fwrite(buf, 1, n, stderr);
            if (send(client_sock, buf, n, 0) < 0) {
                perror("send to client");
                break;
            }
        }
    }
    if (timed_out) {
        kill(pid, SIGKILL);    // 或 SIGTERM
    }

    close(pipe_out[0]);

    // 等待子进程退出，避免僵尸
    waitpid(pid, NULL, 0);

    if (timed_out) {
        const char *err504 =
            "HTTP/1.1 504 Gateway Timeout\r\n"
            "Content-Length: 0\r\n\r\n";
        send(client_sock, err504, strlen(err504), 0);
    }
    // 清理 envp
    char **p;
    for (p = envp; *p; ++p) free(*p);
    free(envp);

    return 0;

err_close_pipes:
    close(pipe_in[0]); close(pipe_in[1]);
    close(pipe_out[0]); close(pipe_out[1]);
err_free_env:
    for (p = envp; p && *p; ++p) free(*p);
    free(envp);
    return -1;
}