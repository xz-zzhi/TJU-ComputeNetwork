#include "http_data.h"

int http_code[]={
    [code400] 400,
    [code501] 501,
    [code200] 200,
    [code505] 505,
    [code404] 404,
};

char* error_msg[]={
    [code400] "HTTP/1.1 400 Bad request\r\n",
    [code404] "HTTP/1.1 404 Not Found\r\n",
    [code200] "HTTP/1.1 200 OK\r\n",
    [code501] "HTTP/1.1 501 Not Implemented\r\n",
    [code505] "HTTP/1.1 505 HTTP Version not supported\r\n"
};

char *defaut_address="static_site";