#include "client.h"

#define	NUM_CON_TIMES 4

#define	CON_TEXTSIZE 32768


typedef struct
{
	int		line;
	int		pos;
} con_cache_t;

typedef struct
{
	bool	wrapped;		// will be set to true after 1st buffer wrap
	bool	started;		// "false" if was no access to console
	bool	wordwrap;

	char	text[CON_TEXTSIZE * 2];	// first half - text, second - color
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int		totallines;		// total lines in console scrollback

	int		startpos;		// points to begin of text buffer
	int		endpos;			// text will be placed to endpos

	int		vislines;

	con_cache_t	disp;
	con_cache_t	notif;

	int		times[NUM_CON_TIMES];	// cls.realtime time the line was generated
} console_t;


static console_t con;

static cvar_t	*con_notifyTime;
static cvar_t	*con_wordWrap;
static cvar_t	*con_colorText;


static bool		chat_team;
static char		chat_buffer[MAXCMDLINE];
static int		chat_bufferlen = 0;


#define MAX_HISTORY	32

static char history[MAX_HISTORY][MAXCMDLINE];
static int	historyLine, historyCount;

// current line
char	editLine[MAXCMDLINE];
int		editPos;


static bool con_initialized;
int		con_height;

// WRAP_CHAR will be placed as "soft" line-feed instead of a space char
#define		WRAP_CHAR	(' ' + 32)


// Functions DrawString used for painting HUD
void DrawStringCommon (int x, int y, const char *s)
{
	int color, c;

	color = 7;
	while (c = *s++)
	{
		if (c == '^')
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
		if (color != 7 && con_colorText->integer)
			re.DrawCharColor (x, y, c, color);
		else
			re.DrawCharColor (x, y, c, 7);
		x += 8;
	}
}


// DrawString() used by HUD routines
void DrawString (int x, int y, const char *s)
{
	if (!cl_draw2d->integer)
		return;
	DrawStringCommon (x, y, s);
}


void Con_ToggleConsole_f (void)
{
	if (cls.keep_console) return;

	Key_ClearTyping ();
	Con_ClearNotify ();
	SCR_ShowConsole ((bool)(cls.key_dest != key_console), false);
}


/*
================
Con_Clear_f
================
*/
static void Con_Clear_f (void)
{
	con.totallines = 1;		// current line, even if empty, encounted
	con.current = con.display = 0;
	con.x = 0;

	con.startpos = con.endpos = 0;
	con.wrapped = false;
	con.text[0] = '\n';		// mark current line end (for correct display)

	if (!con.started)
	{
		linewidth = 38;
		con.started = true;
	}
	// clear line position cache
	con.disp.line = con.notif.line = -1;
	Con_ClearNotify ();
}


/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
static void Con_Dump_f (void)
{
	int		pos, size, out;
	char	c;
	FILE	*f;
	char	buffer[1024], name[MAX_OSPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: condump <filename>\n");
		return;
	}

	Com_sprintf (ARRAY_ARG(name), "%s/%s.txt", FS_Gamedir(), Cmd_Argv(1));

	FS_CreatePath (name);
	f = fopen (name, "w");
	if (!f)
	{
		Com_WPrintf ("ERROR: couldn't open %s\n", name);
		return;
	}

	pos = con.startpos;
	size = con.endpos - pos;
	if (size < 0) size += CON_TEXTSIZE;

	out = 0;
	while (size)
	{
		c = con.text[pos++];
		if (pos >= CON_TEXTSIZE) pos -= CON_TEXTSIZE;
		size--;

		if (c == WRAP_CHAR) c = ' ';	// unwrap words
		c &= 0x7F;						// reset color bit (??)
		buffer[out++] = c;
		if (c == '\n' || out >= sizeof(buffer) - 1 || !size)
		{
			buffer[out] = 0;			// ASCIIZ
			out = 0;
			fprintf (f, "%s", buffer);
		}
	}
	fclose (f);
	Com_Printf ("Console text was dumped to %s\n", name);
}


/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify (void)
{
	memset (con.times, 0, sizeof(con.times));
}


