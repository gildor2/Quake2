#include "client.h"

static bool initialized;

// cvars (non-static for menu)
cvar_t		*crosshair;
cvar_t		*crosshairColor;

// static cvars
static cvar_t	*con_maxSize;
static cvar_t	*scr_centertime;
static cvar_t	*cl_draw2d;

static cvar_t	*netgraph;
static cvar_t	*timegraph;
static cvar_t	*debuggraph;
static cvar_t	*graphheight;
static cvar_t	*graphscale;
static cvar_t	*graphshift;

#define CHAR_WIDTH	8
#define CHAR_HEIGHT	8

// Function DrawString used for painting HUD
void DrawString (int x, int y, const char *s)
{
	int color, c;

	color = C_WHITE;
	while (c = *s++)
	{
		if (c == COLOR_ESCAPE)
		{
			int col;

			col = *s - '0';
			if (col >= 0 && col <= 7)
			{
				color = col;
				s++;
				continue;
			}
		}
		re.DrawChar (x, y, c, color);
		x += CHAR_WIDTH;
	}
}


/*-----------------------------------------------------------------------------
	Graphs (remove, or replace ??)
-----------------------------------------------------------------------------*/

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
static void DrawDebugGraph (void)
{
	int w = min(viddef.width, 1024);
	re.DrawFill (0, viddef.height - graphheight->integer, w, graphheight->integer, 8);

	for (int a = 0; a < w; a++)
	{
		int i = (current-1-a+1024) & 1023;
		float v = values[i].value;
		int color = values[i].color;
		v = v * graphscale->value + graphshift->value;

		if (v < 0)
			v += graphheight->integer * (1 + appRound (-v / graphheight->value));
		int h = appRound(v) % graphheight->integer;
		re.DrawFill (w-1-a, viddef.height - h, 1, h, color);
	}
}


/*-----------------------------------------------------------------------------
	Center print
-----------------------------------------------------------------------------*/

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

	appStrncpyz (scr_centerstring, str, sizeof(scr_centerstring));
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
	Com_Printf("\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");

	s = str;
	do
	{
		// scan the width of the line
		for (l = 0; l < 40; l++)
			if (s[l] == '\n' || !s[l])
				break;
		for (i = 0; i < (40-l)/2; i++)
			line[i] = ' ';

		for (j = 0; j < l; j++)
			line[i++] = s[j];

		line[i] = '\n';
		line[i+1] = 0;

		Com_Printf ("%s", line);

		while (*s && *s != '\n')
			s++;

		if (!*s) break;
		s++;		// skip the \n
	} while (1);
	Com_Printf("\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n");
	Con_ClearNotify ();
}


static void DrawCenterString (void)
{
	scr_centertime_off -= cls.frametime;
	if (scr_centertime_off <= 0) return;

	scr_erase_center = 0;
	const char *start = scr_centerstring;

	int y = (scr_center_lines <= 4) ? appRound (viddef.height * 0.35f) : 48;

	while (true)
	{
		// scan the width of the line
		for (int l = 0; l < 40; l++)
			if (start[l] == '\n' || !start[l])
				break;
		int x = (viddef.width - l * CHAR_WIDTH) / 2;
		for (int j = 0; j < l; j++, x += CHAR_WIDTH)
			re.DrawChar (x, y, start[j]);

		y += CHAR_HEIGHT;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the '\n'
	}
}


/*-----------------------------------------------------------------------------
	Chat input
-----------------------------------------------------------------------------*/

static bool		chat_team;
static char		chat_buffer[MAXCMDLINE];
static int		chat_bufferlen = 0;


static void DrawChatInput (void)
{
	// edit current player message
	if (*re.flags & REF_CONSOLE_ONLY) return;
	if (cls.key_dest != key_message) return;

	int x;
	int v = CHAR_HEIGHT * 8;		//??
	if (chat_team)
	{
		DrawString (8, v, "say_team:");
		x = 11;
	}
	else
	{
		DrawString (8, v, "say:");
		x = 5;
	}

	const char *s = chat_buffer;
	if (chat_bufferlen > viddef.width/CHAR_WIDTH - (x + 1))
		s += chat_bufferlen - (viddef.width/CHAR_WIDTH - (x + 1));
	while (*s)
	{
		re.DrawChar (x * CHAR_WIDTH, v, *s++);
		x++;
	}
	// draw cursor
	re.DrawChar (x * CHAR_WIDTH, v, 10 + ((curtime >> 8) & 1));
}


