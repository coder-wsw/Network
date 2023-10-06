#include <spdlog/spdlog.h>
#include <stdio.h>
#include <windows.h>
#include <zlib.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 7777

int main()
{
    WSADATA wsaData;
    SOCKET listenSocket, clientSocket;
    SOCKADDR_IN serverAddr, clientAddr;

    WSAStartup(MAKEWORD(2, 0), &wsaData);

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));

    listen(listenSocket, SOMAXCONN);

    while (1) {
        int clientLen = sizeof(clientAddr);
        clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &clientLen);

        char request[1024];
        recv(clientSocket, request, 1024, 0);
        FILE* htmlFile;
        if (strstr(request, "Accept-Encoding: gzip")) {
            if ((htmlFile = fopen("index.html", "rb")) == NULL) {
                printf("File open error");
                return 0;
            }

            fseek(htmlFile, 0, SEEK_END); // 将文件指针移到文件末尾
            long file_size =
                ftell(htmlFile); // 获取当前文件指针位置，即文件长度
            rewind(htmlFile); // 将文件指针移到文件开头
            SPDLOG_INFO("file_size: {}", file_size);
            char responseBody[2048];
            size_t total_in = 0;
            auto readSize = fread(responseBody, 1, file_size, htmlFile);
            rewind(htmlFile);
            SPDLOG_INFO("readSize: {}", readSize);
            char response[2048];
            // sprintf(response,
            //         "HTTP/1.1 200 OK\r\n"
            //         "Content-Length: %ld\r\n\r\n",
            //         4892);
            // send(clientSocket, response, strlen(response), 0);
            // while (total_in < file_size) {
            //     char tmp[1024];
            //     auto tmpSize = fread(tmp, 1, 100, htmlFile);
            //     SPDLOG_INFO("tmpSize: {}", tmpSize);
            //     SPDLOG_INFO("strlen(tmp): {}", strlen(tmp));
            //     send(clientSocket, tmp, tmpSize, 0);
            //     total_in += tmpSize;
            // }

            char compressed[1024];

            z_stream stream;
            stream.zalloc = Z_NULL;
            stream.zfree = Z_NULL;
            stream.opaque = Z_NULL;
            deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8,
                         Z_DEFAULT_STRATEGY);
            stream.next_in = (Bytef*)responseBody;
            stream.avail_in = strlen(responseBody);
            stream.next_out = (Bytef*)compressed;
            stream.avail_out = 2048;
            deflate(&stream, Z_FINISH);
            deflateEnd(&stream);

            sprintf(response,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Encoding: gzip\r\n"
                    "Content-Length: %ld\r\n\r\n",
                    stream.total_out);
            send(clientSocket, response, strlen(response), 0);
            send(clientSocket, compressed, stream.total_out, 0);
        }
        else {
            const char* response = "HTTP/1.1 200 OK\r\n"
                                   "Content-Length: 12\r\n\r\n"
                                   "Hello World!";
            send(clientSocket, response, strlen(response), 0);
        }

        closesocket(clientSocket);
    }

    closesocket(listenSocket);
    WSACleanup();

    return 0;
}