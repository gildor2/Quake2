#ifndef __GLW_WIN_H__
#define __GLW_WIN_H__

typedef struct
{
	HDC     hDC;		// handle to device context
	HWND    hWnd;		// handle to window
} glwstate_t;

extern glwstate_t glw_state;

#endif
