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
/*
** RW_DDRAW.C
**
** This handles DirectDraw management under Windows.
*/
#ifndef _WIN32
#  error You should not be compiling this file on this platform
#endif

#include <windows.h>
#include <ddraw.h>
#include "../qcommon/qcommon.h"

#include "rw_win.h"
#include "../ref_soft/r_local.h"
//-- (see in_win.c) #define INITGUID

static const char *DDrawError( int code );

/*
** DDRAW_Init
**
** Builds our DDRAW stuff
*/
bool DDRAW_Init( unsigned char **ppbuffer, int *ppitch )
{
	HRESULT ddrval;
	DDSURFACEDESC ddsd;
	DDSCAPS ddscaps;
	PALETTEENTRY palentries[256];
	int i;
	extern cvar_t *sw_allow_modex;

	typedef HRESULT (WINAPI * pDirectDrawCreate_t) (GUID FAR *, LPDIRECTDRAW FAR *, IUnknown FAR *);
	pDirectDrawCreate_t pDirectDrawCreate;
//	HRESULT (WINAPI *QDirectDrawCreate)( GUID FAR *lpGUID, LPDIRECTDRAW FAR * lplpDDRAW, IUnknown FAR * pUnkOuter );

	Com_Printf ("Initializing DirectDraw\n");


	for ( i = 0; i < 256; i++ )
	{
		palentries[i].peRed		= ( d_8to24table[i] >> 0  ) & 0xff;
		palentries[i].peGreen	= ( d_8to24table[i] >> 8  ) & 0xff;
		palentries[i].peBlue	= ( d_8to24table[i] >> 16 ) & 0xff;
	}

	/*
	** load DLL and fetch pointer to entry point
	*/
	if ( !sww_state.hinstDDRAW )
	{
		Com_Printf ("...loading DDRAW.DLL: ");
		if ( ( sww_state.hinstDDRAW = LoadLibrary( "ddraw.dll" ) ) == NULL )
		{
			Com_WPrintf ("failed\n");
			goto fail;
		}
		Com_Printf ("ok\n" );
	}

	if ( ( pDirectDrawCreate = (pDirectDrawCreate_t) GetProcAddress( sww_state.hinstDDRAW, "DirectDrawCreate" ) ) == NULL )
	{
		Com_WPrintf ("*** DirectDrawCreate == NULL ***\n" );
		goto fail;
	}

	/*
	** create the direct draw object
	*/
	Com_Printf ("...creating DirectDraw object: ");
	if ( ( ddrval = pDirectDrawCreate (NULL, &sww_state.DirectDraw, NULL)) != DD_OK)
	{
		Com_WPrintf ("failed - %s\n", DDrawError( ddrval ) );
		goto fail;
	}
	Com_Printf ("ok\n");

	/*
	** see if linear modes exist first
	*/
	sww_state.modex = false;

	Com_Printf ("...setting exclusive mode: ");
	if ( ( ddrval = sww_state.DirectDraw->SetCooperativeLevel(sww_state.hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN ) ) != DD_OK )
	{
		Com_WPrintf ("failed - %s\n",DDrawError (ddrval) );
		goto fail;
	}
	Com_Printf ("ok\n");

	/*
	** try changing the display mode normally
	*/
	Com_Printf ("...finding display mode\n" );
	Com_Printf ("...setting linear mode: " );
	if ((ddrval = sww_state.DirectDraw->SetDisplayMode (vid.width, vid.height, 8)) == DD_OK)
	{
		Com_Printf ("ok\n");
	}
	/*
	** if no linear mode found, go for modex if we're trying 320x240
	*/
	else if ( (!sw_mode->integer) && sw_allow_modex->integer )
	{
		Com_WPrintf ("failed\n" );
		Com_Printf ("...attempting ModeX 320x240: ");

		/*
		** reset to normal cooperative level
		*/
		sww_state.DirectDraw->SetCooperativeLevel( sww_state.hWnd, DDSCL_NORMAL );

		/*
		** set exclusive mode
		*/
		if ( ( ddrval = sww_state.DirectDraw->SetCooperativeLevel(sww_state.hWnd,
			DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN|DDSCL_NOWINDOWCHANGES|DDSCL_ALLOWMODEX ) ) != DD_OK )
		{
			Com_WPrintf ("failed SCL - %s\n",DDrawError (ddrval));
			goto fail;
		}

		/*
		** change our display mode
		*/
		if ( ( ddrval = sww_state.DirectDraw->SetDisplayMode(vid.width, vid.height, 8 ) ) != DD_OK )
		{
			Com_WPrintf ("failed SDM - %s\n", DDrawError( ddrval ));
			goto fail;
		}
		Com_Printf ("ok\n");

		sww_state.modex = true;
	}
	else
	{
		Com_WPrintf ("failed\n");
		goto fail;
	}

	/*
	** create our front buffer
	*/
	memset( &ddsd, 0, sizeof( ddsd ) );
	ddsd.dwSize = sizeof( ddsd );
	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
	ddsd.dwBackBufferCount = 1;

	Com_Printf ("...creating front buffer: ");
	if ( ( ddrval = sww_state.DirectDraw->CreateSurface(&ddsd, &sww_state.ddsFrontBuffer, NULL ) ) != DD_OK )
	{
		Com_WPrintf ("failed - %s\n", DDrawError( ddrval ));
		goto fail;
	}
	Com_Printf ("ok\n");

	/*
	** see if we're a ModeX mode
	*/
	sww_state.ddsFrontBuffer->GetCaps (&ddscaps);
	if ( ddscaps.dwCaps & DDSCAPS_MODEX )
		Com_Printf ("...using ModeX\n");

	/*
	** create our back buffer
	*/
	ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;

	Com_Printf ("...creating back buffer: ");
	if ( ( ddrval = sww_state.ddsFrontBuffer->GetAttachedSurface( &ddsd.ddsCaps, &sww_state.ddsBackBuffer ) ) != DD_OK )
	{
		Com_WPrintf ("failed - %s\n", DDrawError( ddrval ));
		goto fail;
	}
	Com_Printf ("ok\n");

	/*
	** create our rendering buffer
	*/
	memset( &ddsd, 0, sizeof( ddsd ) );
	ddsd.dwSize = sizeof( ddsd );
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	ddsd.dwHeight = vid.height;
	ddsd.dwWidth = vid.width;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

	Com_Printf ("...creating offscreen buffer: ");
	if ( ( ddrval = sww_state.DirectDraw->CreateSurface( &ddsd, &sww_state.ddsOffScreenBuffer, NULL ) ) != DD_OK )
	{
		Com_WPrintf ("failed - %s\n", DDrawError( ddrval ));
		goto fail;
	}
	Com_Printf ("ok\n");

	/*
	** create our DIRECTDRAWPALETTE
	*/
	Com_Printf ("...creating palette: ");
	if ( ( ddrval = sww_state.DirectDraw->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256, palentries, &sww_state.ddpPalette, NULL ) ) != DD_OK )
	{
		Com_WPrintf ("failed - %s\n", DDrawError( ddrval ));
		goto fail;
	}
	Com_Printf ("ok\n");

	Com_Printf ("...setting palette: ");
	if ( ( ddrval = sww_state.ddsFrontBuffer->SetPalette (sww_state.ddpPalette) ) != DD_OK )
	{
		Com_WPrintf ("failed - %s\n", DDrawError( ddrval ));
		goto fail;
	}
	Com_Printf ("ok\n");

	DDRAW_SetPalette( ( const unsigned char * ) sw_state.currentpalette );

	/*
	** lock the back buffer
	*/
	memset( &ddsd, 0, sizeof( ddsd ) );
	ddsd.dwSize = sizeof( ddsd );

	Com_Printf ("...locking backbuffer: ");
	if ( ( ddrval = sww_state.ddsOffScreenBuffer->Lock( NULL, &ddsd, DDLOCK_WAIT, NULL ) ) != DD_OK )
	{
		Com_WPrintf ("failed - %s\n", DDrawError( ddrval ));
		goto fail;
	}
	Com_Printf ("ok\n");

	*ppbuffer = (byte*)ddsd.lpSurface;
	*ppitch   = ddsd.lPitch;

	for ( i = 0; i < vid.height; i++ )
	{
		memset( *ppbuffer + i * *ppitch, 0, *ppitch );
	}

	sww_state.palettized = true;

	return true;
