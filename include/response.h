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

//响应头结构定义

extern char *ret_head;
extern char *ret_head2;

extern Response* make_response(Request* request,char addr[]);

extern void free_Response(Response * r);

extern int implementPOST(Response* r,Request* request);
extern int implementHEAD(Response* r,Request* request);
extern int implementGET(Response* r,Request* request);
extern int implementUKOWN(Response* r,Request* request);
