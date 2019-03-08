#include "client.h"
#include "cmodel.h"			// for bspfile.Name

static bool initialized;

// cvars (non-static for menu)
cvar_t		*crosshair;
cvar_t		*crosshairColor;


// static cvars
static cvar_t	*con_maxSize;
static cvar_t	*scr_centertime;
static cvar_t	*cl_draw2d;

#define CHAR_WIDTH	8
#define CHAR_HEIGHT	8

// Function DrawString used for painting HUD
//?? may add shadow fx
void DrawString(int x, int y, const char *s)
{
	int color = C_WHITE;
	while (char c = *s++)
	{
		if (c == COLOR_ESCAPE)
		{
			int col = *s - '0';
			if (col >= 0 && col <= 7)
			{
				color = col;
				s++;
				continue;
			}
		}
		RE_DrawChar(x, y, c, color);
		x += CHAR_WIDTH;
	}
}


/*-----------------------------------------------------------------------------
	Graphs (remove, or replace ??)
-----------------------------------------------------------------------------*/

static cvar_t *netgraph, *timegraph, *debuggraph;
static cvar_t *graphheight, *graphscale, *graphshift;

static int gr_current;
static struct {
	float	value;
	int		color;
} gr_values[1024];


void SCR_DebugGraph(float value, int color)
{
	gr_values[gr_current&1023].value = value;
	gr_values[gr_current&1023].color = color;
	gr_current++;
}


static void DrawDebugGraph()
{
	int w = min(viddef.width, 1024);
	RE_Fill8(0, viddef.height - graphheight->integer, w, graphheight->integer, 8);

	for (int a = 0; a < w; a++)
	{
		int i = (gr_current-1-a+1024) & 1023;
		float v = gr_values[i].value * graphscale->value + graphshift->value;
		int color = gr_values[i].color;

		if (v < 0)
			v += graphheight->integer * (1 + appRound(-v / graphheight->value));
		int h = appRound(v) % graphheight->integer;
		RE_Fill8(w-1-a, viddef.height - h, 1, h, color);
	}
}


/*-----------------------------------------------------------------------------
	Center print
-----------------------------------------------------------------------------*/

static char centerstring[1024];
static int	center_lines;
static float centertime_off;


// Called for important messages that should stay in the center of the screen
// for a few moments
void SCR_CenterPrint(const char *str)
{
	appStrncpyz(centerstring, str, sizeof(centerstring));
	centertime_off = scr_centertime->value;

	// count the number of lines for centering
	center_lines = 1;
	const char *s = str;
	while (*s)
		if (*s++ == '\n') center_lines++;

	// echo it to the console
	appPrintf("\n------------------------------------\n\n");
	s = str;
	while (true)
	{
		//?? UGLY WAY ...
		char	line[64];
		int		i, j, len;
		// scan the width of the line
		for (len = 0; len < 40; len++)
			if (s[len] == '\n' || !s[len])
				break;
		for (i = 0; i < (40-len)/2; i++)
			line[i] = ' ';

		for (j = 0; j < len; j++)
			line[i++] = s[j];

		line[i] = '\n';
		line[i+1] = 0;

		appPrintf("%s", line);

		while (*s && *s != '\n')
			s++;

		if (!*s) break;
		s++;		// skip the \n
	}
	appPrintf("------------------------------------\n");
	Con_ClearNotify();
}


static void DrawCenterString()
{
	centertime_off -= cls.frametime;
	if (centertime_off <= 0) return;

	int y = (center_lines <= 4) ? appRound(viddef.height * 0.35f) : 48;
	const char *start = centerstring;
	while (true)
	{
		// scan the width of the line
		int len;
		for (len = 0; len < 40; len++)
			if (start[len] == '\n' || !start[len])
				break;
		int x = (viddef.width - len * CHAR_WIDTH) / 2;
		for (int i = 0; i < len; i++, x += CHAR_WIDTH)
			RE_DrawChar(x, y, *start++);

		y += CHAR_HEIGHT;

		if (!*start) break;
		start++;		// skip the '\n'
	}
}


/*-----------------------------------------------------------------------------
	Chat input
-----------------------------------------------------------------------------*/

static bool		chat_team;
static char		chat_buffer[MAXCMDLINE];
static int		chat_bufferlen = 0;