/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
{
	int		width, size, line, i, x;
	char	c;
	bool	w;

	w = (con_wordWrap && con_wordWrap->integer);
	if (w != con.wordwrap)
	{
		linewidth = -1;					// force resize
		con.wordwrap = con_wordWrap->integer != 0;
	}

	width = (viddef.width >> 3) - 2;
	if (width < 1) width = 38;			// wideo hash't initialized

	if (width == linewidth)
		return;

	linewidth = width;

	size = con.endpos - con.startpos;	// size of data in buffer
	if (size < 0) size+= CON_TEXTSIZE;	// wrap buffer: endpos < startpos

	i = con.startpos;
	x = 0;
	line = 0;
	while (size--)
	{
		c = con.text[i];
		if (c == WRAP_CHAR) c = ' ';	// ignore old WRAP_CHARs
		con.text[i] = c;
		if (++i >= CON_TEXTSIZE) i -= CON_TEXTSIZE;

		if (c == '\n' || ++x >= width)	// line feed or line wrap
		{
			x = 0;
			line++;

			if (con.wordwrap && c != '\n')
			{
				// make a word wrap
				char c1;
				int i1, x1;

				i1 = i;					// seek back to find a space char
				x1 = -1;
				while (++x1 < width)
				{
					if (--i1 < 0) i1 += CON_TEXTSIZE;
					c1 = con.text[i1];

					if (c1 == '\n' || c1 == WRAP_CHAR) break; // wrap found - word is too long
					if (c1 == ' ')
					{
						con.text[i1] = WRAP_CHAR;
						x = x1;
						break;
					}
				}
			}
		}
	}
	con.totallines = line + 1;
	con.current = line;
	con.display = line;
	// clear cache
	con.disp.line = con.notif.line = -1;
}


static int FindLine (int lineno)
{
	int		i, size, line, x;

	// try to get line info from cache
	if (lineno == con.disp.line)
		return con.disp.pos;
	else if (lineno == con.notif.line)
		return con.notif.pos;

	if (lineno == con.current)
	{
		i = con.endpos - con.x;
		if (i < 0) i += CON_TEXTSIZE;
		return i;
	}

	line = con.current - con.totallines + 1;	// number of 1st line in buffer
	if (lineno < line || lineno > con.current) return -1; // this line is out of buffer
	if (lineno == line) return con.startpos;	// first line in buffer

	size = con.endpos - con.startpos;	// number of bytes in buffer
	if (size < 0) size += CON_TEXTSIZE;	// wrap buffer: endpos < startpos

	if (!size) return -1;				// no text in buffer

	i = con.startpos;
	x = 0;
	while (size--)
	{
		char	c;

		c = con.text[i++];
		if (i >= CON_TEXTSIZE) i -= CON_TEXTSIZE;

		if (c == '\n' || c == WRAP_CHAR || ++x >= linewidth)
		{
			x = 0;
			line++;
			if (line == lineno) return i;
		}
	}
	return -1; // should not happen
}


static void PlaceChar (char c, byte color)
{
	int		size, i, x;

	// calculate free space in buffer
	size = con.startpos - con.endpos;
	if (size < 0) size += CON_TEXTSIZE;	// size of free space in buffer

	if (!size && !con.wrapped) size = CON_TEXTSIZE;
	if (size <= 2)
	{	// kill first line in buffer
		char	t;

		x = 0;
		while (true)
		{
			t = con.text[con.startpos++];
			if (con.startpos >= CON_TEXTSIZE) con.startpos -= CON_TEXTSIZE;

			if (t == '\n' || t == WRAP_CHAR || ++x >= linewidth) break; // killed
		}
		con.totallines--;
	}

	if (c == '\r')
	{
		con.endpos = FindLine (con.current);
		con.times[con.current % NUM_CON_TIMES] = cls.realtime;
		con.x = 0;
		return;
	}

	// mark time for transparent overlay
	con.times[con.current % NUM_CON_TIMES] = cls.realtime;

	con.text[con.endpos] = c;
	con.text[con.endpos + CON_TEXTSIZE] = color;
	if (++con.endpos >= CON_TEXTSIZE)
	{
		con.endpos -= CON_TEXTSIZE;
		con.wrapped = true;
	}
	con.text[con.endpos] = '\n';	// mark (temporary) end of line

	if (c == '\n' || ++con.x >= linewidth)
	{	// new line
		con.x = 0;
		if (con.wordwrap && c != '\n')
		{	// make a word wrap
			i = con.endpos;	// seek back to find space
			x = -1;
			while (++x < linewidth)
			{
				if (--i < 0) i += CON_TEXTSIZE;
				c = con.text[i];

				if (c == '\n' || c == WRAP_CHAR) break; // wrap found - word is too long
				if (c == ' ')
				{
					con.text[i] = WRAP_CHAR;
					con.x = x;
					break;
				}
			}
		}
		if (con.display == con.current)
			con.display++;
		con.current++;
		con.totallines++;
	}
}


