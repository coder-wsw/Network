#include <iostream>
#include <process.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <winsock2.h>
#include <zlib.h>

#define BUF_SIZE 1024
#define BUF_SMALL_SIZE 100

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
void error_handling(const char* message);

unsigned WINAPI RequestHandler(void* arg);
char* ContentType(char* file);
void SendData(SOCKET sock, char* ct, char* fileName);
void SendErrorMSG(SOCKET sock);
void ErrorHandling(const char* message);

int main(int argc, char* argv[])
{
    SPDLOG_INFO("argc: {}", argc);
    SPDLOG_INFO("argv[0]: {}", argv[0]);

    WSADATA wsaData;
    SOCKET hServSock, hClntSock;
    SOCKADDR_IN servAddr, clntAddr;
    HANDLE hThread;
    int szClntAddr;
    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        error_handling("WSAStartup() error!");
    hServSock = socket(PF_INET, SOCK_STREAM, 0);
    if (hServSock == INVALID_SOCKET)
        error_handling("socket() error!");
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(atoi(argv[1]));
    // SPDLOG_INFO("servAddr.addr: {}", servAddr.sin_addr.s_addr);
    if (bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
        error_handling("bind() error!");
    if (listen(hServSock, 1) == SOCKET_ERROR)
        error_handling("listen() error!");
    while (1) {
        hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &szClntAddr);
        if (hClntSock < 0) {
            if (errno == EAGAIN || errno == EMFILE || errno == ENFILE)
                break;
            else if (errno == ECONNABORTED)
                continue;
            else
                break;
        }
        std::thread t(RequestHandler, (void*)&hClntSock);
        t.detach();
        printf("Connected client IP: %s \n", inet_ntoa(clntAddr.sin_addr));
    }
    SPDLOG_INFO("close socket");
    closesocket(hServSock);
    WSACleanup();
    return 0;
}

unsigned WINAPI RequestHandler(void* arg)
{
    SOCKET hClntSock = *((SOCKET*)arg);
    char reqLine[BUF_SIZE];
    char method[BUF_SMALL_SIZE];
    char ct[BUF_SMALL_SIZE];
    char fileName[BUF_SMALL_SIZE];
    // SPDLOG_INFO("hClntSock: {}", hClntSock);
    auto rtn = recv(hClntSock, reqLine, BUF_SIZE, 0);
    if (rtn == SOCKET_ERROR) {
        SPDLOG_INFO("recv error");
        return 1;
    }
    SPDLOG_INFO("reqLine: {}", reqLine);
    if (strstr(reqLine, "HTTP/") == NULL) {
        SendErrorMSG(hClntSock);
        closesocket(hClntSock);
        return 1;
    }
    strcpy_s(method, strtok(reqLine, " /"));
    if (strcmp(method, "GET")) {
        SendErrorMSG(hClntSock);
        closesocket(hClntSock);
        return 1;
    }
    strcpy(fileName, strtok(NULL, " /"));
    SPDLOG_INFO("fileName: {}", fileName);
    strcpy(ct, ContentType(fileName));
    SendData(hClntSock, ct, fileName);
    return 0;
}

void SendData(SOCKET sock, char* ct, char* filename)
{
    char protocol[] = "HTTP/1.1 200 OK\r\n";
    char server[] = "Server:Linux Web Server\r\n";
    char cntEnc[] = "Content-Encoding: gzip\r\n";
    char cntType[BUF_SMALL_SIZE];
    char buf[BUF_SIZE];
    z_stream c_stream;
    size_t total_in = 0, gzip_in, bound_size;

    c_stream.zalloc = NULL;
    c_stream.zfree = NULL;
    c_stream.opaque = NULL;
    if (deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8,
                     Z_DEFAULT_STRATEGY) != Z_OK) {
        errno = EBADMSG;
        return;
    }
    c_stream.avail_in = 0;
    c_stream.avail_out = 0;
    c_stream.total_in = 0;

    gzip_in = c_stream.total_in;
    FILE* sendFile;
    sprintf(cntType, "Content-type:%s;\r\n\r\n", ct);
    if ((sendFile = fopen(filename, "rb")) == NULL) {
        SendErrorMSG(sock);
        return;
    }

    fseek(sendFile, 0, SEEK_END); // 将文件指针移到文件末尾
    long file_size = ftell(sendFile); // 获取当前文件指针位置，即文件长度
    rewind(sendFile);                 // 将文件指针移到文件开头

    char cntLen[BUF_SMALL_SIZE];
    sprintf(cntLen, "Content-length:%ld\r\n", 1024);
    send(sock, protocol, strlen(protocol), 0);
    send(sock, server, strlen(server), 0);
    send(sock, cntLen, strlen(cntLen), 0);
    send(sock, cntEnc, strlen(cntEnc), 0);
    send(sock, cntType, strlen(cntType), 0);

    while (total_in < file_size) {
        char* dest;
        if (c_stream.avail_in == 0) {
            auto size = fread(buf, 1, BUF_SIZE, sendFile);
            c_stream.next_in = (Bytef*)buf;
            c_stream.avail_in = size;
        }

        if (c_stream.avail_out == 0) {
            bound_size = compressBound(c_stream.avail_in);
            dest = (char*)malloc(bound_size);
            c_stream.next_out = (Bytef*)dest;
            c_stream.avail_out = bound_size;
        }

        if (deflate(&c_stream, Z_NO_FLUSH) != Z_OK) {
            errno = EBADMSG;
            return;
        }
        int encoded_size = c_stream.total_out - gzip_in;
        send(sock, dest, encoded_size, 0);
        sprintf(cntLen, "Content-length:%d\r\n", encoded_size);
        send(sock, cntLen, strlen(cntLen), 0);

        free(dest);
        total_in += c_stream.total_in - gzip_in;
        gzip_in = c_stream.total_in;
    }

    deflateEnd(&c_stream);
    fclose(sendFile);
    closesocket(sock);
}

void SendErrorMSG(SOCKET sock)
{
    char protocol[] = "HTTP/1.0 400 Bad Request\r\n";
    char server[] = "Server:Linux Web Server \r\n";
    char cntLen[] = "Content-length:2048\r\n";
    char cntType[] = "Content-type:text/html\r\n\r\n";
    char content[] =
        "<html><head><title>NETWORK</title></head>"
        "<body><font size=+5><br>发生错误！查看请求文件名和请求方式！"
        "</font></body></html>";
    send(sock, protocol, strlen(protocol), 0);
    send(sock, server, strlen(server), 0);
    send(sock, cntLen, strlen(cntLen), 0);
    send(sock, cntType, strlen(cntType), 0);
    send(sock, content, strlen(content), 0);
    closesocket(sock);
}

char* ContentType(char* file)
{
    char extension[BUF_SMALL_SIZE];
    char fileName[BUF_SMALL_SIZE];
    strcpy(fileName, file);
    strtok(fileName, ".");
    strcpy(extension, strtok(NULL, "."));
    if (!strcmp(extension, "html") || !strcmp(extension, "htm"))
        return "text/html";
    else
        return "text/plain";
}

void ErrorHandling(const char* message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void error_handling(const char* message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}