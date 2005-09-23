#ifndef __WINQUAKE_INCLUDED__
#define __WINQUAKE_INCLUDED__

//#define WINVER 0x0500				// to allow some additional stuff in windows headers (current: will display "beta headers" msg)
#define WIN32_LEAN_AND_MEAN			// exclude rarely-used services from windown headers
#include <windows.h>
#include <mmsystem.h>

// stuff from Win98+ and Win2000+

#define SPI_GETMOUSESPEED         0x0070
#define SPI_SETMOUSESPEED         0x0071
#define WM_MOUSEWHEEL             0x020A


// Remove some MS defines from <windows.h>

#undef SHIFT_PRESSED
#undef MOD_ALT

#include "qcommon.h"

extern HWND cl_hwnd;
extern bool	ActiveApp, MinimizedApp, FullscreenApp;

// in_win.c
void IN_Activate (bool active);	//?? declared in cl_input.h too
extern bool in_needRestart;		//?? used from vid_dll.c::Vid_NewVindow()


// message hooks
typedef bool (*MSGHOOK_FUNC) (UINT uMsg, WPARAM wParam, LPARAM lParam);

void AddMsgHook (MSGHOOK_FUNC func);
void RemoveMsgHook (MSGHOOK_FUNC func);


#endif