void Key_Message (int key)
{
	if (key == K_ENTER || key == K_KP_ENTER)
	{
		if (chat_team)
			Cbuf_AddText ("say_team \"");
		else
			Cbuf_AddText ("say \"");
		Cbuf_AddText (chat_buffer);
		Cbuf_AddText ("\"\n");

		cls.key_dest = key_game;
		chat_bufferlen = 0;
		chat_buffer[0] = 0;
		return;
	}

	if (key == K_ESCAPE)
	{
		cls.key_dest = key_game;
		chat_bufferlen = 0;
		chat_buffer[0] = 0;
		return;
	}

	if (key == K_BACKSPACE)
	{
		if (chat_bufferlen)
		{
			chat_bufferlen--;
			chat_buffer[chat_bufferlen] = 0;
		}
		return;
	}

	if (key < 32 || key >= 128)
		return;		// non printable

	if (chat_bufferlen == sizeof(chat_buffer)-1)
		return;		// buffer full

	chat_buffer[chat_bufferlen++] = key;
	chat_buffer[chat_bufferlen] = 0;
}


static void SCR_MessageMode_f (void)
{
	chat_team = false;
	cls.key_dest = key_message;
}


static void SCR_MessageMode2_f (void)
{
	chat_team = true;
	cls.key_dest = key_message;
}


/*-----------------------------------------------------------------------------
	Console and loading plaque
-----------------------------------------------------------------------------*/

static const char *map_levelshot;

void SCR_SetLevelshot (char *name)
{
	static const char defLevelshot[] = "/pics/levelshot";

	if (!name)
	{
		char mapname[MAX_QPATH], *tmp;

		// get levelshot name from map name
		appCopyFilename (mapname, cl.configstrings[CS_MODELS+1], sizeof(mapname));
		tmp = strchr (mapname, '.');
		if (tmp) tmp[0] = 0;
		tmp = strrchr (mapname, '/');
		if (!tmp)
			tmp = tmp ? ++tmp : mapname;
		else
			tmp++;				// skip '/'
		name = va("/levelshots/%s", tmp);
	}

	if (map_levelshot && stricmp (map_levelshot, defLevelshot))
		return;						// already set non-default levelshot

	if (ImageExists (name))
	{
		static char levelshot[MAX_OSPATH];

		Com_DPrintf ("SetLevelshot: %s\n", name);
		strcpy (levelshot, name);
		re.ReloadImage (name);		// force renderer to refresh image
		map_levelshot = levelshot;
	}
	else if (ImageExists (defLevelshot))
	{
		if (map_levelshot != defLevelshot)
		{
			Com_DPrintf ("SetLevelshot: %s\n", defLevelshot);
			map_levelshot = defLevelshot;
		}
	}
	else
		map_levelshot = NULL;

	SCR_BeginLoadingPlaque ();
}


/*
================
SCR_BeginLoadingPlaque
================
*/
void SCR_BeginLoadingPlaque (void)
{
	guard(SCR_BeginLoadingPlaque);
	S_StopAllSounds_f ();
	cl.sound_prepped = false;		// don't play ambients
	CDAudio_Stop ();
	cls.loading = true;
	cls.disable_servercount = cl.servercount;

	M_ForceMenuOff ();
	SCR_ShowConsole (false, true);
	SCR_UpdateScreen ();
	unguard;
}

/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque (bool force)
{
	guard(SCR_EndLoadingPlaque);
	if (force || (cls.disable_servercount != cl.servercount && cl.refresh_prepped))
	{
		cls.loading = false;
		map_levelshot = NULL;
		Con_ClearNotify ();
		SCR_ShowConsole (false, true);
	}
	unguard;
}


static float conCurrent = 0;	// aproaches con_desired

