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
// screen.h

void	SCR_Init (void);

void	SCR_UpdateScreen (void);

void	SCR_CenterPrint (char *str);

//void	SCR_BeginLoadingPlaque (void); -- declared in qcommon.h
void	SCR_EndLoadingPlaque (bool force);	//?? is "force" needed ?

void	SCR_DebugGraph (float value, int color);

void	SCR_TouchPics (void);

// here for menu:
extern	cvar_t	*crosshair;
extern	cvar_t	*crosshairColor;

typedef struct vrect_s
{
	int		x, y, width, height;
} vrect_t;

extern	vrect_t		scr_vrect;		// position of render window

void DrawString (int x, int y, const char *s);	// draw colorized string; used by menus/console
void SCR_SetLevelshot (char *name = NULL);		// give levelshot name (if name not specified - compute from map name)
void SCR_ShowConsole (bool show, bool noAnim);
void SCR_ToggleConsole (void);
void Key_Message (int key);

//
// scr_cin.c
//
void SCR_PlayCinematic (char *name);
bool SCR_DrawCinematic (void);
void SCR_RunCinematic (void);
void SCR_StopCinematic (void);
void SCR_FinishCinematic (void);
