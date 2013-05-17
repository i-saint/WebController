// created by i-saint
// distributed under Creative Commons Attribution (CC BY) license.
// https://github.com/i-saint/WebController

#include "WebController.h"
#include "WebController_Internal.h"

// VirtualAllocEx で dll の path を対象プロセスに書き込み、
// CreateRemoteThread で対象プロセスにスレッドを作って、↑で書き込んだ dll path をパラメータにして LoadLibraryA を呼ぶ。
// 結果、対象プロセス内で任意の dll を実行させる。 
bool InjectDLL(HANDLE hProcess, const char* dllname)
{
    SIZE_T bytesRet = 0;
    DWORD oldProtect = 0;
    LPVOID remote_addr = NULL;
    HANDLE hThread = NULL;
    size_t len = strlen(dllname) + 1;

    remote_addr = ::VirtualAllocEx(hProcess, 0, 1024, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if(remote_addr==NULL) { return false; }
    ::VirtualProtectEx(hProcess, remote_addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
    ::WriteProcessMemory(hProcess, remote_addr, dllname, len, &bytesRet);
    ::VirtualProtectEx(hProcess, remote_addr, len, oldProtect, &oldProtect);

    hThread = ::CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)((void*)&LoadLibraryA), remote_addr, 0, NULL);
    ::WaitForSingleObject(hThread, INFINITE); 
    ::VirtualFreeEx(hProcess, remote_addr, 0, MEM_RELEASE);
    return true;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE prev, LPWSTR cmd, int show)
{
    char exePath[MAX_PATH] = {0};
    char dllPath[MAX_PATH] = {0};

    OPENFILENAMEA fn;
    ::ZeroMemory(&fn, sizeof(fn));
    fn.lStructSize = sizeof(fn);
    fn.lpstrFilter = "Executables\0*.exe\0";
    fn.nFilterIndex = 1;
    fn.lpstrFile = exePath;
    fn.nMaxFile = MAX_PATH;
    fn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;

    if(GetOpenFileNameA(&fn)) {
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ::ZeroMemory(&si, sizeof(si));
        ::ZeroMemory(&pi, sizeof(pi));
        si.cb = sizeof(si);
        BOOL ret = ::CreateProcessA(exePath, NULL, NULL, NULL, FALSE,
            NORMAL_PRIORITY_CLASS|CREATE_SUSPENDED, NULL, NULL, &si, &pi);
        if(ret) {
            GetModuleDirectory(dllPath, MAX_PATH);
            std::string dll = std::string(dllPath)+"WebController.dll";
            InjectDLL(pi.hProcess, dll.c_str());
            ::ResumeThread(pi.hThread);
        }
    }
    return 0;
}