/*
================
Con_Print
================
*/
void Con_Print (char *txt)
{
	byte	color;
	char	c;

	if (!con.started) Con_Clear_f ();

	color = 7;

	while (c = *txt++)
	{
		if (c == '^')
		{
			int		col;

			col = *txt - '0';
			if (col >= 0 && col <= 7)
			{
				color = col;
				txt++;
				continue;
			}
		}
		if (c == '\r' && *txt == '\n')					// handle CR/LF correctly
		{
			c = '\n';
			txt++;
		}
		else if (c == WRAP_CHAR)
			c = ' ';									// force WRAP_CHAR (or'ed space) to be a space

		PlaceChar (c, color);
	}
}


/*
==============
Con_CenteredPrint
==============
*/
void Con_CenteredPrint (char *text)
{
	int		l;
	char	buffer[1024];

	l = strlen (text);
	l = (linewidth - l) / 2;
	if (l < 0)
		l = 0;
	memset (buffer, ' ', l);
	strcpy (buffer + l, text);
	strcat (buffer, "\n");
	Con_Print (buffer);
}


/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
static void Con_DrawInput (void)
{
	int		y, i, shift;
	char	*text, c;
	bool	eoln;

//	if (cls.key_dest == key_menu) return;
//	if (cls.key_dest != key_console && cls.state == ca_active)
//		return;		// don't draw anything (always draw if not active)

	text = editLine;

	// prestep if horizontally scrolling
	if (editPos >= linewidth)
	{
		shift = 1 + editPos - linewidth;
		text += shift;
	}
	else
		shift = 0;

	// draw it
	if (!(*re.flags & REF_CONSOLE_ONLY))
		y = con.vislines - 22;
	else
		y = (viddef.height >> 3) - 2;

	eoln = false;
	for (i = 0; i < linewidth; i++, text++)
	{
		if (!eoln)
		{
			c = *text;
			if (!c) eoln = true;
		}
		if (eoln) c = ' ';
		if ((i == editPos  - shift) && (curtime >> 8) & 1 &&
//??			(cls.key_dest == key_console || cls.state != ca_active))
			cls.key_dest != key_menu)
		{
			if (!(*re.flags & REF_CONSOLE_ONLY))
			{
				re.DrawCharColor ((i+1)<<3, y, c, 7);
				re.DrawFill2 ((i+1)<<3, y+4, 8, 4, 0.2, 1, 0.3, 0.5);
				continue;
			}
			c = 11 + 128;								// cursor char
		}

		if (!(*re.flags & REF_CONSOLE_ONLY))
			re.DrawCharColor ((i+1)<<3, y, c, 7);		// do not colorize input
		else
			re.DrawConCharColor (i+1, y, c, 7);
	}
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (qboolean drawBack)
{
	int		x, v, i, pos;
	char	*s;
	bool	bgPaint;

	if (*re.flags & REF_CONSOLE_ONLY || !cl_draw2d->integer)
		return;

	v = 0;
	pos = -1;
	bgPaint = !drawBack;
	for (i = con.current - NUM_CON_TIMES + 1; i <= con.current; i++)
	{
		int		time;

		if (i < 0) continue;

		time = con.times[i % NUM_CON_TIMES];
		if (time == 0) continue;

		time = cls.realtime - time;
		if (time > con_notifyTime->value * 1000) continue;

		if (pos == -1)
		{
			pos = FindLine (i);		// else (pos!=-1) - already searched on previous loop
			// cache info
			con.notif.line = i;
			con.notif.pos = pos;
		}

		if (pos == -1) continue;	// should not happen

		if (!bgPaint)
		{
			re.DrawFill2 (0, 0, viddef.width * 3 / 4, NUM_CON_TIMES * 8 + 4, 1, 0, 0, 0.3);
			bgPaint = true;
		}
		for (x = 0; x < linewidth; x++)
		{
			int		color;
			char	c;

			c = con.text[pos];
			color = con.text[pos + CON_TEXTSIZE];
			if (++pos >= CON_TEXTSIZE) pos -= CON_TEXTSIZE;

			if (c == '\n' || c == WRAP_CHAR) break;
			re_DrawCharColor ((x + 1) << 3, v, c, color);
		}

		v += 8;
	}


	if (cls.key_dest == key_message)	// edit current player message
	{
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

		s = chat_buffer;
		if (chat_bufferlen > (viddef.width >> 3) - (x + 1))
			s += chat_bufferlen - ((viddef.width >> 3) - (x + 1));
		while(*s)
		{
			re_DrawCharColor (x << 3, v, *s++, 7);
			x++;
		}
		// draw cursor
		re_DrawCharColor (x << 3, v, 10 + ((curtime >> 8) & 1), 7);
		v += 8;
	}
}


/*
================
Con_DrawConsole

Draws the console with the solid background
================
*/
void Con_DrawConsole (float frac)
{
	int		i, j, x, y, n, topline;
	int		row, rows, lines;
	int		dx, dy, color;
	char	*text, c, dlbar[1024];
	static const char version[] = APPNAME " v" STR(VERSION);

	lines = Q_round (viddef.height * frac);
	con_height = lines;
	if (lines <= 0) return;

	if (lines > viddef.height)
		lines = viddef.height;

	// draw the background
	if (!(*re.flags & REF_CONSOLE_ONLY))
	{
		re.DrawFill2 (0, 0, viddef.width, lines, 0, 0, 0.02, 0.5);
		if (lines < viddef.height)
			re.DrawFill2 (0, lines - 1, viddef.width, 1, 0.2, 0.2, 0.2, 0.8);
	}

	// Variables for console-only mode
	dx = viddef.width >> 3;
	dy = viddef.height >> 3;

	// draw version info
	i = sizeof(version) - 1;
	for (x = 0; x < i; x++)
		if (!(*re.flags & REF_CONSOLE_ONLY))
			re.DrawCharColor (viddef.width - i * 8 - 4 + x * 8, lines - 12, 128 + version[x], 7);
		else
			re.DrawConCharColor (dx - i - 1 + x, dy - 1, 128 + version[x], 7);

	// draw the text
	con.vislines = lines;

	if (!(*re.flags & REF_CONSOLE_ONLY))
	{
		rows = (lines - 22) >> 3;		// rows of text to draw
		y = lines - 30;
	}
	else
	{
		rows = dy - 2;
		y = rows - 1;
	}

	topline = con.current - con.totallines + 1; // number of top line in buffer

	// fix con.display if out of buffer
	con.display = bound(con.display, topline, con.current);

	if (con.totallines && con.display != con.current)
	{
		/*------ draw arrows to show the buffer is backscrolled -------*/
		if (!(*re.flags & REF_CONSOLE_ONLY))
		{
			y = (rows - 1) * 8;
			for (x = 0; x < linewidth; x += 4)
				re.DrawCharColor ((x + 1) * 8, y, '^', 7);
			y -= 8;
		}
		else
		{
			y = rows - 1;
			for (x = 0; x < linewidth; x += 4)
				re.DrawConCharColor (x + 1, y, '^', 7);
			y--;
		}
		rows--;
	}

	row = con.display - rows + 1; // top line to display

	if (row < topline)
	{
		// row is out of (before) buffer
		j = topline - row;
		row = topline;
		rows -= j;
	}

	i = FindLine (row);

	// cache info
	con.disp.line = row;
	con.disp.pos = i;

	if (!(*re.flags & REF_CONSOLE_ONLY))
		y -= (rows - 1) * 8;
	else
		y -= rows - 1;
	if (rows > 0 && i != -1)
	{
		while (rows--)
		{
			for (x = 0; x < linewidth; x++)
			{
				c = con.text[i];
				color = con.text[i + CON_TEXTSIZE];
				if (++i >= CON_TEXTSIZE) i -= CON_TEXTSIZE;

				if (c == '\n' || c == WRAP_CHAR) break;

				if (!con_colorText->integer) color = 7;
				if (!(*re.flags & REF_CONSOLE_ONLY))
					re.DrawCharColor ((x + 1) * 8, y, c, color);
				else
					re.DrawConCharColor (x + 1, y, c, color);
			}
			if (!(*re.flags & REF_CONSOLE_ONLY))
				y += 8;
			else
				y++;
		}
	}

	//ZOID
	// draw the download bar
	// figure out width
	if (!(*re.flags & REF_CONSOLE_ONLY) && cls.download)
	{
		if ((text = strrchr (cls.downloadname, '/')) != NULL)
			text++;
		else
			text = cls.downloadname;

		x = linewidth - ((linewidth * 7) / 40);
		y = x - strlen (text) - 8;
		i = linewidth / 3;
		if (strlen (text) > i)
		{
			y = x - i - 11;
			strncpy (dlbar, text, i);
			dlbar[i] = 0;
			strcat (dlbar, "...");
		} else
			strcpy (dlbar, text);
		strcat (dlbar, ": ");
		i = strlen (dlbar);
		dlbar[i++] = '\x80';
		// where's the dot go?
		if (cls.downloadpercent == 0)
			n = 0;
		else
			n = y * cls.downloadpercent / 100;

		for (j = 0; j < y; j++)
			if (j == n)
				dlbar[i++] = '\x83';
			else
				dlbar[i++] = '\x81';
		dlbar[i++] = '\x82';
		dlbar[i] = 0;

		Com_sprintf (dlbar+i, sizeof(dlbar)-i, " %02d%%", cls.downloadpercent);

		// draw it
		y = con.vislines - 12;
		for (i = 0; i < strlen (dlbar); i++)
			re.DrawCharColor ((i + 1) * 8, y, dlbar[i], 7);
	}
	//ZOID

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ();
}


/*-----------------------------------------------------------------------------
	Keyboard handling
-----------------------------------------------------------------------------*/


static bool iswordsym (char n)
{
	return (n >= '0' && n <= '9') || (n >= 'A' && n <= 'Z') || (n >= 'a' && n <= 'z') || n == '_';
}

void Key_Console (int key, int modKey)
{
	int		i;
	char	*s;

	if (modKey == K_INS && SHIFT_PRESSED)
		modKey = 'v'+MOD_CTRL;			// Shift-Ins -> Ctrl-V (move to keys.c ??)

	switch (modKey)
	{
	case MOD_CTRL+'v':
		if (s = Sys_GetClipboardData ())
		{
//			strtok (s, "\n\r\b");
			i = strlen (s);
			if (i + editPos > MAXCMDLINE - 1)
				i = MAXCMDLINE - editPos - 1;

   			if (i > 0)
			{
				s[i] = 0;
				strcat (editLine, s);
				editPos += i;
			}
			free (s);
		}
		return;

	case MOD_CTRL+'l':
		Cbuf_AddText ("clear\n");
		return;

	case K_ENTER:
	case K_KP_ENTER:
		// trim spaces at line end
/*		for (i = strlen (editLine) - 1; i >= 0; i--)
			if (editLine[i] == ' ')
				editLine[i] = 0;
			else
				break; */
		// backslash text are commands, else chat
		if (editLine[1] == '\\' || editLine[1] == '/')
			Cbuf_AddText (editLine + 2);			// skip the prompt
		else
			Cbuf_AddText (editLine + 1);			// valid command
		Cbuf_AddText ("\n");

		Com_Printf ("%s\n", editLine);
		if (!editLine[1])							// do not add empty line to history
		{
			Key_ClearTyping ();
			return;
		}

		// find the same line in history and remove it
		for (i = 0; i < historyCount; i++)
			if (!strcmp (history[i], editLine))
			{
				if (i != historyCount - 1) memcpy (history[i], history[i+1], sizeof(history[0]) * (historyCount - 1 - i));
				historyCount--;
				break;
			}

		// add current line to history
		if (historyCount < MAX_HISTORY)
			historyCount++;
		else
			memcpy (history[0], history[1], sizeof(history[0]) * (MAX_HISTORY - 1));
		memcpy (history[historyCount-1], editLine, sizeof(editLine));
		historyLine = historyCount;
		// prepare new line
		Key_ClearTyping ();
		if (cls.state == ca_disconnected)
			SCR_UpdateScreen ();					// force an update, because the command may take some time
		return;

	case K_TAB:
		// command completion
		CompleteCommand ();
		return;

	case K_LEFTARROW:
	case K_BACKSPACE:
	case MOD_CTRL+K_LEFTARROW:
	case MOD_CTRL+K_BACKSPACE:
		if (editPos <= 1) return;
		i = editPos;
		if (modKey >= MOD_CTRL)			// CTRL_PRESSED
		{
			while (editPos > 1 && !iswordsym (editLine[--editPos]));
			while (editPos > 0 && iswordsym (editLine[--editPos]));
			editPos++;
		}
		else
			editPos--;

		if (key == K_BACKSPACE)		// not modKey ! (i.e. without modifier)
			strcpy (&editLine[editPos], &editLine[i]);
		return;

	case K_RIGHTARROW:
		if (editLine[editPos]) editPos++;
		return;

	case MOD_CTRL+K_RIGHTARROW:
		while (editLine[editPos] && iswordsym (editLine[editPos])) editPos++;
		while (editLine[editPos] && !iswordsym (editLine[editPos])) editPos++;
		return;

	case K_DEL:
		s = &editLine[editPos];
		strcpy (s, s + 1);
		return;

	case MOD_CTRL+K_DEL:
		s = &editLine[editPos];
		*s = 0;
		return;

	case K_UPARROW:
//	case MOD_CTRL+'p':
		if (!historyLine) return;					// empty or top of history

		strcpy (editLine, history[--historyLine]);
		editPos = strlen (editLine);
		return;

	case K_DOWNARROW:
//	case MOD_CTRL+'n':
		if (historyLine >= historyCount) return;	// bottom of history

		historyLine++;
		if (historyLine == historyCount)
			Key_ClearTyping ();
		else
		{
			strcpy (editLine, history[historyLine]);
			editPos = strlen (editLine);
		}
		return;

	case MOD_CTRL+K_PGUP:
	case K_MWHEELUP:
		con.display -= 8;
		return;

	case MOD_CTRL+K_PGDN:
	case K_MWHEELDOWN:
		con.display += 8;
		return;

	case K_PGUP:
	case MOD_CTRL+K_MWHEELUP:
		con.display -= 2;
		if (con.display < con.current - con.totallines + 10)
			con.display = con.current - con.totallines + 10;
		return;

	case K_PGDN:
	case MOD_CTRL+K_MWHEELDOWN:
		con.display += 2;
		return;

	case K_HOME:
		editPos = 1;
		return;

	case K_END:
		editPos = strlen (editLine);
		return;

	case MOD_CTRL+K_HOME:
		con.display = con.current - con.totallines + 10;
		return;

	case MOD_CTRL+K_END:
		con.display = con.current;
		return;
	}

	if (modKey >= MOD_CTRL)
		return;					// do not allow CTRL/ALT+key to produce chars

	// numpad keys -> ASCII
	switch (key)
	{
	case K_KP_SLASH:
		key = '/';
		break;
	case K_KP_MINUS:
		key = '-';
		break;
	case K_KP_PLUS:
		key = '+';
		break;
	case K_KP_HOME:
		key = '7';
		break;
	case K_KP_UPARROW:
		key = '8';
		break;
	case K_KP_PGUP:
		key = '9';
		break;
	case K_KP_LEFTARROW:
		key = '4';
		break;
	case K_KP_5:
		key = '5';
		break;
	case K_KP_RIGHTARROW:
		key = '6';
		break;
	case K_KP_END:
		key = '1';
		break;
	case K_KP_DOWNARROW:
		key = '2';
		break;
	case K_KP_PGDN:
		key = '3';
		break;
	case K_KP_INS:
		key = '0';
		break;
	case K_KP_DEL:
		key = '.';
		break;
	}

	if (key < 32 || key >= 128)
		return;		// non printable

	if (editPos < MAXCMDLINE - 2)
	{
		for (i = MAXCMDLINE - 1; i > editPos; i--) editLine[i] = editLine[i - 1];
		editLine[editPos++] = key;
	}
}


/*-----------------------------------------------------------------------------
	Chat input
-----------------------------------------------------------------------------*/

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


static void Con_MessageMode_f (void)
{
	chat_team = false;
	cls.key_dest = key_message;
}


static void Con_MessageMode2_f (void)
{
	chat_team = true;
	cls.key_dest = key_message;
}


/*-----------------------------------------------------------------------------
	Initialization
-----------------------------------------------------------------------------*/

void Con_Init (void)
{
CVAR_BEGIN(vars)
	CVAR_VAR(con_notifyTime, 3, 0),
	CVAR_VAR(con_wordWrap, 1, CVAR_ARCHIVE),
	CVAR_VAR(con_colorText, 1, CVAR_ARCHIVE)
CVAR_END

	if (con_initialized) return;

	Cvar_GetVars (ARRAY_ARG(vars));
	linewidth = -1;		// force Con_CheckResize()

	if (!con.started) Con_Clear_f ();

	Con_CheckResize ();

	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand ("clear", Con_Clear_f);
	Cmd_AddCommand ("condump", Con_Dump_f);

	con_initialized = true;
}