fail:
	Com_WPrintf ("*** DDraw init failure ***\n");

	DDRAW_Shutdown();
	return false;
}

/*
** DDRAW_SetPalette
**
** Sets the color table in our DIB section, and also sets the system palette
** into an identity mode if we're running in an 8-bit palettized display mode.
**
** The palette is expected to be 1024 bytes, in the format:
**
** R = offset 0
** G = offset 1
** B = offset 2
** A = offset 3
*/
void DDRAW_SetPalette( const unsigned char *pal )
{
	PALETTEENTRY palentries[256];
	int i;

	if (!sww_state.ddpPalette) return;

	for (i = 0; i < 256; i++, pal += 4)
	{
		palentries[i].peRed   = pal[0];
		palentries[i].peGreen = pal[1];
		palentries[i].peBlue  = pal[2];
		palentries[i].peFlags = PC_RESERVED | PC_NOCOLLAPSE;
	}

	if (sww_state.ddpPalette->SetEntries( 0, 0, 256, palentries ) != DD_OK )
	{
		Com_WPrintf ("DDRAW_SetPalette() - SetEntries failed\n");
	}
}

/*
** DDRAW_Shutdown
*/
void DDRAW_Shutdown( void )
{
	if (sww_state.ddsOffScreenBuffer)
	{
		Com_Printf ("...releasing offscreen buffer\n");
		sww_state.ddsOffScreenBuffer->Unlock (vid.buffer);
		sww_state.ddsOffScreenBuffer->Release ();
		sww_state.ddsOffScreenBuffer = NULL;
	}

	if (sww_state.ddsBackBuffer)
	{
		Com_Printf ("...releasing back buffer\n");
		sww_state.ddsBackBuffer->Release ();
		sww_state.ddsBackBuffer = NULL;
	}

	if (sww_state.ddsFrontBuffer)
	{
		Com_Printf ("...releasing front buffer\n");
		sww_state.ddsFrontBuffer->Release ();
		sww_state.ddsFrontBuffer = NULL;
	}

	if (sww_state.ddpPalette)
	{
		Com_Printf ("...releasing palette\n");
		sww_state.ddpPalette->Release ();
		sww_state.ddpPalette = NULL;
	}

	if ( sww_state.DirectDraw )
	{
		Com_Printf ("...restoring display mode\n");
		sww_state.DirectDraw->RestoreDisplayMode ();
		Com_Printf ("...restoring normal coop mode\n");
	    sww_state.DirectDraw->SetCooperativeLevel (sww_state.hWnd, DDSCL_NORMAL);
		Com_Printf ("...releasing lpDirectDraw\n");
		sww_state.DirectDraw->Release ();
		sww_state.DirectDraw = NULL;
	}

	if ( sww_state.hinstDDRAW )
	{
		Com_Printf ("...freeing library\n");
		FreeLibrary( sww_state.hinstDDRAW );
		sww_state.hinstDDRAW = NULL;
	}
}

