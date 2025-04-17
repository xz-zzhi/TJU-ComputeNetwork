#include "parse.h"
#include "Apache_log.h"
#include "http_data.h"
#include "response.h"

char* ret_head="Server: Liso/1.0\r\nDate: Mon, 14 Apr 2025 23:47:44 CST\r\n";
char* ret_head2="Last-modified: Thu, 24 Feb 2022 10:20:31 GMT\r\nConnection: keep-alive\r\n";

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
        strcat(r->path,"/static_site/index.html");
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