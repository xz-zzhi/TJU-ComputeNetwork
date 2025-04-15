#include "Apache_log.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* 用户自定义日志格式内容 */
char* user_log_format[]={
    [access_log] ="%h - - %t \"%r\" %>s %b",
    [error_log]="[%t] [%l] [pid %P] %F: %E: [client %a] %M"
};

/* 日志格式存储变量 */
Log_Format log_fromat[5];

hook_group hook_zzhi={0};


void get_other_msg(Request* r,int response_status,int response_bytes,char* error_message,char* client_ip) {
    if(r==NULL) {
        return;
    }
    r->client_ip=client_ip;
    r->error_message=error_message;
    r->response_status=response_status;
    r->response_bytes=response_bytes;
    r->log_level=100;  // 默认日志级别
}

void parse_format(int opr) {
    Log_Format* lf=&log_fromat[opr];
    lf->count=0;
    const char* p=user_log_format[opr];

    while(*p&&lf->count<MAX_TOKEN) {
        if(*p=='%') {
            p++;  // 跳过 '%'
            if(*p=='h') {
                lf->type[lf->count]=ELEMENT_FORMAT;
                lf->make_log[lf->count]=format_client_ip;
                lf->text[lf->count]=NULL;
                lf->count++;
                p++;
            }
            else if(*p=='t') {
                lf->type[lf->count]=ELEMENT_FORMAT;
                lf->make_log[lf->count]=format_time;
                lf->text[lf->count]=NULL;
                lf->count++;
                p++;
            }
            else if(*p=='l') {
                lf->type[lf->count]=ELEMENT_FORMAT;
                lf->make_log[lf->count]=format_log_level;
                lf->text[lf->count]=NULL;
                lf->count++;
                p++;
            }
            else if(*p=='P') {
                lf->type[lf->count]=ELEMENT_FORMAT;
                lf->make_log[lf->count]=format_pid;
                lf->text[lf->count]=NULL;
                lf->count++;
                p++;
            }
            else if(*p=='F') {
                lf->type[lf->count]=ELEMENT_FORMAT;
                lf->make_log[lf->count]=format_file;
                lf->text[lf->count]=NULL;
                lf->count++;
                p++;
            }
            else if(*p=='E') {
                lf->type[lf->count]=ELEMENT_FORMAT;
                lf->make_log[lf->count]=format_error;
                lf->text[lf->count]=NULL;
                lf->count++;
                p++;
            }
            else if(*p=='M') {
                lf->type[lf->count]=ELEMENT_FORMAT;
                lf->make_log[lf->count]=format_message;
                lf->text[lf->count]=NULL;
                lf->count++;
                p++;
            }
            else if(*p=='r') {
                lf->type[lf->count]=ELEMENT_FORMAT;
                lf->make_log[lf->count]=format_request_line;
                lf->text[lf->count]=NULL;
                lf->count++;
                p++;
            }
            else if(*p=='>') { // 处理 %>s
                p++; // 跳过 '>'
                if(*p=='s') {
                    lf->type[lf->count]=ELEMENT_FORMAT;
                    lf->make_log[lf->count]=format_status;
                    lf->text[lf->count]=NULL;
                    lf->count++;
                    p++;
                }
                else {
                    p++;
                }
            }
            else if(*p=='b') {
                lf->type[lf->count]=ELEMENT_FORMAT;
                lf->make_log[lf->count]=format_bytes;
                lf->text[lf->count]=NULL;
                lf->count++;
                p++;
            }
            else {
                // 未知格式符，跳过
                p++;
            }
        }
        else {
            const char* start=p;
            while(*p&&*p!='%')
                p++;
            int len=p-start;
            lf->type[lf->count]=ELEMENT_TEXT;
            lf->text[lf->count]=(char*)malloc(len+1);
            strncpy(lf->text[lf->count],start,len);
            lf->text[lf->count][len]='\0';
            lf->make_log[lf->count]=NULL;
            lf->count++;
        }
    }
}