//!! sometimes console should be painted before menu! (when cls.keep_console)
static void DrawGUI (bool allowNotifyArea)
{
	int		currTime, timeDelta;
	static int lastConTime = 0;
	float conDesired = 0;		// 0.0 to 1.0 lines of console to display

	Con_CheckResize ();

	// draw full-screen console in "console-only" mode and exit
	if (*re.flags & REF_CONSOLE_ONLY)
	{
		cls.key_dest = key_console;
		Con_DrawConsole (1.0f);
		return;
	}

	// decide on the height of the console
	if (cls.key_dest == key_console || cls.keep_console)
		conDesired = bound(con_maxSize->value, 0.1, 1);
	else
		conDesired = 0;		// none visible

	// scroll console
	currTime = Sys_Milliseconds ();
	timeDelta = lastConTime ? currTime - lastConTime : 0;
	lastConTime = currTime;
	if (conDesired < conCurrent)
	{
		conCurrent -= 3 * timeDelta / 1000.0f;
		if (conCurrent < conDesired)
			conCurrent = conDesired;
	}
	else if (conDesired > conCurrent)
	{
		conCurrent += 3 * timeDelta / 1000.0f;
		if (conCurrent > conDesired)
			conCurrent = conDesired;
	}
	else					// console in-place
		lastConTime = 0;

	// draw loading splash
	if (cls.loading)
	{
		if (developer->integer)
		{
			// draw full-screen console before loading plaque if in developer mode
			re.DrawDetailedPic (0, 0, viddef.width, viddef.height, "conback");
			Con_DrawConsole (1.0f);
#define DEV_SHOT_FRAC	4		// part of screen for levelshot when loading in "developer" mode
			if (map_levelshot)
				re.DrawStretchPic (viddef.width * (DEV_SHOT_FRAC-1) / DEV_SHOT_FRAC, 0,
					viddef.width / DEV_SHOT_FRAC, viddef.height / DEV_SHOT_FRAC, map_levelshot);
		}
		else
		{
			if (map_levelshot)
				re.DrawDetailedPic (0, 0, viddef.width, viddef.height, map_levelshot);
			else
			{
				int		w, h;

				re.DrawDetailedPic (0, 0, viddef.width, viddef.height, "conback");
				re.DrawGetPicSize (&w, &h, "loading");
				re.DrawPic ((viddef.width - w) / 2, (viddef.height - h) / 2, "loading");
			}
			if (!conCurrent)
				Con_DrawNotify (false);		// do not draw notify area when console is visible too
			allowNotifyArea = false;
		}
	}

	// draw downloading info
	if (cls.download && !(developer->integer && cls.loading))
	{
		int w = viddef.width / 2;
		int r = w * cls.downloadpercent / 100;
		int top = viddef.height - 3 * CHAR_HEIGHT - 2;
		int height = 2 * CHAR_HEIGHT + 4;
		re.DrawFill2 (6, top, r, height, RGB(0.5,0,0));
		re.DrawFill2 (6 + r, top, w - r, height, RGB(0.1,0.1,0.1));
		DrawString (8, viddef.height - 3 * CHAR_HEIGHT, va("Downloading: %s", cls.downloadname));
		DrawString (8, viddef.height - 2 * CHAR_HEIGHT, va("%d%% complete", cls.downloadpercent));
	}

	// draw menu
	M_Draw ();

	// draw console
	if (!cls.loading || !developer->integer)
		if (conCurrent)
			Con_DrawConsole (conCurrent);
		else if (allowNotifyArea && cl_draw2d->integer && (cls.key_dest == key_game || cls.key_dest == key_message))
				Con_DrawNotify (true);
}


void SCR_ShowConsole (bool show, bool noAnim)
{
	guard(SCR_ShowConsole);

	if (*re.flags & REF_CONSOLE_ONLY)
	{
		// ignore "show" arg
		cls.key_dest = key_console;
		return;
	}

	if (!show)
	{
		cls.key_dest = m_menudepth > 0 ? key_menu : key_game;
		if (noAnim) conCurrent = 0;
		if (cls.key_dest != key_menu)
			CL_Pause (false);
	}
	else
	{
		cls.key_dest = key_console;
		if (noAnim) conCurrent = bound(con_maxSize->value, 0.1, 1);	//?? different (may be, not needed)
		CL_Pause (true);
	}

	unguard;
}


void SCR_ToggleConsole (void)
{
	if (cls.keep_console) return;

	Con_ClearTyping ();
	Con_ClearNotify ();
	SCR_ShowConsole (cls.key_dest != key_console, false);
}


/*-----------------------------------------------------------------------------
	Screenshots
-----------------------------------------------------------------------------*/

