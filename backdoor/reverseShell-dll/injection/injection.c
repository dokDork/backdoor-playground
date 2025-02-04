#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>

// Get the process ID by process name
int getPIDbyProcName(const char* procName) {
    int pid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hSnap, &pe32) != FALSE) {
        while (pid == 0 && Process32Next(hSnap, &pe32) != FALSE) {
            if (strcmp(pe32.szExeFile, procName) == 0) {
                pid = pe32.th32ProcessID;
            }
        }
    }
    CloseHandle(hSnap);
    return pid;
}

const char* DLL_NAME = "test.dll";

typedef LPVOID Memory;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HWND hWnd = GetConsoleWindow(); // Get the console window handle
    ShowWindow(hWnd, SW_HIDE); // Hide the console window
    // Get the process ID by the process name (in this case, "notepad.exe")
    int processId = getPIDbyProcName("notepad.exe");

    if (processId == 0) {
        printf("Process not found\n");
        return 1;
    }

    // Open a handle to the process with full access permissions
    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (processHandle == NULL) {
        printf("Error opening handle\n");
        return 1;
    }

    char dllPath[MAX_PATH];
    GetFullPathNameA(DLL_NAME, MAX_PATH, dllPath, NULL);

    // Allocate memory in the process to store the DLL path
    Memory dllPathAddress = VirtualAllocEx(processHandle, NULL, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (dllPathAddress == NULL) {
        printf("Error creating memory\n");
        CloseHandle(processHandle);
        return 1;
    }

    // Write the DLL name to the allocated memory of the process
    if (!WriteProcessMemory(processHandle, dllPathAddress, dllPath, strlen(dllPath) + 1, NULL)) {
        printf("Failed to write the DLL name to allocated memory\n");
        VirtualFreeEx(processHandle, dllPathAddress, 0, MEM_RELEASE);
        CloseHandle(processHandle);
        return 1;
    }

    // Get the address of the LoadLibraryA function from kernel32.dll
    Memory loadLibraryAddress = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
    if (loadLibraryAddress == NULL) {
        printf("Failed to get the address of LoadLibraryA\n");
        VirtualFreeEx(processHandle, dllPathAddress, 0, MEM_RELEASE);
        CloseHandle(processHandle);
        return 1;
    }

    // Create a remote thread in the process to load the DLL
    HANDLE remoteThread = CreateRemoteThread(processHandle, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddress, dllPathAddress, 0, NULL);
    if (remoteThread == NULL) {
        printf("Error injecting DLL\n");
        VirtualFreeEx(processHandle, dllPathAddress, 0, MEM_RELEASE);
        CloseHandle(processHandle);
        return 1;
    }

    // Wait for the remote thread to complete execution
    WaitForSingleObject(remoteThread, INFINITE);

    CloseHandle(remoteThread);
    VirtualFreeEx(processHandle, dllPathAddress, 0, MEM_RELEASE);
    CloseHandle(processHandle);

    printf("DLL injection completed\n");
    return 0;
}
