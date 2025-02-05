#include <stdio.h>
#include <windows.h>
#include <shellapi.h>
#include <stdint.h>
#include <tlhelp32.h>

// Function to get the process ID by its name
DWORD GetProcessIdByName(const char* processName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Error obtaining the snapshot of processes.\n");
        return 0;
    }

    PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };
    if (!Process32First(hSnapshot, &pe32))
    {
        fprintf(stderr, "Error obtaining information about the first process.\n");
        CloseHandle(hSnapshot);
        return 0;
    }

    DWORD processId = 0;

    do
    {
        if (strcmp(pe32.szExeFile, processName) == 0)
        {
            processId = pe32.th32ProcessID; // Update to get the most recent instance
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);

    return processId; // Return the last found PID
}

int main()
{
    const char* processName = "notepad.exe";
    const char* dllPath = "test.dll"; // Replace with the actual path of your DLL

    // Create a new instance of notepad.exe
    intptr_t result = (intptr_t)ShellExecuteA(NULL, "open", processName, NULL, NULL, SW_HIDE);
    if (result <= 32)
    {
        fprintf(stderr, "Error creating the process.\n");
        return 1;
    }

    // Get the process ID from the instance handle
    DWORD pid;
    GetWindowThreadProcessId((HWND)result, &pid);

    printf("Process created. PID: %lu\n", pid);

    // Search for the most recent process by its name and display its PID
    DWORD foundPid = GetProcessIdByName(processName);
    
    if (foundPid != 0)
    {
        printf("PID of the found process: %lu\n", foundPid);

        // Open the process with the necessary permissions
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, foundPid);
        if (!hProcess)
        {
            fprintf(stderr, "Error opening the process.\n");
            return 1;
        }

        // Allocate space in the process for the DLL name
        LPVOID dllPathMemory = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
        if (!dllPathMemory)
        {
            fprintf(stderr, "Error allocating memory in the process.\n");
            CloseHandle(hProcess);
            return 1;
        }

        // Write the DLL name into the memory of the process
        if (!WriteProcessMemory(hProcess, dllPathMemory, dllPath, strlen(dllPath) + 1, NULL))
        {
            fprintf(stderr, "Error writing the DLL name into the memory of the process.\n");
            CloseHandle(hProcess);
            return 1;
        }

        // Get the address of LoadLibraryA function from kernel32.dll
        HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
        if (!hKernel32)
        {
            fprintf(stderr, "Error obtaining handle for kernel32.dll.\n");
            CloseHandle(hProcess);
            return 1;
        }

        FARPROC pLoadLibraryA = GetProcAddress(hKernel32, "LoadLibraryA");
        if (!pLoadLibraryA)
        {
            fprintf(stderr, "Error obtaining address of LoadLibraryA function.\n");
            CloseHandle(hProcess);
            return 1;
        }

        // Create a remote thread in the process to load the DLL
        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryA, dllPathMemory, 0, NULL);
        if (!hThread)
        {
            fprintf(stderr, "Error creating remote thread in the process.\n");
            CloseHandle(hProcess);
            return 1;
        }

        // Wait for the remote thread to finish
        if (WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED)
        {
            fprintf(stderr, "Error waiting for remote thread to finish.\n");
            CloseHandle(hThread);
            CloseHandle(hProcess);
            return 1;
        }

        printf("DLL successfully injected and loaded.\n");

        // Clean up resources
        CloseHandle(hThread);
        CloseHandle(hProcess);
    }
    else
    {
        printf("The process was not found.\n");
    }

    return 0;
}