static void Screenshot_f (bool usage, int argc, char **argv)
{
	int		i, flags;
	static char filename[MAX_OSPATH], tmpName[MAX_OSPATH];

	filename[0] = 0;
	flags = 0;

	if (usage)
	{
		Com_Printf ("Usage: screenshot [-levelshot] [-no2d] [-nogamma] [-silent] [-jpeg] [<filename>]\n");
		return;
	}

	for (i = 1; i < argc; i++)
	{
		char *opt = argv[i];
		if (opt[0] == '-')
		{
			opt++;
			if (!stricmp (opt, "levelshot"))
			{
				char	*tmp;

				if (cls.state != ca_active)
				{
					Com_WPrintf ("No levelshots in disconnected state\n");
					return;
				}

				if (!(tmp = strrchr (map_name, '/')))
				{
					Com_WPrintf ("Invalid map_name: %s\n", map_name);
					return;
				}
				tmp++;	// skip '/'

				flags |= SHOT_SMALL|SHOT_NO_2D|SHOT_NOGAMMA;
				appSprintf (ARRAY_ARG(filename), "%s/levelshots/%s", FS_Gamedir (), tmp);
				// cut extension
				tmp = strrchr (filename, '.');
				if (tmp) *tmp = 0;
			}
			else if (!stricmp (opt, "no2d"))
				flags |= SHOT_NO_2D;
			else if (!stricmp (opt, "nogamma"))
				flags |= SHOT_NOGAMMA;
			else if (!stricmp (opt, "silent"))
				flags |= SHOT_SILENT;
			else if (!stricmp (opt, "jpeg"))
				flags |= SHOT_JPEG;
			else
			{
				Com_WPrintf ("Unknown option: %s\n", opt);
				return;
			}
		}
		else
		{
			if (filename[0])
				Com_WPrintf ("WARNING: name already specified (%s). Changed.\n", filename);
			appSprintf (ARRAY_ARG(tmpName), "%s/screenshots/%s", FS_Gamedir (), opt);
			appCopyFilename (filename, tmpName, sizeof(filename));
		}
	}

	re.Screenshot (flags, filename);
}


//-----------------------------------------------------------------------------

/*
================
SCR_TimeRefresh_f
================
*/
static void TimeRefresh_f (void)
{
	int		i;
	int		start, stop;
	float	time;

	if ( cls.state != ca_active )
		return;

	start = Sys_Milliseconds ();

	for (i = 0; i < 128; i++)
	{
		cl.refdef.viewangles[1] = i/128.0*360.0;

		re.BeginFrame (0);
		re.RenderFrame (&cl.refdef);
		re.EndFrame();
	}

	stop = Sys_Milliseconds ();
	time = (stop-start)/1000.0;
	Com_Printf ("%f seconds (%f fps)\n", time, 128/time);
}


/*-----------------------------------------------------------------------------
	Game HUD
-----------------------------------------------------------------------------*/

#define	ICON_WIDTH		24
#define	ICON_HEIGHT		24
#define	HUDCHAR_WIDTH	16
#define	ICON_SPACE		8


static void SizeHUDString (const char *string, int *w, int *h)
{
	int		lines, width, current;
	char	c;

	lines = 1;
	width = 0;

	current = 0;
	while (c = *string++)
	{
		if (c == '\n')
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
	}

	*w = width * CHAR_WIDTH;
	*h = lines * CHAR_HEIGHT;
}

static void DrawHUDString (char *string, int x0, int y, int centerwidth, int color)
{
	int		width, i, x;
	char	c;

	while (*string)
	{
		// scan out one line of text from the string
		width = 0;
		while (true)
		{
			c = string[width];
			if (!c || c == '\n') break;
			width++;
		}

		x = centerwidth ? x0 + (centerwidth - width * CHAR_WIDTH) / 2 : x0;
		for (i = 0; i < width; i++)
		{
			re.DrawChar (x, y, *string++, color);
			x += CHAR_WIDTH;
		}
		if (*string)
		{
			string++;	// skip the \n
			y += CHAR_HEIGHT;
		}
	}
}


