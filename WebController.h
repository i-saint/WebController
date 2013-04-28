#ifndef WebController_h
#define WebController_h
#pragma comment(lib, "psapi.lib")
#include <windows.h>
#include <psapi.h>
#include <stdint.h>
#include <vector>
#include <regex>

typedef char            int8;
typedef short           int16;
typedef int             int32;
typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned int    uint32;

extern "C" {

    struct wcConfig
    {
        bool override_user32;
        bool override_winmm;
        bool override_dinput8;
        bool override_xinput;
    };

    struct wcInputData
    {
        static const int32 MaxPads = 8;
        struct Keyboard
        {
            uint8 keys[256];
        };

        struct Mouse
        {
            int32 x,y;
            uint32 buttons;
        };

        struct Pad
        {
            static const int32 MaxButtons = 16;
            int32 x1,y1;
            int32 x2,y2;
            int32 pov;
            int32 trigger1;
            int32 trigger2;
            uint8 buttons[MaxButtons];
        };

        Keyboard key;
        Mouse    mouse;
        Pad      pad[MaxPads];
    };

    __declspec(dllexport) wcConfig*     wcGetConfig();
    __declspec(dllexport) bool          wcStartServer();
    __declspec(dllexport) bool          wcStopServer();
    __declspec(dllexport) wcInputData*  wcGetData();
} // extern "C"


#endif // WebController_h
