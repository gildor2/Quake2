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
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

/*

  full screen console
  put up loading plaque
  blanked background with loading plaque
  blanked background with menu
  cinematics
  full screen image for quit and victory

  end of unit intermissions

  */

#include "client.h"

float		scr_con_current;	// aproaches scr_conlines
float		scr_conlines;		// 0.0 to 1.0 lines of console to display

qboolean	scr_initialized;	// ready to draw

int			scr_draw_loading;
char		*scr_map_levelshot;

vrect_t		scr_vrect;			// position of render window on screen


cvar_t		*scr_viewsize;
cvar_t		*scr_centertime;

cvar_t		*scr_netgraph;
cvar_t		*scr_timegraph;
cvar_t		*scr_debuggraph;
cvar_t		*scr_graphheight;
cvar_t		*scr_graphscale;
cvar_t		*scr_graphshift;


char		crosshair_pic[MAX_QPATH];
int			crosshair_width, crosshair_height;

void SCR_TimeRefresh_f (void);
void SCR_Loading_f (void);


/*
===============================================================================

BAR GRAPHS

===============================================================================
*/

/*
==============
CL_AddNetgraph

A new packet was just parsed
==============
*/
void CL_AddNetgraph (void)
{
	int		i;
	int		in;
	int		ping;

	// if using the debuggraph for something else, don't
	// add the net lines
	if (scr_debuggraph->integer || scr_timegraph->integer)
		return;

	for (i=0 ; i<cls.netchan.dropped ; i++)
		SCR_DebugGraph (30, 0x40);

	for (i=0 ; i<cl.surpressCount ; i++)
		SCR_DebugGraph (30, 0xdf);

	// see what the latency was on this packet
	in = cls.netchan.incoming_acknowledged & (CMD_BACKUP-1);
	ping = cls.realtime - cl.cmd_time[in];
	ping /= 30;
	if (ping > 30)
		ping = 30;
	SCR_DebugGraph (ping, 0xd0);
}


typedef struct
{
	float	value;
	int		color;
} graphsamp_t;

static	int			current;
static	graphsamp_t	values[1024];

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph (float value, int color)
{
	values[current&1023].value = value;
	values[current&1023].color = color;
	current++;
}

/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph (void)
{
	int		a, x, y, w, i, h;
	float	v;
	int		color;

	//
	// draw the graph
	//
	w = scr_vrect.width;

	x = scr_vrect.x;
	y = scr_vrect.y+scr_vrect.height;
	re.DrawFill (x, y-scr_graphheight->integer,
		w, scr_graphheight->integer, 8);

	for (a=0 ; a<w ; a++)
	{
		i = (current-1-a+1024) & 1023;
		v = values[i].value;
		color = values[i].color;
		v = v*scr_graphscale->value + scr_graphshift->value;

		if (v < 0)
			v += scr_graphheight->integer * (1+Q_round(-v/scr_graphheight->value));
		h = Q_round(v) % scr_graphheight->integer;
		re.DrawFill (x+w-1-a, y - h, 1,	h, color);
	}
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

static char		scr_centerstring[1024];
static float	scr_centertime_off;
static int		scr_center_lines;
static int		scr_erase_center;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (char *str)
{
	char	*s;
	char	line[64];
	int		i, j, l;

	Q_strncpyz (scr_centerstring, str, sizeof(scr_centerstring));
	scr_centertime_off = scr_centertime->value;

	// count the number of lines for centering
	scr_center_lines = 1;
	s = str;
	while (*s)
	{
		if (*s == '\n')
			scr_center_lines++;
		s++;
	}

	// echo it to the console
	Com_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");

	s = str;
	do
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (s[l] == '\n' || !s[l])
				break;
		for (i=0 ; i<(40-l)/2 ; i++)
			line[i] = ' ';

		for (j=0 ; j<l ; j++)
		{
			line[i++] = s[j];
		}

		line[i] = '\n';
		line[i+1] = 0;

		Com_Printf ("%s", line);

		while (*s && *s != '\n')
			s++;

		if (!*s)
			break;
		s++;		// skip the \n
	} while (1);
	Com_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_ClearNotify ();
}