static void DrawField (int x, int y, int color, int width, int value)
{
	char	num[16], *ptr;

	if (width < 1)
		return;

	// draw number string
	int len = appSprintf (ARRAY_ARG(num), "%d", value);
	if (width > 5) width = 5;
	if (len > width) len = width;

	// align number to right
	x += 2 + HUDCHAR_WIDTH * (width - len);

	ptr = num;
	while (*ptr && len--)
	{
		if (*ptr == '-')
			re.DrawPic (x, y, va("%snum_minus", color ? "a" : ""));
		else
			re.DrawPic (x, y, va("%snum_%c", color ? "a" : "", *ptr));
		x += HUDCHAR_WIDTH;
		ptr++;
	}
}


#define	DISPLAY_ITEMS	17

static void DrawInventory (void)
{
	int		i, index[MAX_ITEMS];

	int num = 0;
	int selected_num = 0;
	int selected = cl.frame.playerstate.stats[STAT_SELECTED_ITEM];
	for (i = 0; i < MAX_ITEMS; i++)
	{
		if (i == selected)
			selected_num = num;
		if (cl.inventory[i])
		{
			index[num] = i;
			num++;
		}
	}

	// determine scroll point
	int top = selected_num - DISPLAY_ITEMS/2;
	if (num - top < DISPLAY_ITEMS)
		top = num - DISPLAY_ITEMS;
	if (top < 0)
		top = 0;

	int x = (viddef.width-256)/2;
	int y = (viddef.height-240)/2;

	re.DrawPic (x, y+CHAR_HEIGHT, "inventory");

	y += 3 * CHAR_HEIGHT;
	x += 3 * CHAR_WIDTH;
	DrawString (x, y,			  S_GREEN"hotkey ### item");
	DrawString (x, y+CHAR_HEIGHT, S_GREEN"------ --- ----");
	y += 2 * CHAR_HEIGHT;

	for (i = top; i < num && i < top+DISPLAY_ITEMS; i++)
	{
		int		key;
		char	binding[256];
		const char *keyName;

		int item = index[i];
		appSprintf (ARRAY_ARG(binding), "use %s", cl.configstrings[CS_ITEMS+item]);
		if (Key_FindBinding (binding, &key, 1))
			keyName = Key_KeynumToString (key);
		else
			keyName = "";

		if (item == selected)
			re.DrawChar (x-CHAR_WIDTH, y, 13);	//?? original: 15 (but not displayed) anyway

		DrawString (x, y, va("%s%6s %3i %s", (item == selected) ? S_GREEN: S_WHITE,
			keyName, cl.inventory[item], cl.configstrings[CS_ITEMS+item]));
		y += CHAR_HEIGHT;
	}
}


static char	crosshair_pic[MAX_QPATH];
static int	crosshair_width, crosshair_height;

static void DrawCrosshair (void)
{
	if (!crosshair->integer || cl.refdef.rdflags & RDF_THIRD_PERSON)
		return;

	if (crosshair->modified)
	{
		crosshair->modified = false;
		SCR_TouchPics ();
	}

	if (!crosshair_pic[0]) return;

	re.DrawPic ((viddef.width - crosshair_width) / 2, (viddef.height - crosshair_height) / 2,
		crosshair_pic, crosshairColor->integer);
}

