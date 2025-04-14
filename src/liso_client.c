/******************************************************************************
* echo_client.c                                                               *
*                                                                             *
* Description: This file contains the C source code for an echo client.       *
*              The client connects to an arbitrary <host,port> and sends input*
*              from a specified file.                                        *
*                                                                             *
* Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                         *
*          Wolf Richter <wolf@cs.cmu.edu>                                     *
*          Modified by ChatGPT                                                *
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#define BUF_SIZE 4096
#define DEFAULT_DIR "samples/"

// 将文件内容读取到 buffer 中，filePath 为完整路径
void read_from_file(const char* filePath, char* buffer) {
    FILE* file = fopen(filePath, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    // 读取文件内容到 buffer 中
    size_t num_read = fread(buffer, 1, fileSize, file);
    if (num_read != fileSize) {
        fprintf(stderr, "Warning: Expected to read %ld bytes but read %zu bytes\n", fileSize, num_read);
    }

    buffer[fileSize] = '\0';  // 确保buffer是一个以 '\0' 结尾的C字符串

    fclose(file);
}

int main(int argc, char* argv[])
{
    if(argc != 4) {
        fprintf(stderr, "Usage: %s <server-ip> <port> <file-name>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char buf[BUF_SIZE];

    int status, sock;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    struct addrinfo* servinfo; // will point to the results
    hints.ai_family = AF_INET;         // IPv4
    hints.ai_socktype = SOCK_STREAM;   // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;       // fill in my IP for me

    if ((status = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return EXIT_FAILURE;
    }

    if ((sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
        fprintf(stderr, "Socket creation failed\n");
        return EXIT_FAILURE;
    }

    if (connect(sock, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        fprintf(stderr, "Connect failed\n");
        return EXIT_FAILURE;
    }

    // 构造完整文件路径：如果 argv[3] 中不含路径分隔符，则默认加上 DEFAULT_DIR 前缀
    char filePath[256] = {0};
    if (strchr(argv[3], '/') == NULL) {
        snprintf(filePath, sizeof(filePath), "%s%s", DEFAULT_DIR, argv[3]);
    } else {
        // 如果包含路径则直接使用
        strncpy(filePath, argv[3], sizeof(filePath)-1);
    }

    char msg[BUF_SIZE];
    // 从指定的文件中读取内容
    read_from_file(filePath, msg);

    int bytes_received;
    fprintf(stdout, "-------Sending-------\n");
    fprintf(stdout, "%s", msg);

    send(sock, msg, strlen(msg), 0);

    if ((bytes_received = recv(sock, buf, BUF_SIZE, 0)) > 0) {
        buf[bytes_received] = '\0';
        fprintf(stdout, "-------Received-------\n");
        fprintf(stdout, "%s", buf);
    }

    freeaddrinfo(servinfo);
    close(sock);
    return EXIT_SUCCESS;
}
