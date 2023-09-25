#include <iostream>
#include <process.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

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
    int szClntAddr;
    HANDLE hThread;
    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        error_handling("WSAStartup() error!");
    SPDLOG_INFO("WAADATA: {},{},{},{}", wsaData.wVersion, wsaData.wHighVersion,
                wsaData.szDescription, wsaData.szSystemStatus);
    hServSock = socket(PF_INET, SOCK_STREAM, 0);
    if (hServSock == INVALID_SOCKET)
        error_handling("socket() error!");
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(atoi(argv[1]));
    if (bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
        error_handling("bind() error!");
    if (listen(hServSock, 5) == SOCKET_ERROR)
        error_handling("listen() error!");
    while (1) {
        szClntAddr = sizeof(clntAddr);
        hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &szClntAddr);
        hThread = (HANDLE)_beginthreadex(NULL, 0, RequestHandler,
                                         (void*)&hClntSock, 0, NULL);

        char reqLine[BUF_SIZE];
        recv(hClntSock, reqLine, BUF_SIZE, 0);
        std::cout << "reqLine: " << reqLine << std::endl;
        printf("Connected client IP: %s \n", inet_ntoa(clntAddr.sin_addr));
    }
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
    recv(hClntSock, reqLine, BUF_SIZE, 0);
    // std::cout << "reqLine: " << reqLine << std::endl;
    // printf("reqLine: %s \n",reqLine);
    // SPDLOG_INFO("reqLine: {}", reqLine);
    if (strstr(reqLine, "HTTP/") == NULL) {
        SendErrorMSG(hClntSock);
        closesocket(hClntSock);
        return 1;
    }
    strcpy(method, strtok(reqLine, " /"));
    if (strcmp(method, "GET")) {
        SendErrorMSG(hClntSock);
        closesocket(hClntSock);
        return 1;
    }
    strcpy(fileName, strtok(NULL, " /"));
    strcpy(ct, ContentType(fileName));
    SendData(hClntSock, ct, fileName);
    return 0;
}

void SendData(SOCKET sock, char* ct, char* filename)
{
    char protocol[] = "HTTP/1.0 200 OK\r\n";
    char server[] = "Server:Linux Web Server \r\n";
    char cntLen[] = "Content-length:2048\r\n";
    char cntType[BUF_SMALL_SIZE];
    char buf[BUF_SIZE];
    FILE* sendFile;

    sprintf(cntType, "Content-type:%s\r\n\r\n", ct);
    if ((sendFile = fopen(filename, "rb")) == NULL) {
        SendErrorMSG(sock);
        return;
    }

    send(sock, protocol, strlen(protocol), 0);
    send(sock, server, strlen(server), 0);
    send(sock, cntLen, strlen(cntLen), 0);
    send(sock, cntType, strlen(cntType), 0);

    while (fgets(buf, BUF_SIZE, sendFile) != NULL)
        send(sock, buf, strlen(buf), 0);
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