/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef __RW_WIN_H__
#define __RW_WIN_H__


typedef struct
{
	HDC				hDC;				// global DC we're using
	HWND			hWnd;				// HWND of parent window

	HDC				hdcDIBSection;		// DC compatible with DIB section
	HBITMAP			hDIBSection;		// DIB section
	unsigned char	*pDIBBase;			// DIB base pointer, NOT used directly for rendering!

	HPALETTE		hPal;				// palette we're using
	HPALETTE		hpalOld;			// original system palette
	COLORREF		oldsyscolors[20];	// original system colors

	HINSTANCE		hinstDDRAW;			// library instance for DDRAW.DLL
	IDirectDraw		*DirectDraw;		// pointer to DirectDraw object

	IDirectDrawSurface *ddsFrontBuffer;	// video card display memory front buffer
	IDirectDrawSurface *ddsBackBuffer;	// system memory backbuffer
	IDirectDrawSurface *ddsOffScreenBuffer;	// system memory backbuffer
	IDirectDrawPalette *ddpPalette;		// DirectDraw palette

	bool			palettized;			// true if desktop is paletted
	bool			modex;

	bool			initializing;
} swwstate_t;

extern swwstate_t sww_state;

/*
** DIB code
*/
bool	DIB_Init( byte **ppbuffer, int *ppitch );
void    DIB_Shutdown( void );
void    DIB_SetPalette( const byte *palette );

bool	DDRAW_Init( byte **ppbuffer, int *ppitch );
void    DDRAW_Shutdown( void );
void    DDRAW_SetPalette( const byte *palette );

#endif
