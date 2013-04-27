#include <windows.h>
#include "WebController.h"
#include "WebController_Internal.h"

typedef BOOL (WINAPI *GetCursorInfoT)(PCURSORINFO pci);
typedef BOOL (WINAPI *GetKeyboardStateT)(PBYTE lpKeyState);
static GetCursorInfoT orig_GetCursorInfo;
static GetKeyboardStateT orig_GetKeyboardState;

static BOOL WINAPI fake_GetCursorInfo(PCURSORINFO pci)
{
    BOOL ret = orig_GetCursorInfo(pci);
    if(WebController_GetConfig()->override_user32) {
        WebController_StartServer();
        const WebControllerData *input = WebController_GetData();
        // todo
    }
    return ret;
}

static BOOL WINAPI fake_GetKeyboardState(PBYTE lpKeyState)
{
    BOOL ret = orig_GetKeyboardState(lpKeyState);
    if(WebController_GetConfig()->override_user32) {
        WebController_StartServer();
        const WebControllerData *input = WebController_GetData();
        for(int i=0; i<256; ++i) {
            lpKeyState[i] |= (input->key.keys[i] & 0x80);
        }
    }
    return ret;
}

static FuncInfo g_user32_funcs[] = {
    {"GetCursorInfo", 0, (void*)&fake_GetCursorInfo, (void**)&orig_GetCursorInfo},
    {"GetKeyboardState", 0, (void*)&fake_GetKeyboardState, (void**)&orig_GetKeyboardState},
};
OverrideInfo g_user32_overrides = {"user32.dll", _countof(g_user32_funcs), g_user32_funcs};
