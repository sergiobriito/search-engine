#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring> 

using namespace std;

class TCPServer{
public:
    struct sockaddr_in address;
    string host;
    int port;

    TCPServer(const string& host,const int port) : host(host), port(port) {};

    void start(){
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(host.c_str());
        address.sin_port = htons(port);
        socklen_t addr_len = sizeof(address);

        if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
            perror("Bind failed");
            close(serverSocket);
            exit(EXIT_FAILURE);
        }

        listen(serverSocket, 10);
        getsockname(serverSocket, (struct sockaddr*)&address, &addr_len);
        cout << "Listening at " << inet_ntoa(address.sin_addr) << ":" << ntohs(address.sin_port) << std::endl;

        while (true){
            struct sockaddr_in clientAddr;
            int client = accept(serverSocket, nullptr, nullptr);
            socklen_t clientAddrLen = sizeof(clientAddr);
            getpeername(client, (struct sockaddr*)&clientAddr, &clientAddrLen);

            cout << "Accepted connection from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << endl;

            char buffer[1024];  
            int bytesRead = recv(client, buffer, sizeof(buffer), 0);
            buffer[bytesRead] = '\0';

            string message = handleRequest(string(buffer,bytesRead));
            send(client, message.c_str(), strlen(message.c_str()), 0);
            close(client);
        };

        close(serverSocket);
    };

    virtual string handleRequest(string data) = 0;
};

#endif