/*
================
SCR_ExecuteLayoutString
================
*/
static void ExecuteLayoutString (const char *s)
{
	int		x, y;
	int		value;
	char	*token;
	int		width;
	int		index;
	clientinfo_t	*ci;

	guard(SCR_ExecuteLayoutString);

	if (cls.state != ca_active || !cl.refresh_prepped)
		return;

	if (!s[0]) return;

	x = y = 0;
	width = 3;

	while (s)
	{
		token = COM_Parse (s);
		if (!strcmp (token, "xl"))
			x = atoi (COM_Parse (s));
		else if (!strcmp (token, "xr"))
			x = viddef.width + atoi (COM_Parse (s));
		else if (!strcmp (token, "xv"))
			x = viddef.width/2 - 160 + atoi (COM_Parse (s));
		else if (!strcmp (token, "yt"))
			y = atoi (COM_Parse (s));
		else if (!strcmp (token, "yb"))
			y = viddef.height + atoi (COM_Parse (s));
		else if (!strcmp (token, "yv"))
			y = viddef.height/2 - 120 + atoi (COM_Parse (s));
		else if (!strcmp (token, "pic"))
		{	// draw a pic from a stat number
			value = cl.frame.playerstate.stats[atoi (COM_Parse (s))];
			if (value >= MAX_IMAGES)
				Com_DropError ("Pic >= MAX_IMAGES");
			if (cl.configstrings[CS_IMAGES+value])
				re.DrawPic (x, y, cl.configstrings[CS_IMAGES+value]);
		}
		else if (!strcmp (token, "client"))
		{	// draw a deathmatch client block
			x = viddef.width/2 - 160 + atoi (COM_Parse (s));
			y = viddef.height/2 - 120 + atoi (COM_Parse (s));

			value = atoi (COM_Parse (s));
			if (value >= MAX_CLIENTS || value < 0)
				Com_DropError ("client >= MAX_CLIENTS");
			ci = &cl.clientinfo[value];

			int score = atoi (COM_Parse (s));
			int ping = atoi (COM_Parse (s));
			int time = atoi (COM_Parse (s));

			int x1 = x+32;
			DrawString (x1, y,				va(S_GREEN"%s", ci->name));
			DrawString (x1, y+CHAR_HEIGHT,	va("Score: "S_GREEN"%d", score));
			DrawString (x1, y+CHAR_HEIGHT*2,va("Ping:  %d", ping));
			DrawString (x1, y+CHAR_HEIGHT*3,va("Time:  %d", time));

			if (!ci->icon)
				ci = &cl.baseclientinfo;
			re.DrawPic (x, y, ci->iconname);
		}
		else if (!strcmp (token, "ctf"))
		{	// draw a ctf client block
			char	block[80];

			x = viddef.width/2 - 160 + atoi (COM_Parse (s));
			y = viddef.height/2 - 120 + atoi (COM_Parse (s));

			value = atoi (COM_Parse (s));
			if (value >= MAX_CLIENTS || value < 0)
				Com_DropError ("client >= MAX_CLIENTS");
			ci = &cl.clientinfo[value];

			int score = atoi (COM_Parse (s));
			int ping = atoi (COM_Parse (s));
			if (ping > 999) ping = 999;

			appSprintf (ARRAY_ARG(block), "%3d %3d %-12.12s", score, ping, ci->name);

			if (value == cl.playernum)
				DrawString (x, y, va(S_RED"%s", block));
			else
				DrawString (x, y, block);
		}
		else if (!strcmp (token, "picn"))
		{	// draw a pic from a name
			re.DrawPic (x, y, COM_Parse (s));
		}
		else if (!strcmp (token, "num"))
		{	// draw a number
			width = atoi (COM_Parse (s));
			value = cl.frame.playerstate.stats[atoi (COM_Parse (s))];
			DrawField (x, y, 0, width, value);
		}
		else if (!strcmp (token, "hnum"))
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
				re.DrawPic (x, y, "field_3");

			DrawField (x, y, color, width, value);
		}
		else if (!strcmp (token, "anum"))
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
				re.DrawPic (x, y, "field_3");

			DrawField (x, y, color, width, value);
		}
		else if (!strcmp (token, "rnum"))
		{	// armor number
			int		color;

			width = 3;
			value = cl.frame.playerstate.stats[STAT_ARMOR];
			if (value < 1) continue;

			color = 0;	// green

			if (cl.frame.playerstate.stats[STAT_FLASHES] & 2)
				re.DrawPic (x, y, "field_3");

			DrawField (x, y, color, width, value);
		}
		else if (!strcmp (token, "stat_string"))
		{
			index = cl.frame.playerstate.stats[atoi (COM_Parse (s))];
			if (index < 0 || index >= MAX_CONFIGSTRINGS)
				Com_DropError ("Bad stat_string index");
			DrawString (x, y, cl.configstrings[index]);
		}
		else if (!strcmp (token, "cstring"))
			DrawHUDString (COM_Parse (s), x, y, 320, C_WHITE);
		else if (!strcmp (token, "cstring2"))
			DrawHUDString (COM_Parse (s), x, y, 320, C_GREEN);
		else if (!strcmp (token, "string"))
			DrawString (x, y, COM_Parse (s));
		else if (!strcmp (token, "string2"))
			DrawString (x, y, va(S_GREEN"%s", COM_Parse (s)));
		else if (!strcmp (token, "if"))
		{	// draw a number
			value = cl.frame.playerstate.stats[atoi (COM_Parse (s))];
			if (!value)
			{	// skip to endif
				while (s && strcmp (token, "endif"))
					token = COM_Parse (s);
			}
		}
	}

	unguard;
}