void SCR_DrawCenterString (void)
{
	char	*start;
	int		l;
	int		j;
	int		x, y;
	int		remaining;

	// the finale prints the characters one at a time
	remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if (scr_center_lines <= 4)
		y = Q_round (viddef.height * 0.35f);
	else
		y = 48;

	do
	{
		// scan the width of the line
		for (l = 0; l < 40; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (viddef.width - l*8)/2;
		for (j = 0; j < l; j++, x+=8)
		{
			re_DrawChar (x, y, start[j]);
			if (!remaining--)
				return;
		}

		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

void SCR_CheckDrawCenterString (void)
{
	scr_centertime_off -= cls.frametime;

	if (scr_centertime_off <= 0)
		return;

	SCR_DrawCenterString ();
}

//=============================================================================

/*
=================
SCR_CalcVrect

Sets scr_vrect, the coordinates of the rendered window
=================
*/
static void SCR_CalcVrect (void)
{
	float	frac;

	frac = Cvar_Clamp (scr_viewsize, 40, 100) / 100.0f;

	scr_vrect.width = Q_round (viddef.width * frac);
	scr_vrect.height = Q_round (viddef.height * frac);

	scr_vrect.width &= ~7;		// align(8)
	scr_vrect.height &= ~1;		// align(2)

	scr_vrect.x = (viddef.width - scr_vrect.width) / 2;
	scr_vrect.y = (viddef.height - scr_vrect.height) / 2;
}


/*
=================
SCR_Sky_f

Set a specific sky and rotation speed
=================
*/
void SCR_Sky_f (void)
{
	float	rotate;
	vec3_t	axis;

	if (Cmd_Argc() < 2)
	{
		Com_Printf ("Usage: sky <basename> <rotate> <axis x y z>\n");
		return;
	}
	if (Cmd_Argc() > 2)
		rotate = atof(Cmd_Argv(2));
	else
		rotate = 0;
	if (Cmd_Argc() == 6)
	{
		axis[0] = atof(Cmd_Argv(3));
		axis[1] = atof(Cmd_Argv(4));
		axis[2] = atof(Cmd_Argv(5));
	}
	else
	{
		axis[0] = 0;
		axis[1] = 0;
		axis[2] = 1;
	}

	re.SetSky (Cmd_Argv(1), rotate, axis);
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
CVAR_BEGIN(vars)
	CVAR_VAR(scr_viewsize, 100, CVAR_ARCHIVE),
	CVAR_VAR(scr_centertime, 2.5, 0),
	{&scr_netgraph, "netgraph", "0", 0},
	{&scr_timegraph, "timegraph", "0", 0},
	{&scr_debuggraph, "debuggraph", "0", 0},
	{&scr_graphheight, "graphheight", "32", 0},
	{&scr_graphscale, "graphscale", "1", 0},
	{&scr_graphshift, "graphshift", "0", 0},
CVAR_END

	Cvar_GetVars (ARRAY_ARG(vars));

	Cmd_AddCommand ("timerefresh",SCR_TimeRefresh_f);
	Cmd_AddCommand ("loading",SCR_Loading_f);
	Cmd_AddCommand ("sky",SCR_Sky_f);

	scr_initialized = true;
}


/*
==============
SCR_DrawPause
==============
*/
void SCR_DrawPause (void)
{
	int		w, h;

	if (!cl_paused->integer)
		return;

	re.DrawGetPicSize (&w, &h, "pause");
	re_DrawPic ((viddef.width-w)/2, viddef.height/2 + 8, "pause");
}

//=============================================================================

/*
==================
SCR_RunConsole

Scroll it up or down
==================
*/
void SCR_RunConsole (void)
{
	static int lastConTime = 0;
	int		currTime, timeDelta;

	// decide on the height of the console
	if (*re.flags & REF_CONSOLE_ONLY)
	{
		cls.key_dest = key_console;
		scr_conlines = 1.0;
		return;
	}

	if (cls.key_dest == key_console || cls.keep_console)
		scr_conlines = 0.8;
//		scr_conlines = 0.5;		// half screen
	else
		scr_conlines = 0;		// none visible

	currTime = Sys_Milliseconds ();
	if (lastConTime)
		timeDelta = currTime - lastConTime;
	else
		timeDelta = 0;
	lastConTime = currTime;

	if (scr_conlines < scr_con_current)
	{
		scr_con_current -= 3 * timeDelta / 1000.0f;
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;

	}
	else if (scr_conlines > scr_con_current)
	{
		scr_con_current += 3 * timeDelta / 1000.0f;
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}
	else		// console in-place
		lastConTime = 0;
}

/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole (void)
{
	Con_CheckResize ();

	if (*re.flags & REF_CONSOLE_ONLY || cls.state == ca_disconnected || cls.state == ca_connecting)
	{	// forced full screen console
		Con_DrawConsole (1.0);
		return;
	}

	if (cls.state != ca_active || !cl.refresh_prepped)
	{	// connected, but can't render
		re.DrawFill (0, 0, viddef.width, viddef.height, 0);
//		re.DrawFill (0, viddef.height/2, viddef.width, viddef.height/2, 0);
		Con_DrawConsole (0.5);
		return;
	}

	if (scr_con_current)
	{
		Con_DrawConsole (scr_con_current);
	}
	else
	{
		if (cls.key_dest == key_game || cls.key_dest == key_message)
			Con_DrawNotify (true);	// only draw notify in game
	}
}

//=============================================================================

static int loadingScrEnabled;

void SCR_SetLevelshot (char *name)
{
	static char levelshot[MAX_OSPATH];
	int		width;

	if (scr_map_levelshot)
		return;			// already set

	strcpy (levelshot, name);
	re.ReloadImage (levelshot);		// force renderer to refresh image
	re.DrawGetPicSize (&width, NULL, levelshot);
	if (width > 0)
	{
		scr_map_levelshot = levelshot;
		scr_draw_loading = 1;
		Com_DPrintf ("SetLevelshot: %s\n", levelshot);
		loadingScrEnabled = 20;		// allow only 20 frames with "ca_disconnected" state
	}
	else
		scr_map_levelshot = NULL;
}


/*
==============
SCR_DrawLoading
==============
*/


void SCR_DrawLoading (void)
{
	int		w, h;

	if (!scr_draw_loading)
		return;

	if (cls.state == ca_disconnected || cls.state == ca_active || cls.key_dest == key_menu)
	{
//Com_Printf("draw(disc): %d, %d\n", scr_draw_loading, loadingScrEnabled);//!!
		if (!loadingScrEnabled)
		{
			scr_draw_loading = 0;
			return;
		}
		else
			loadingScrEnabled--;
	}
//Com_Printf("draw(conn): %d %d %d\n", cls.state == ca_disconnected, cls.state == ca_active, cls.key_dest == key_menu);//!!

	if (scr_map_levelshot)
	{
		re.DrawStretchPic (0, 0, viddef.width, viddef.height, scr_map_levelshot);
		//!! DrawLevelshotDetail for GL
		Con_DrawNotify (false);
	}
	else //if (scr_draw_loading == 2)
	{
		re.DrawGetPicSize (&w, &h, "loading");
		re_DrawPic ((viddef.width - w) / 2, (viddef.height - h) / 2, "loading");
	}
}

/*
================
SCR_BeginLoadingPlaque
================
*/
void SCR_BeginLoadingPlaque (void)
{
	S_StopAllSounds ();
	cl.sound_prepped = false;		// don't play ambients
	CDAudio_Stop ();
	if (cls.disable_screen)
		return;
	if (developer->integer)
		return;
//	if (cls.state == ca_disconnected)
//		return;	// if at console, don't bring up the plaque
//	if (cls.key_dest == key_console)
//		return;
	if (cl.cinematictime > 0)
		scr_draw_loading = 2;	// clear to black first
	else
		scr_draw_loading = 1;
	SCR_UpdateScreen ();
	cls.disable_screen = 0;//Sys_Milliseconds ();
	cls.disable_servercount = cl.servercount;
	loadingScrEnabled = 20;		// allow only 20 frames with "ca_disconnected" state
}

/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque (void)
{
	scr_draw_loading = 0;
	loadingScrEnabled = 0;
	scr_map_levelshot = NULL;
	cls.disable_screen = 0;
	Con_ClearNotify ();
}

/*
================
SCR_Loading_f
================
*/
void SCR_Loading_f (void)
{
	SCR_BeginLoadingPlaque ();
}

/*
================
SCR_TimeRefresh_f
================
*/
void SCR_TimeRefresh_f (void)
{
	int		i;
	int		start, stop;
	float	time;

	if ( cls.state != ca_active )
		return;

	start = Sys_Milliseconds ();

	if (Cmd_Argc() == 2)
	{	// run without page flipping
		re.BeginFrame( 0 );
		for (i=0 ; i<128 ; i++)
		{
			cl.refdef.viewangles[1] = i/128.0*360.0;
			re.RenderFrame (&cl.refdef);
		}
		re.EndFrame();
	}
	else
	{
		for (i=0 ; i<128 ; i++)
		{
			cl.refdef.viewangles[1] = i/128.0*360.0;

			re.BeginFrame( 0 );
			re.RenderFrame (&cl.refdef);
			re.EndFrame();
		}
	}

	stop = Sys_Milliseconds ();
	time = (stop-start)/1000.0;
	Com_Printf ("%f seconds (%f fps)\n", time, 128/time);
}


/*
==============
SCR_TileClear

Clear any parts of the tiled background that were drawn on last frame
==============
*/
void SCR_TileClear (void)
{
	int		x1, x2, y1, y2;

	if (scr_con_current == 1.0f || scr_viewsize->integer == 100 || cl.cinematictime > 0) return;

	y1 = scr_vrect.y;
	y2 = y1 + scr_vrect.height;
	re.DrawTileClear (0, 0, viddef.width, y1, "backtile");
	re.DrawTileClear (0, y2, viddef.width, viddef.height - y2, "backtile");
	x1 = scr_vrect.x;
	x2 = x1 + scr_vrect.width;
	re.DrawTileClear (0, y1, x1, scr_vrect.height, "backtile");
	re.DrawTileClear (x2, y1, viddef.width - x2, scr_vrect.height, "backtile");

	return;
}


//===============================================================


#define STAT_MINUS		10	// num frame for '-' stats digit
static char *sb_nums[2][11] =
{
	{"num_0", "num_1", "num_2", "num_3", "num_4", "num_5", "num_6", "num_7", "num_8", "num_9", "num_minus"},
	{"anum_0", "anum_1", "anum_2", "anum_3", "anum_4", "anum_5", "anum_6", "anum_7", "anum_8", "anum_9", "anum_minus"}
};

#define	ICON_WIDTH	24
#define	ICON_HEIGHT	24
#define	CHAR_WIDTH	16
#define	ICON_SPACE	8



/*
================
SizeHUDString

Allow embedded \n in the string
================
*/
void SizeHUDString (char *string, int *w, int *h)
{
	int		lines, width, current;

	lines = 1;
	width = 0;

	current = 0;
	while (*string)
	{
		if (*string == '\n')
		{
			lines++;
			current = 0;
		}
		else
		{
			current++;
			if (current > width)
				width = current;
		}
		string++;
	}

	*w = width * 8;
	*h = lines * 8;
}

void DrawHUDString (char *string, int x, int y, int centerwidth, int xor)
{
	int		margin;
	char	line[1024];
	int		width;
	int		i;

	margin = x;

	while (*string)
	{
		// scan out one line of text from the string
		width = 0;
		while (*string && *string != '\n')
			line[width++] = *string++;
		line[width] = 0;

		if (centerwidth)
			x = margin + (centerwidth - width*8)/2;
		else
			x = margin;
		for (i=0 ; i<width ; i++)
		{
			re_DrawChar (x, y, line[i]^xor);
			x += 8;
		}
		if (*string)
		{
			string++;	// skip the \n
			x = margin;
			y += 8;
		}
	}
}


/*
==============
SCR_DrawField
==============
*/
void SCR_DrawField (int x, int y, int color, int width, int value)
{
	char	num[16], *ptr;
	int		l;
	int		frame;

	if (width < 1)
		return;

	// draw number string
	if (width > 5)
		width = 5;

	Com_sprintf (ARRAY_ARG(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + CHAR_WIDTH*(width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		re_DrawPic (x, y, sb_nums[color][frame]);
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}


/*
===============
SCR_TouchPics

Allows rendering code to cache all needed sbar graphics
===============
*/
void SCR_TouchPics (void)
{
	int		i, j, ch_num;

	if (*re.flags & REF_CONSOLE_ONLY)
		return;

	for (i=0 ; i<2 ; i++)
		for (j=0 ; j<11 ; j++)
			re.RegisterPic (sb_nums[i][j]);		// can remember image handles and use later (faster drawing, but need API extension ??)

	ch_num = crosshair->integer;
	if (ch_num)
	{
		if (ch_num > 0)
		{
			Com_sprintf (ARRAY_ARG(crosshair_pic), "ch%d", crosshair->integer);
			re.DrawGetPicSize (&crosshair_width, &crosshair_height, crosshair_pic);
			if (crosshair_width <= 0)
				ch_num = -1;								// invalid value
		}
		if (ch_num <= 0)
		{
			crosshair_pic[0] = 0;
			if (ch_num < 0) Cvar_Set ("crosshair", "0");	// invalid value becomes zero
		}
	}
}

/*
================
SCR_ExecuteLayoutString

================
*/
void SCR_ExecuteLayoutString (char *s)
{
	int		x, y;
	int		value;
	char	*token;
	int		width;
	int		index;
	clientinfo_t	*ci;

	if (cls.state != ca_active || !cl.refresh_prepped)
		return;

	if (!s[0]) return;

	x = y = 0;
	width = 3;

	while (s)
	{
		token = COM_Parse (&s);
		if (!strcmp(token, "xl"))
		{
			token = COM_Parse (&s);
			x = atoi(token);
			continue;
		}
		if (!strcmp(token, "xr"))
		{
			token = COM_Parse (&s);
			x = viddef.width + atoi(token);
			continue;
		}
		if (!strcmp(token, "xv"))
		{
			token = COM_Parse (&s);
			x = viddef.width/2 - 160 + atoi(token);
			continue;
		}

		if (!strcmp(token, "yt"))
		{
			token = COM_Parse (&s);
			y = atoi(token);
			continue;
		}
		if (!strcmp(token, "yb"))
		{
			token = COM_Parse (&s);
			y = viddef.height + atoi(token);
			continue;
		}
		if (!strcmp(token, "yv"))
		{
			token = COM_Parse (&s);
			y = viddef.height/2 - 120 + atoi(token);
			continue;
		}

		if (!strcmp(token, "pic"))
		{	// draw a pic from a stat number
			token = COM_Parse (&s);
			value = cl.frame.playerstate.stats[atoi(token)];
			if (value >= MAX_IMAGES)
				Com_Error (ERR_DROP, "Pic >= MAX_IMAGES");
			if (cl.configstrings[CS_IMAGES+value])
				re_DrawPic (x, y, cl.configstrings[CS_IMAGES+value]);

			continue;
		}

		if (!strcmp(token, "client"))
		{	// draw a deathmatch client block
			int		score, ping, time;

			token = COM_Parse (&s);
			x = viddef.width/2 - 160 + atoi(token);
			token = COM_Parse (&s);
			y = viddef.height/2 - 120 + atoi(token);

			token = COM_Parse (&s);
			value = atoi(token);
			if (value >= MAX_CLIENTS || value < 0)
				Com_Error (ERR_DROP, "client >= MAX_CLIENTS");
			ci = &cl.clientinfo[value];

			token = COM_Parse (&s);
			score = atoi(token);

			token = COM_Parse (&s);
			ping = atoi(token);

			token = COM_Parse (&s);
			time = atoi(token);

			DrawString (x+32, y, va("^2%s", ci->name));
			DrawString (x+32, y+8,  "Score: ");
			DrawString (x+32+7*8, y+8,  va("^2%i", score));
			DrawString (x+32, y+16, va("Ping:  %i", ping));
			DrawString (x+32, y+24, va("Time:  %i", time));

			if (!ci->icon)
				ci = &cl.baseclientinfo;
			re_DrawPic (x, y, ci->iconname);
			continue;
		}

		if (!strcmp(token, "ctf"))
		{	// draw a ctf client block
			int		score, ping;
			char	block[80];

			token = COM_Parse (&s);
			x = viddef.width/2 - 160 + atoi(token);
			token = COM_Parse (&s);
			y = viddef.height/2 - 120 + atoi(token);

			token = COM_Parse (&s);
			value = atoi(token);
			if (value >= MAX_CLIENTS || value < 0)
				Com_Error (ERR_DROP, "client >= MAX_CLIENTS");
			ci = &cl.clientinfo[value];

			token = COM_Parse (&s);
			score = atoi(token);

			token = COM_Parse (&s);
			ping = atoi(token);
			if (ping > 999)
				ping = 999;

			Com_sprintf (ARRAY_ARG(block), "%3d %3d %-12.12s", score, ping, ci->name);

			if (value == cl.playernum)
				DrawString (x, y, va("^1%s", block));
			else
				DrawString (x, y, block);
			continue;
		}

		if (!strcmp(token, "picn"))
		{	// draw a pic from a name
			token = COM_Parse (&s);
			re_DrawPic (x, y, token);
			continue;
		}

		if (!strcmp(token, "num"))
		{	// draw a number
			token = COM_Parse (&s);
			width = atoi(token);
			token = COM_Parse (&s);
			value = cl.frame.playerstate.stats[atoi(token)];
			SCR_DrawField (x, y, 0, width, value);
			continue;
		}

		if (!strcmp(token, "hnum"))
		{	// health number
			int		color;

			width = 3;
			value = cl.frame.playerstate.stats[STAT_HEALTH];
			if (value > 25)
				color = 0;	// green
			else if (value > 0)
				color = (cl.frame.serverframe>>2) & 1;		// flash
			else
				color = 1;

			if (cl.frame.playerstate.stats[STAT_FLASHES] & 1)
				re_DrawPic (x, y, "field_3");

			SCR_DrawField (x, y, color, width, value);
			continue;
		}

		if (!strcmp(token, "anum"))
		{	// ammo number
			int		color;

			width = 3;
			value = cl.frame.playerstate.stats[STAT_AMMO];
			if (value > 5)
				color = 0;	// green
			else if (value >= 0)
				color = (cl.frame.serverframe>>2) & 1;		// flash
			else
				continue;	// negative number = don't show

			if (cl.frame.playerstate.stats[STAT_FLASHES] & 4)
				re_DrawPic (x, y, "field_3");

			SCR_DrawField (x, y, color, width, value);
			continue;
		}

		if (!strcmp(token, "rnum"))
		{	// armor number
			int		color;

			width = 3;
			value = cl.frame.playerstate.stats[STAT_ARMOR];
			if (value < 1)
				continue;

			color = 0;	// green

			if (cl.frame.playerstate.stats[STAT_FLASHES] & 2)
				re_DrawPic (x, y, "field_3");

			SCR_DrawField (x, y, color, width, value);
			continue;
		}


		if (!strcmp(token, "stat_string"))
		{
			token = COM_Parse (&s);
			index = atoi(token);
			if (index < 0 || index >= MAX_CONFIGSTRINGS)
				Com_Error (ERR_DROP, "Bad stat_string index");
			index = cl.frame.playerstate.stats[index];
			if (index < 0 || index >= MAX_CONFIGSTRINGS)
				Com_Error (ERR_DROP, "Bad stat_string index");
			DrawString (x, y, cl.configstrings[index]);
			continue;
		}

		if (!strcmp(token, "cstring"))
		{
			token = COM_Parse (&s);
			DrawHUDString (token, x, y, 320, 0);
			continue;
		}

		if (!strcmp(token, "cstring2"))
		{
			token = COM_Parse (&s);
			DrawHUDString (token, x, y, 320,0x80);
			continue;
		}

		if (!strcmp(token, "string"))
		{
			token = COM_Parse (&s);
			DrawString (x, y, token);
			continue;
		}

		if (!strcmp(token, "string2"))
		{
			token = COM_Parse (&s);
			DrawString (x, y, va("^2%s", token));
			continue;
		}

		if (!strcmp(token, "if"))
		{	// draw a number
			token = COM_Parse (&s);
			value = cl.frame.playerstate.stats[atoi(token)];
			if (!value)
			{	// skip to endif
				while (s && strcmp(token, "endif") )
				{
					token = COM_Parse (&s);
				}
			}
			continue;
		}
	}
}


/*
================
SCR_DrawStats

The status bar is a small layout program that
is based on the stats array
================
*/
void SCR_DrawStats (void)
{
	SCR_ExecuteLayoutString (cl.configstrings[CS_STATUSBAR]);
}


/*
================
SCR_DrawLayout

================
*/
void SCR_DrawLayout (void)
{
	if (!cl.frame.playerstate.stats[STAT_LAYOUTS])
		return;
	SCR_ExecuteLayoutString (cl.layout);
}

//=======================================================

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen (void)
{
	int numframes;
	int i;
	float separation[2] = { 0, 0 };

	// if the screen is disabled (loading plaque is up, or vid mode changing)
	// do nothing at all
	if (cls.disable_screen)
	{
		if (Sys_Milliseconds() - cls.disable_screen > 120000)
		{
			cls.disable_screen = 0;
			Com_Printf ("Loading plaque timed out.\n");
		}
		return;
	}

	if (!scr_initialized || !con_initialized)
		return;				// not initialized yet

	/*
	** range check cl_camera_separation so we don't inadvertently fry someone's
	** brain
	*/
	Cvar_Clamp (cl_stereo_separation, 0, 1);

	if ( cl_stereo->integer )
	{
		numframes = 2;
		separation[0] = -cl_stereo_separation->value / 2;
		separation[1] =  cl_stereo_separation->value / 2;
	}
	else
	{
		separation[0] = 0;
		separation[1] = 0;
		numframes = 1;
	}

	for ( i = 0; i < numframes; i++ )
	{
		re.BeginFrame( separation[i] );

		if (scr_draw_loading == 2)
		{	//  loading plaque over black screen
			re.SetRawPalette (NULL);
			SCR_DrawLoading ();
		}
		// if a cinematic is supposed to be running, handle menus
		// and console specially
		else if (cl.cinematictime > 0)
		{
			if (cls.key_dest == key_menu || cls.key_dest == key_bindingMenu)
			{
				if (cl.cinematicpalette_active)
				{
					re.SetRawPalette (NULL);
					cl.cinematicpalette_active = false;
				}
				M_Draw ();
			}
			else if (cls.key_dest == key_console)
			{
				if (cl.cinematicpalette_active)
				{
					re.SetRawPalette(NULL);
					cl.cinematicpalette_active = false;
				}
				SCR_DrawConsole ();
			}
			else
				SCR_DrawCinematic ();
		}
		else
		{

			// make sure the game palette is active
			if (cl.cinematicpalette_active)
			{
				re.SetRawPalette(NULL);
				cl.cinematicpalette_active = false;
			}

			// do 3D refresh drawing, and then update the screen
			SCR_CalcVrect ();
			// clear any dirty part of the background
			SCR_TileClear ();

			V_RenderView ( separation[i] );

			SCR_DrawStats ();
			if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 1)
				SCR_DrawLayout ();
			if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 2)
				CL_DrawInventory ();

			// show disconnected icon when server is not responding
			if (cl.overtime > 200)
				re_DrawPic (scr_vrect.x+64, scr_vrect.y, "net");

			SCR_CheckDrawCenterString ();

			if (scr_timegraph->integer)
				SCR_DebugGraph (cls.frametime*300, 0);

			if (scr_debuggraph->integer || scr_timegraph->integer || scr_netgraph->integer)
				SCR_DrawDebugGraph ();

			SCR_DrawPause ();
			SCR_DrawConsole ();		//!! sometimes console should be painted after menu!
			M_Draw ();
			SCR_DrawLoading ();
		}
	}
	re.EndFrame();
}