static void DrawChatInput()
{
	// edit current player message
	if (cls.key_dest != key_message) return;

	int x;
	int v = CHAR_HEIGHT * 8;		//?? num notify lines
	if (chat_team)
	{
		DrawString(8, v, "say_team:");
		x = 11;
	}
	else
	{
		DrawString(8, v, "say:");
		x = 5;
	}

	const char *s = chat_buffer;
	if (chat_bufferlen > viddef.width/CHAR_WIDTH - (x + 1))
		s += chat_bufferlen - (viddef.width/CHAR_WIDTH - (x + 1));
	while (*s)
	{
		RE_DrawChar(x * CHAR_WIDTH, v, *s++);
		x++;
	}
	// draw cursor
	RE_DrawChar(x * CHAR_WIDTH, v, 10 + ((cls.realtime >> 8) & 1));
}


void Key_Message(int key)
{
	if (key == K_ENTER || key == K_KP_ENTER)
	{
		if (chat_team)
			Cbuf_AddText("say_team \"");
		else
			Cbuf_AddText("say \"");
		Cbuf_AddText(chat_buffer);
		Cbuf_AddText("\"\n");

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


static void MessageMode_f()
{
	chat_team = false;
	cls.key_dest = key_message;
}


static void MessageMode2_f()
{
	chat_team = true;
	cls.key_dest = key_message;
}


/*-----------------------------------------------------------------------------
	Console and loading plaque
-----------------------------------------------------------------------------*/

static const char *map_levelshot;
static TString<64> LoadingNotify;
static float loadingFrac = 0;

void SCR_SetLevelshot(const char *name)
{
	static const char defLevelshot[] = "pics/levelshot";

	if (!name)
	{
		// get levelshot name from map name
		TString<64> MapName;
		MapName.filename(cl.configstrings[CS_MODELS+1]);
		char *tmp = MapName.chr('.');
		if (tmp) tmp[0] = 0;
		tmp = MapName.rchr('/');
		tmp = tmp ? tmp+1 : MapName;
		name = va("levelshots/%s", tmp);
	}

	if (map_levelshot && stricmp(map_levelshot, defLevelshot))
		return;						// already set non-default levelshot

	if (ImageExists(name))
	{
		static TString<MAX_OSPATH> Levelshot;

		Com_DPrintf("SetLevelshot: %s\n", name);
		Levelshot = name;
		map_levelshot = Levelshot;
		// force renderer to refresh image
		CBasicImage *img = RE_RegisterPic(name);
		if (img) img->Reload();
	}
	else if (ImageExists(defLevelshot))
	{
		if (map_levelshot != defLevelshot)
		{
			Com_DPrintf("SetLevelshot: %s\n", defLevelshot);
			map_levelshot = defLevelshot;
		}
	}
	else
		map_levelshot = NULL;

	SCR_BeginLoadingPlaque();
}


void SCR_BeginLoadingPlaque(bool server)
{
	guard(SCR_BeginLoadingPlaque);

	if (server) cl.configstrings[CS_NAME][0] = 0;	// clear previous message

	S_StopAllSounds_f();
	cl.sound_ambient = false;						// don't play ambients
	CDAudio_Stop();
	cls.loading = true;
	cls.disable_servercount = cl.servercount;
	LoadingNotify[0] = 0;

	M_ForceMenuOff();
	SCR_ShowConsole(false, true);
	SCR_UpdateScreen();
	unguard;
}


void SCR_EndLoadingPlaque(bool force)
{
	guard(SCR_EndLoadingPlaque);
	if (force || (cls.disable_servercount != cl.servercount && cl.rendererReady))
	{
		cls.loading = false;
		map_levelshot = NULL;
		Con_ClearNotify();
		SCR_ShowConsole(false, true);
	}
	unguard;
}


void SCR_LoadingNotify(const char *msg, float frac)
{
	LoadingNotify = msg;
	loadingFrac   = frac;
	Sys_ProcessMessages();
	SCR_UpdateScreen();
}


static float conCurrent = 0;	// aproaches con_desired

//!! sometimes console should be painted before menu! (when cls.keep_console)
static void DrawGUI(bool allowNotifyArea)
{
	Con_CheckResize();

	// decide on the height of the console
	float conDesired = 0;		// 0.0 to 1.0 lines of console to display
	if (cls.key_dest == key_console || cls.keep_console)
		conDesired = bound(con_maxSize->value, 0.1, 1);
	else
		conDesired = 0;		// none visible

	// scroll console
	static unsigned lastConTime = 0;
	unsigned currTime  = appMilliseconds();
	unsigned timeDelta = lastConTime ? currTime - lastConTime : 0;
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
		if (DEVELOPER)
		{
			// draw full-screen console before loading plaque if in developer mode
			RE_DrawDetailedPic(0, 0, viddef.width, viddef.height, "pics/conback");
			Con_DrawConsole(1.0f);
#define DEV_SHOT_FRAC	4		// part of screen for levelshot when loading in "developer" mode
			if (map_levelshot)
				RE_DrawStretchPic(viddef.width * (DEV_SHOT_FRAC-1) / DEV_SHOT_FRAC, 0,
					viddef.width / DEV_SHOT_FRAC, viddef.height / DEV_SHOT_FRAC, map_levelshot);
		}
		else
		{
			if (map_levelshot)
				RE_DrawDetailedPic(0, 0, viddef.width, viddef.height, map_levelshot);
			else
			{
				RE_DrawDetailedPic(0, 0, viddef.width, viddef.height, "pics/conback");
				RE_DrawPic(viddef.width / 2, viddef.height / 2, "pics/loading", ANCHOR_CENTER);
			}
			// common params
			int left   = viddef.width / 4;
			int w      = viddef.width / 2;
			int r      = appRound(w * loadingFrac);
			int top    = viddef.height * 4 / 5;
			int height = 2 * CHAR_HEIGHT;
			// map name -- draw multi-line centered message
			if (cl.configstrings[CS_NAME][0])
			{
				const char *src = cl.configstrings[CS_NAME];
				int shift = 0;
				while (src)
				{
					char buf[80]; // 64 (configstring size) + gap
					const char *end = strchr(src, '\n');
					int len;
					if (end)
					{
						len = end - src;
						if (len > 64) len = 64; // in a case map name exactly fits 64 bytes, i.e. no zero char
						appStrncpyz(buf, src, len + 1);
						src = end + 1;
					}
					else
					{
						len = strlen(src);
						memcpy(buf, src, len);
						buf[len] = 0;
						src = NULL;
					}
					// note: up to 6 lines until overlap
					DrawString((viddef.width - CHAR_WIDTH*len) / 2, top - CHAR_HEIGHT*6 + shift, va(S_YELLOW"%s", buf));
					shift += CHAR_HEIGHT;
				}
			}
			if (LoadingNotify[0])
			{
				// graphical progress indicator
				RE_Fill(left, top, r, height, RGBA(1,1,1,0.1));
				RE_Fill(left + r, top, w - r, height, RGBA(0.1,0.1,0.1,0.5));
				// hotification text
				DrawString(left + 8, top + 4, va(S_BLACK"%s", *LoadingNotify));
			}
			allowNotifyArea = false;
		}
	}

	// draw downloading info
	if (cls.download && !(DEVELOPER && cls.loading))
	{
		int w      = viddef.width / 2;
		int r      = w * cls.downloadPercent / 100;
		int top    = viddef.height - 3 * CHAR_HEIGHT - 2;
		int height = 2 * CHAR_HEIGHT + 4;
		RE_Fill(6, top, r, height, RGB(0.5,0,0));
		RE_Fill(6 + r, top, w - r, height, RGB(0.1,0.1,0.1));
		DrawString(8, viddef.height - 3 * CHAR_HEIGHT, va("Downloading: %s", *cls.DownloadName));
		DrawString(8, viddef.height - 2 * CHAR_HEIGHT, va("%d%% complete", cls.downloadPercent));
	}

	// draw menu
	M_Draw();

	// draw console
	if (!(cls.loading && DEVELOPER))
	{
		if (conCurrent)
			Con_DrawConsole(conCurrent);
		else if (allowNotifyArea && cl_draw2d->integer && (cls.key_dest == key_game || cls.key_dest == key_message))
			Con_DrawNotify(true);
	}
}


void SCR_ShowConsole(bool show, bool noAnim)
{
	guard(SCR_ShowConsole);

	if (!show)
	{
		cls.key_dest = (m_current) ? key_menu : key_game;
		if (noAnim) conCurrent = 0;
		if (cls.key_dest != key_menu)
			CL_Pause(false);
	}
	else
	{
		cls.key_dest = key_console;
		if (noAnim) conCurrent = bound(con_maxSize->value, 0.1, 1);	//?? different (may be, not needed)
		CL_Pause(true);
	}

	unguard;
}


void SCR_ToggleConsole()
{
	if (cls.keep_console) return;

	Con_ClearTyping();
	Con_ClearNotify();
	SCR_ShowConsole(cls.key_dest != key_console, false);
}


static void Screenshot_f(bool usage, int argc, char **argv)
{
	if (usage)
	{
		appPrintf("Usage: screenshot [-levelshot] [-no2d] [-nogamma] [-silent] [-jpeg] [<filename>]\n");
		return;
	}

	static TString<MAX_OSPATH> Filename;
	Filename[0] = 0;
	int flags = 0;
	for (int i = 1; i < argc; i++)
	{
		const char *opt = argv[i];
		if (opt[0] == '-')
		{
			opt++;
			if (!stricmp(opt, "levelshot"))
			{
				if (cls.state != ca_active)
				{
					appWPrintf("No levelshots when disconnected\n");
					return;
				}

				char *tmp;
				if (!(tmp = strrchr(bspfile.Name, '/')))
				{
					appWPrintf("Invalid map_name: %s\n", *bspfile.Name);
					return;
				}
				tmp++;	// skip '/'

				flags |= SHOT_SMALL|SHOT_NO_2D|SHOT_NOGAMMA;
				Filename.sprintf("./%s/levelshots/%s", FS_Gamedir(), tmp);
				// cut extension
				tmp = Filename.rchr('.');
				if (tmp) *tmp = 0;
			}
			else if (!stricmp(opt, "no2d"))
				flags |= SHOT_NO_2D;
			else if (!stricmp(opt, "nogamma"))
				flags |= SHOT_NOGAMMA;
			else if (!stricmp(opt, "silent"))
				flags |= SHOT_SILENT;
			else if (!stricmp(opt, "jpeg"))
				flags |= SHOT_JPEG;
			else
			{
				appWPrintf("Unknown option: %s\n", opt);
				return;
			}
		}
		else
		{
			if (Filename[0])
				appWPrintf("WARNING: name already specified (%s). Changed.\n", *Filename);
			Filename.filename(va("./%s/screenshots/%s", FS_Gamedir(), opt));
		}
	}

	RE_Screenshot(flags, Filename);
}


static void TimeRefresh_f(bool usage, int argc, char **argv)
{
	//?? name of this func came from "time refresh" == "profile renderer"
	//?? we was renamed refresh=>renderer, so - rename this func too?
	if (usage || argc > 2)
	{
		appPrintf("Usage: timerefresh [steps=256]\n");
		return;
	}
	if (cls.state != ca_active) return;

	keydest_t keyDest = cls.key_dest;

	int64 start = appCycles64();
	double time = 0;

	int steps = (argc == 2) ? atoi(argv[1]) : 256;
	for (int i = 0; i < steps; i++)
	{
		cl.refdef.viewangles[1] = (float)i * 360 / steps;

		RE_BeginFrame(cls.realtime / 1000.0);
		RE_RenderFrame(&cl.refdef);
		RE_EndFrame();
		int64 timeDelta = appCycles64() - start;
		time = appCyclesToMsecf(timeDelta) / 1000;
		// prevent too long measuring
		Sys_ProcessMessages();			// poll keyboard
		if (cls.key_dest != keyDest)	// console or menu was activated or deactivated
		{
			steps = i;
			break;
		}
	}

	appPrintf("%d frames/%g seconds (%g fps)\n", steps, time, steps / time);
}


/*-----------------------------------------------------------------------------
	Game HUD
-----------------------------------------------------------------------------*/

#define	ICON_WIDTH		24
#define	ICON_HEIGHT		24
#define	HUDCHAR_WIDTH	16
#define	ICON_SPACE		8

#if 0
static void SizeHUDString(const char *string, int *w, int *h)
{
	int lines = 1;
	int width = 0;
	int current = 0;
	while (char c = *string++)
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
#endif

static void DrawHUDString(char *string, int x0, int y, int centerwidth, int color)
{
	while (*string)
	{
		// scan out one line of text from the string
		int width = 0;
		while (true)
		{
			char c = string[width];
			if (!c || c == '\n') break;
			width++;
		}

		int x = centerwidth ? x0 + (centerwidth - width * CHAR_WIDTH) / 2 : x0;
		for (int i = 0; i < width; i++)
		{
			RE_DrawChar(x, y, *string++, color);
			x += CHAR_WIDTH;
		}
		if (*string)
		{
			string++;	// skip the \n
			y += CHAR_HEIGHT;
		}
	}
}


static void DrawField(int x, int y, bool color, int width, int value)
{
	if (width < 1) return;

	// draw number string
	TString<16> Num;
	int len = Num.sprintf("%d", value);
	if (width > 5) width = 5;
	if (len > width) len = width;

	// align number to right
	x += 2 + HUDCHAR_WIDTH * (width - len);

	const char *ptr = Num;
	while (*ptr && len--)
	{
		if (*ptr == '-')
			RE_DrawPic(x, y, va("pics/%snum_minus", color ? "a" : ""));
		else
			RE_DrawPic(x, y, va("pics/%snum_%c", color ? "a" : "", *ptr));
		x += HUDCHAR_WIDTH;
		ptr++;
	}
}


#define	DISPLAY_ITEMS	17

static void DrawInventory()
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

	RE_DrawPic(x, y+CHAR_HEIGHT, "pics/inventory");

	y += 3 * CHAR_HEIGHT;
	x += 3 * CHAR_WIDTH;
	DrawString(x, y,			  S_GREEN"hotkey ### item");
	DrawString(x, y+CHAR_HEIGHT, S_GREEN"------ --- ----");
	y += 2 * CHAR_HEIGHT;

	for (i = top; i < num && i < top+DISPLAY_ITEMS; i++)
	{
		int item = index[i];
		TString<256> Binding;
		Binding.sprintf("use %s", cl.configstrings[CS_ITEMS+item]);

		int		key;
		const char *keyName;
		if (Key_FindBinding(Binding, &key, 1))
			keyName = Key_KeynumToString(key);
		else
			keyName = "";

		if (item == selected)
			RE_DrawChar(x-CHAR_WIDTH, y, 13);	//?? original: 15 (but not displayed anyway)

		DrawString(x, y, va("%s%6s %3i %s", (item == selected) ? S_GREEN: S_WHITE,
			keyName, cl.inventory[item], cl.configstrings[CS_ITEMS+item]));
		y += CHAR_HEIGHT;
	}
}


static TString<MAX_QPATH> CrosshairPic;

static void DrawCrosshair()
{
	if (!crosshair->integer || cl.refdef.rdflags & RDF_THIRD_PERSON)
		return;

	if (crosshair->modified)
	{
		crosshair->modified = false;
		SCR_TouchPics();
	}

	if (!CrosshairPic[0]) return;

	RE_DrawPic(viddef.width / 2, viddef.height / 2, CrosshairPic, ANCHOR_CENTER, crosshairColor->integer);
}


static void ExecuteLayoutString(const char *s)
{
	guard(SCR_ExecuteLayoutString);

	if (cls.state != ca_active || !cl.rendererReady)
		return;

	if (!s[0]) return;

	int x = 0, y = 0;

	while (s)
	{
		int value, index;
		clientInfo_t *ci;

		const char *token = COM_Parse(s);
		if (!strcmp(token, "xl"))
			x = atoi(COM_Parse(s));
		else if (!strcmp(token, "xr"))
			x = viddef.width + atoi(COM_Parse(s));
		else if (!strcmp(token, "xv"))
			x = viddef.width/2 - 160 + atoi(COM_Parse(s));
		else if (!strcmp(token, "yt"))
			y = atoi(COM_Parse(s));
		else if (!strcmp(token, "yb"))
			y = viddef.height + atoi(COM_Parse(s));
		else if (!strcmp(token, "yv"))
			y = viddef.height/2 - 120 + atoi(COM_Parse(s));
		else if (!strcmp(token, "pic"))
		{	// draw a pic from a stat number
			value = cl.frame.playerstate.stats[atoi(COM_Parse(s))];
			if (value >= MAX_IMAGES)
				Com_DropError("Pic >= MAX_IMAGES");
			if (cl.configstrings[CS_IMAGES+value][0])
				RE_DrawPic(x, y, va("pics/%s", cl.configstrings[CS_IMAGES+value]));
		}
		else if (!strcmp(token, "client"))
		{	// draw a deathmatch client block
			x = viddef.width/2 - 160 + atoi(COM_Parse(s));
			y = viddef.height/2 - 120 + atoi(COM_Parse(s));

			value = atoi(COM_Parse(s));
			if (value >= MAX_CLIENTS || value < 0)
				Com_DropError("client >= MAX_CLIENTS");
			ci = &cl.clientInfo[value];

			int score = atoi(COM_Parse(s));
			int ping  = atoi(COM_Parse(s));
			int time  = atoi(COM_Parse(s));

			int x1 = x+32;
			DrawString(x1, y,				va(S_GREEN"%s", *ci->PlayerName));
			DrawString(x1, y+CHAR_HEIGHT,	va("Score: "S_GREEN"%d", score));
			DrawString(x1, y+CHAR_HEIGHT*2,va("Ping:  %d", ping));
			DrawString(x1, y+CHAR_HEIGHT*3,va("Time:  %d", time));

			RE_DrawStretchPic(x, y, 32, 32, ci->IconName);
		}
		else if (!strcmp(token, "ctf"))
		{	// draw a ctf client block
			x = viddef.width/2 - 160 + atoi(COM_Parse(s));
			y = viddef.height/2 - 120 + atoi(COM_Parse(s));

			value = atoi(COM_Parse(s));
			if (value >= MAX_CLIENTS || value < 0)
				Com_DropError("client >= MAX_CLIENTS");
			ci = &cl.clientInfo[value];

			int score = atoi(COM_Parse(s));
			int ping = atoi(COM_Parse(s));
			if (ping > 999) ping = 999;

			TString<64> Str;
			Str.sprintf("%3d %3d %-12.12s", score, ping, *ci->PlayerName);

			if (value == cl.playernum)
				DrawString(x, y, va(S_RED"%s", *Str));
			else
				DrawString(x, y, *Str);
		}
		else if (!strcmp(token, "picn"))
		{	// draw a pic from a name
			RE_DrawPic(x, y, va("pics/%s", COM_Parse(s)));
		}
		else if (!strcmp(token, "num"))
		{	// draw a number
			int width = atoi(COM_Parse(s));
			value = cl.frame.playerstate.stats[atoi(COM_Parse(s))];
			DrawField(x, y, false, width, value);
		}
		else if (!strcmp(token, "hnum"))
		{	// health number
			bool	color;

			value = cl.frame.playerstate.stats[STAT_HEALTH];
			if (value > 25)
				color = false;	// green
			else if (value > 0)
				color = (cl.frame.serverframe>>2) & 1;		// flash
			else
				color = true;

			if (cl.frame.playerstate.stats[STAT_FLASHES] & 1)
				RE_DrawPic(x, y, "pics/field_3");

			DrawField(x, y, color, 3, value);
		}
		else if (!strcmp(token, "anum"))
		{	// ammo number
			bool	color;

			value = cl.frame.playerstate.stats[STAT_AMMO];
			if (value > 5)
				color = false;	// green
			else if (value >= 0)
				color = (cl.frame.serverframe>>2) & 1;		// flash
			else
				continue;	// negative number = don't show

			if (cl.frame.playerstate.stats[STAT_FLASHES] & 4)
				RE_DrawPic(x, y, "pics/field_3");

			DrawField(x, y, color, 3, value);
		}
		else if (!strcmp(token, "rnum"))
		{	// armor number
			value = cl.frame.playerstate.stats[STAT_ARMOR];
			if (value < 1) continue;

			if (cl.frame.playerstate.stats[STAT_FLASHES] & 2)
				RE_DrawPic(x, y, "pics/field_3");

			DrawField(x, y, false, 3, value);		// color = green
		}
		else if (!strcmp(token, "stat_string"))
		{
			index = cl.frame.playerstate.stats[atoi(COM_Parse(s))];
			if (index < 0 || index >= MAX_CONFIGSTRINGS)
				Com_DropError("Bad stat_string index");
			DrawString(x, y, cl.configstrings[index]);
		}
		else if (!strcmp(token, "cstring"))
			DrawHUDString(COM_Parse(s), x, y, 320, C_WHITE);
		else if (!strcmp(token, "cstring2"))
			DrawHUDString(COM_Parse(s), x, y, 320, C_GREEN);
		else if (!strcmp(token, "string"))
			DrawString(x, y, COM_Parse(s));
		else if (!strcmp(token, "string2"))
			DrawString(x, y, va(S_GREEN"%s", COM_Parse(s)));
		else if (!strcmp(token, "if"))
		{	// draw a number
			value = cl.frame.playerstate.stats[atoi(COM_Parse(s))];
			if (!value)
			{	// skip to endif
				while (s && strcmp(token, "endif"))
					token = COM_Parse(s);
			}
		}
	}

	unguard;
}


//?? remove; but: this code allow us to cycle crosshairs (when ch#N absent, crosshair is set to 0)
void SCR_TouchPics()
{
	int ch_num = crosshair->integer;
	if (ch_num)
	{
		if (ch_num > 0)
		{
			CrosshairPic.sprintf("pics/ch%d", crosshair->integer);
			if (!RE_RegisterPic(CrosshairPic))
				ch_num = -1;								// invalid value
		}
		if (ch_num <= 0)
		{
			CrosshairPic[0] = 0;
			if (ch_num < 0) Cvar_Set("crosshair", "0");	// invalid value becomes zero
		}
	}
}


// This is called every frame, and can also be called explicitly to flush text to the screen.

//#define PROFILE_UPD_SCR		1

#if PROFILE_UPD_SCR
#define PRF(x)	x
#else
#define PRF(x)
#endif

void SCR_UpdateScreen()
{
	guard(SCR_UpdateScreen);

	if (!initialized) return;		// not initialized yet

#if PROFILE_UPD_SCR
	unsigned beforeGUI = 0, afterGUI = 0, beforeHUD = 0;
	unsigned beforeBegin = appCycles();
#endif

	RE_BeginFrame(cls.realtime / 1000.0);

	PRF(unsigned afterBegin = appCycles());

	if (cl.cinematicActive)
	{
		if (!SCR_DrawCinematic())
			RE_Fill(0, 0, viddef.width, viddef.height, RGB(0,0,0));
		DrawGUI(false);
	}
	else
	{
		if (V_RenderView())
		{
			//------------------- HUD --------------------
			if (cl_draw2d->integer)
			{
				PRF(beforeHUD = appCycles());
				DrawCrosshair();
				// SCR_DrawStats:
				ExecuteLayoutString(cl.configstrings[CS_STATUSBAR]);
				// SCR_DrawLayout:
				if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 1)
					ExecuteLayoutString(cl.layout);
				// draw inventory
				if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 2)
					DrawInventory();

				// show disconnected icon when server is not responding
				if (cl.overtime > 200)
					RE_DrawPic(64, 0, "pics/net");

				DrawCenterString();

				// draw pause
				if (cl_paused->integer)
					RE_DrawPic(viddef.width / 2, viddef.height / 2, "pics/pause", ANCHOR_CENTER);

				DrawChatInput();
			}
		}
		else
		{
			// 3D not rendered - draw background
			if (!cls.loading || !map_levelshot)
				RE_DrawDetailedPic(0, 0, viddef.width, viddef.height, "pics/conback");
			if (cls.state == ca_disconnected && !cls.loading)
				M_ForceMenuOn();
		}

		PRF(beforeGUI = appCycles());
		DrawGUI(true);
		PRF(afterGUI = appCycles());

		if (timegraph->integer)
			SCR_DebugGraph(cls.frametime * 300, 0);
		if (debuggraph->integer || timegraph->integer || netgraph->integer)
			DrawDebugGraph();
	}

	PRF(unsigned beforeEnd = appCycles());
	RE_EndFrame();
#if PROFILE_UPD_SCR
	unsigned afterEnd = appCycles();
	RE_DrawTextLeft(va("SCR: begin: %5.2f 3Dview: %5.2f hud: %5.2f gui: %5.2f mid: %5.2f end: %5.2f",
		appCyclesToMsecf(afterBegin - beforeBegin), appCyclesToMsecf(beforeHUD - afterBegin),
		appCyclesToMsecf(beforeGUI - beforeHUD), appCyclesToMsecf(afterGUI - beforeGUI),
		appCyclesToMsecf(beforeEnd - afterGUI), appCyclesToMsecf(afterEnd - beforeEnd)), RGB(0.2,1,0.2));
#endif

	unguard;
}


void SCR_Init()
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

	Cvar_GetVars(ARRAY_ARG(vars));

	RegisterCommand("timerefresh", TimeRefresh_f);
	RegisterCommand("toggleconsole", SCR_ToggleConsole);
	RegisterCommand("messagemode", MessageMode_f);
	RegisterCommand("messagemode2", MessageMode2_f);
	RegisterCommand("screenshot", Screenshot_f);

	initialized = true;
}