//=======================================================

/*
===============
SCR_TouchPics

Allows rendering code to cache all needed sbar graphics
===============
*/
void SCR_TouchPics (void)
{
	int		ch_num;

	if (*re.flags & REF_CONSOLE_ONLY)
		return;

//	for (int i = 0; i < 2; i++)
//		for (int j = 0 ; j < 11 ; j++)
//			re.RegisterPic (sb_nums[i][j]);		// can remember image handles and use later (faster drawing, but need API extension ??)

	ch_num = crosshair->integer;
	if (ch_num)
	{
		if (ch_num > 0)
		{
			appSprintf (ARRAY_ARG(crosshair_pic), "ch%d", crosshair->integer);
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
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen (void)
{
	float separation[2] = {0, 0};

	guard(SCR_UpdateScreen);

	if (!initialized) return;		// not initialized yet

	// range check cl_camera_separation so we don't inadvertently fry someone's brain
	Cvar_Clamp (cl_stereo_separation, 0, 1);

	int numframes;
	if (cl_stereo->integer)
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

	for (int i = 0; i < numframes; i++)
	{
		re.BeginFrame (separation[i]);

		if (cl.cinematicActive)
		{
			if (!SCR_DrawCinematic ())
				re.DrawFill2 (0, 0, viddef.width, viddef.height, RGB(0,0,0));
			DrawGUI (false);
		}
		else
		{
			if (V_RenderView (separation[i]))
			{
				//------------------- HUD --------------------
				if (cl_draw2d->integer)
				{
					DrawCrosshair ();
					// SCR_DrawStats:
					ExecuteLayoutString (cl.configstrings[CS_STATUSBAR]);
					// SCR_DrawLayout:
					if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 1)
						ExecuteLayoutString (cl.layout);
					// draw inventory
					if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 2)
						DrawInventory ();

					// show disconnected icon when server is not responding
					if (cl.overtime > 200)
						re.DrawPic (64, 0, "net");

					DrawCenterString ();

					// draw pause
					if (cl_paused->integer)
					{
						int		w, h;
						re.DrawGetPicSize (&w, &h, "pause");
						re.DrawPic ((viddef.width - w) / 2, (viddef.height - h) / 2, "pause");
					}

					DrawChatInput ();
				}
			}
			else
			{
				// 3D not rendered - draw background
				if (!cls.loading || !map_levelshot)
					re.DrawDetailedPic (0, 0, viddef.width, viddef.height, "conback");
				if (cls.state == ca_disconnected && !cls.loading)
					M_ForceMenuOn ();
			}

			DrawGUI (true);

			if (timegraph->integer)
				SCR_DebugGraph (cls.frametime * 300, 0);
			if (debuggraph->integer || timegraph->integer || netgraph->integer)
				DrawDebugGraph ();
		}
	}
	re.EndFrame();

	unguard;
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
	CVAR_VAR(crosshair, 0, CVAR_ARCHIVE),
	CVAR_FULL(&crosshairColor, "crosshairColor", STR(C_WHITE), CVAR_ARCHIVE),

	CVAR_VAR(con_maxSize, 0.8, 0),
	CVAR_VAR(scr_centertime, 2.5, 0),
	CVAR_VAR(cl_draw2d, 1, 0),
	CVAR_VAR(netgraph, 0, 0),
	CVAR_VAR(timegraph, 0, 0),
	CVAR_VAR(debuggraph, 0, 0),
	CVAR_VAR(graphheight, 32, 0),
	CVAR_VAR(graphscale, 1, 0),
	CVAR_VAR(graphshift, 0, 0)
CVAR_END

	Cvar_GetVars (ARRAY_ARG(vars));

	RegisterCommand ("timerefresh", TimeRefresh_f);
	RegisterCommand ("loading", SCR_BeginLoadingPlaque);
	RegisterCommand ("toggleconsole", SCR_ToggleConsole);
	RegisterCommand ("messagemode", SCR_MessageMode_f);
	RegisterCommand ("messagemode2", SCR_MessageMode2_f);
	RegisterCommand ("screenshot", Screenshot_f);

	initialized = true;
}
