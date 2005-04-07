#ifndef __GLW_WIN_H__
#define __GLW_WIN_H__

typedef struct
{
	HWND    hWnd;		// handle to window
	HDC     hDC;		// handle to device context
} glwstate_t;

extern glwstate_t glw_state;

#endif
