// created by i-saint
// distributed under Creative Commons Attribution (CC BY) license.
// https://github.com/i-saint/WebController

#include <windows.h>
#include "WebController.h"
#include "WebController_Internal.h"

extern wcOverrideInfo g_user32_overrides;
extern wcOverrideInfo g_winmm_overrides;
extern wcOverrideInfo g_dinput8_overrides;
extern wcOverrideInfo g_xinput_overrides;
static wcOverrideInfo *g_overrides[] = {&g_user32_overrides, &g_winmm_overrides, &g_dinput8_overrides, &g_xinput_overrides};

typedef HMODULE (WINAPI *LoadLibraryAT)(LPCSTR lpFileName);
typedef HMODULE (WINAPI *LoadLibraryWT)(LPWSTR lpFileName);
typedef HMODULE (WINAPI *LoadLibraryExAT)(LPCSTR lpFileName, HANDLE hFile, DWORD dwFlags);
typedef HMODULE (WINAPI *LoadLibraryExWT)(LPWSTR lpFileName, HANDLE hFile, DWORD dwFlags);
static LoadLibraryAT orig_LoadLibraryA;
static LoadLibraryWT orig_LoadLibraryW;
static LoadLibraryExAT orig_LoadLibraryExA;
static LoadLibraryExWT orig_LoadLibraryExW;

static void OverrideExports(HMODULE mod)
{
    if(mod==nullptr) { return; }

    char pDLLpath[MAX_PATH] = {0};
    DWORD size = ::GetModuleFileNameA(mod, pDLLpath, MAX_PATH);
    const char *pDLLName = pDLLpath;
    for(size_t i=0; ; ++i) {
        if(pDLLpath[i]=='\0') { break; }
        if(pDLLpath[i]=='\\' || pDLLpath[i]=='/') { pDLLName = pDLLpath+(i+1); }
    }
    for(size_t mi=0; mi<_countof(g_overrides); ++mi) {
        wcOverrideInfo &oinfo = *g_overrides[mi];
        std::regex reg(oinfo.dllname, std::regex::ECMAScript|std::regex::icase);
        if(std::regex_match(pDLLName, reg)) {
            for(size_t fi=0; fi<oinfo.num_funcs; ++fi) {
                wcFuncInfo &finfo = oinfo.funcs[fi];
                if(finfo.name!=nullptr) {
                    void *orig = OverrideDLLExport(mod, finfo.name, finfo.func);
                    if(*finfo.func_orig==nullptr) { *finfo.func_orig=orig; }
                }
            }
        }
    }
}

static void OverrideImports()
{
    for(size_t mi=0; mi<_countof(g_overrides); ++mi) {
        wcOverrideInfo &oinfo = *g_overrides[mi];
        EnumerateDLLImportsEveryModule(oinfo.dllname,
            [&](const char *funcname, void *&func) {
                for(size_t fi=0; fi<oinfo.num_funcs; ++fi) {
                    wcFuncInfo &finfo = oinfo.funcs[fi];
                    if(finfo.name==nullptr) { continue; }
                    if(strcmp(funcname, finfo.name)==0) {
                        if(*finfo.func_orig==nullptr) { *finfo.func_orig=func; }
                        ForceWrite<void*>(func, finfo.func);
                    }
                }
        },
            [&](DWORD ordinal, void *&func) {
                for(size_t fi=0; fi<oinfo.num_funcs; ++fi) {
                    wcFuncInfo &finfo = oinfo.funcs[fi];
                    if(finfo.ordinal==0) { continue; }
                    if(finfo.ordinal==ordinal) {
                        if(*finfo.func_orig==nullptr) { *finfo.func_orig=func; }
                        ForceWrite<void*>(func, finfo.func);
                    }
                }
        });
    }
}


static HMODULE WINAPI fake_LoadLibraryA(LPCSTR lpFileName)
{
    HMODULE ret = orig_LoadLibraryA(lpFileName);
    OverrideExports(ret);
    return ret;
}

static HMODULE WINAPI fake_LoadLibraryW(LPWSTR lpFileName)
{
    HMODULE ret = orig_LoadLibraryW(lpFileName);
    OverrideExports(ret);
    return ret;
}

static HMODULE WINAPI fake_LoadLibraryExA(LPCSTR lpFileName, HANDLE hFile, DWORD dwFlags)
{
    HMODULE ret = orig_LoadLibraryExA(lpFileName, hFile, dwFlags);
    OverrideExports(ret);
    return ret;
}

static HMODULE WINAPI fake_LoadLibraryExW(LPWSTR lpFileName, HANDLE hFile, DWORD dwFlags)
{
    HMODULE ret = orig_LoadLibraryExW(lpFileName, hFile, dwFlags);
    OverrideExports(ret);
    return ret;
}


static void WebController_SetHooks()
{
    bool ok = false;
    if(HMODULE kernelbase=::GetModuleHandleA("KernelBase.dll")) {
        if(void *fp=GetProcAddress(kernelbase, "LoadLibraryExW")) {
            (void*&)orig_LoadLibraryExW = Hotpatch(fp, fake_LoadLibraryExW);
            ok = true;
        }
    }
    if(!ok) {
        (void*&)orig_LoadLibraryA = Hotpatch(&LoadLibraryA, fake_LoadLibraryA);
        (void*&)orig_LoadLibraryW = Hotpatch(&LoadLibraryW, fake_LoadLibraryW);
    }
    OverrideImports();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if(fdwReason==DLL_PROCESS_ATTACH) {
        WebController_SetHooks();
        ::LoadLibraryA("user32.dll");
        ::LoadLibraryA("winmm.dll");
        ::LoadLibraryA("dinput8.dll");
        ::LoadLibraryA("xinput1_3.dll");
    }
    else if(fdwReason==DLL_PROCESS_DETACH) {
        //StopWebControllerServer();
    }
    return TRUE;
}