static const char *DDrawError (int code)
{
	static struct {
		int		code;
		char	*str;
	} info[] = {
		// list DirectDraw error consts
#define T(str)	{DDERR_##str, #str}
		T(ALREADYINITIALIZED), T(BLTFASTCANTCLIP), T(CANNOTATTACHSURFACE), T(CANNOTDETACHSURFACE),
		T(CANTCREATEDC), T(CANTDUPLICATE), T(CLIPPERISUSINGHWND), T(COLORKEYNOTSET), T(CURRENTLYNOTAVAIL),
		T(DIRECTDRAWALREADYCREATED), T(EXCEPTION), T(EXCLUSIVEMODEALREADYSET), T(GENERIC), T(HEIGHTALIGN),
		T(HWNDALREADYSET), T(HWNDSUBCLASSED), T(IMPLICITLYCREATED), T(INCOMPATIBLEPRIMARY), T(INVALIDCAPS),
		T(INVALIDCLIPLIST), T(INVALIDDIRECTDRAWGUID), T(INVALIDMODE), T(INVALIDOBJECT), T(INVALIDPARAMS),
		T(INVALIDPIXELFORMAT), T(INVALIDPOSITION), T(INVALIDRECT), T(LOCKEDSURFACES), T(NO3D), T(NOALPHAHW),
		T(NOBLTHW), T(NOCLIPLIST), T(NOCLIPPERATTACHED), T(NOCOLORCONVHW), T(NOCOLORKEY), T(NOCOLORKEYHW),
		T(NOCOOPERATIVELEVELSET), T(NODC), T(NODDROPSHW), T(NODIRECTDRAWHW), T(NOEMULATION),
		T(NOEXCLUSIVEMODE), T(NOFLIPHW), T(NOGDI), T(NOHWND), T(NOMIRRORHW), T(NOOVERLAYDEST), T(NOOVERLAYHW),
		T(NOPALETTEATTACHED), T(NOPALETTEHW), T(NORASTEROPHW), T(NOROTATIONHW), T(NOSTRETCHHW), T(NOT4BITCOLOR),
		T(NOT4BITCOLORINDEX), T(NOT8BITCOLOR), T(NOTAOVERLAYSURFACE), T(NOTEXTUREHW), T(NOTFLIPPABLE),
		T(NOTFOUND), T(NOTLOCKED), T(NOTPALETTIZED), T(NOVSYNCHW), T(NOZBUFFERHW), T(NOZOVERLAYHW), T(OUTOFCAPS),
		T(OUTOFMEMORY), T(OUTOFVIDEOMEMORY), T(OVERLAYCANTCLIP), T(OVERLAYCOLORKEYONLYONEACTIVE),
		T(OVERLAYNOTVISIBLE), T(PALETTEBUSY), T(PRIMARYSURFACEALREADYEXISTS), T(REGIONTOOSMALL),
		T(SURFACEALREADYATTACHED), T(SURFACEALREADYDEPENDENT), T(SURFACEBUSY), T(SURFACEISOBSCURED),
		T(SURFACELOST), T(SURFACENOTATTACHED), T(TOOBIGHEIGHT), T(TOOBIGSIZE), T(TOOBIGWIDTH), T(UNSUPPORTED),
		T(UNSUPPORTEDFORMAT), T(UNSUPPORTEDMASK), T(VERTICALBLANKINPROGRESS), T(WASSTILLDRAWING),
		T(WRONGMODE), T(XALIGN)
	};

	if (code == DD_OK)
		return "DD_OK";
	else
	{
		char	*s;
		int		i;

		s = NULL;
		for (i = 0; i < ARRAY_COUNT(info); i++)
			if (info[i].code == code)
			{
				s = info[i].str;
				break;
			}
		if (s)
			return va("DDERR_%s", s);
		else
			return va("DDERR_UNK_%d", code);
	}
}