void write_log(Request* r,int opr) {
    if(r==NULL) {
        return;
    }
    Log_Format* lf=&log_fromat[opr];

    char log_entry[LOG_BUFFER_SIZE]={0};
    char temp[256]={0};
    int i;
    for(i=0; i<lf->count; i++) {
        if(lf->type[i]==ELEMENT_TEXT) {
            strncat(log_entry,lf->text[i],sizeof(log_entry)-strlen(log_entry)-1);
        }
        else if(lf->type[i]==ELEMENT_FORMAT&&lf->make_log[i]!=NULL) {
            memset(temp,0,sizeof(temp));
            lf->make_log[i](r,temp,sizeof(temp));
            strncat(log_entry,temp,sizeof(log_entry)-strlen(log_entry)-1);
        }
    }

    int fd;
    if(opr==error_log) {
        fd=open("A_log/error.log",O_WRONLY|O_CREAT|O_APPEND,0644);
    }
    else if(opr==access_log) {
        fd=open("A_log/access.log",O_WRONLY|O_CREAT|O_APPEND,0644);
    }
    else {
        fd=open("A_log/error.log",O_WRONLY|O_CREAT|O_APPEND,0644);
    }

    if(fd!=-1) {
        strncat(log_entry,"\n",sizeof(log_entry)-strlen(log_entry)-1);
        write(fd,log_entry,strlen(log_entry));
        close(fd);
    }
    else {
        perror("Failed to open log file");
    }
}


void format_time(Request* r,char* buf,int bufsize) {
    time_t now=time(NULL);
    struct tm* local=localtime(&now);
    snprintf(buf,bufsize,"%04d-%02d-%02d %02d:%02d:%02d",
        local->tm_year+1900,local->tm_mon+1,local->tm_mday,
        local->tm_hour,local->tm_min,local->tm_sec);
}

void format_log_level(Request* r,char* buf,int bufsize) {
    const char* level="info";
    if(r->log_level==0)
        level="error";
    else if(r->log_level==1)
        level="warn";
    snprintf(buf,bufsize,"%s",level);
}

void format_pid(Request* r,char* buf,int bufsize) {
    snprintf(buf,bufsize,"%d",getpid());
}

void format_file(Request* r,char* buf,int bufsize) {
    snprintf(buf,bufsize,"%s",__FILE__);
}

void format_error(Request* r,char* buf,int bufsize) {
    if(r->error_message)
        snprintf(buf,bufsize,"%s",r->error_message);
    else
        snprintf(buf,bufsize,"None");
}

void format_client_ip(Request* r,char* buf,int bufsize) {
    if(r->client_ip)
        snprintf(buf,bufsize,"%s",r->client_ip);
    else
        snprintf(buf,bufsize,"unknown");
}

void format_message(Request* r,char* buf,int bufsize) {
    if(r->error_message)
        snprintf(buf,bufsize,"%s",r->error_message);
    else
        snprintf(buf,bufsize,"No message");
}

void format_request_line(Request* r,char* buf,int bufsize) {
    if(r->http_method)
        snprintf(buf,bufsize,"%s %s %s",r->http_method,r->http_uri,r->http_version);
    else
        snprintf(buf,bufsize,"GET /");
}

void format_status(Request* r,char* buf,int bufsize) {
    snprintf(buf,bufsize,"%d",r->response_status);
}

void format_bytes(Request* r,char* buf,int bufsize) {
    snprintf(buf,bufsize,"%d",r->response_bytes);
}




void hook_register(hook_function h) {
    if(hook_zzhi.count<MAX_TOKEN)
        hook_zzhi.hook[hook_zzhi.count++]=h;
}

void run_hook(Request* r) {
    int i;
    for(i=0; i<hook_zzhi.count; i++) {
        hook_zzhi.hook[i](r);
    }
}
