#include "client.h"

#define	NUM_CON_TIMES 6

#define	CON_TEXTSIZE 32768

//?? move consts outside; may be, make a variable; should be synched with renderer char sizes
#define CHAR_WIDTH	8
#define CHAR_HEIGHT	8


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
#define		WRAP_CHAR	(char)(' ' + 128)


void Con_ToggleConsole_f (void)
{
	if (cls.keep_console) return;

	Key_ClearTyping ();
	Con_ClearNotify ();
	SCR_ShowConsole (cls.key_dest != key_console, false);
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
static void Con_Dump_f (bool usage, int argc, char **argv)
{
	int		pos, size, out;
	char	c;
	FILE	*f;
	char	buffer[1024], name[MAX_OSPATH];

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: condump <filename>\n");
		return;
	}

	Com_sprintf (ARRAY_ARG(name), "%s/%s.txt", FS_Gamedir(), argv[1]);

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

	width = (viddef.width / CHAR_WIDTH) - 2;
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

	color = C_WHITE;

	while (c = *txt++)
	{
		if (c == COLOR_ESCAPE)
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
================
DrawInput
================
*/
static void DrawInput (void)
{
	int		y, i, shift;
	char	*text, c, color;
	bool	eoln;

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
		y = con.vislines - CHAR_HEIGHT - 14;
	else
		y = (viddef.height / CHAR_HEIGHT) - 2;

	eoln = false;
	for (i = 0; i < linewidth; i++, text++)
	{
		int x = (i+1) * CHAR_WIDTH;
		if (!eoln)
		{
			c = *text;
			if (!c) eoln = true;
		}
		if (eoln) c = ' ';
		// draw blinking cursor
		if ((i == editPos  - shift) && (curtime >> 8) & 1 && cls.key_dest != key_menu)
		{
			if (!(*re.flags & REF_CONSOLE_ONLY))
			{
				re.DrawChar (x, y, c);
				re.DrawFill2 (x, y+CHAR_HEIGHT/2, CHAR_WIDTH, CHAR_HEIGHT/2, RGBA(0.2,1,0.3,0.5));
				continue;
			}
			c = 11;									// cursor char
			color = C_GREEN;
		}
		else
			color = C_WHITE;						// do not colorize input
		// draw char
		if (!(*re.flags & REF_CONSOLE_ONLY))
			re.DrawChar (x, y, c, color);
		else
			re.DrawConChar (i+1, y, c, color);
	}
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (bool drawBack)
{
	int		x, v, i, pos;
	char	*s;

	if (*re.flags & REF_CONSOLE_ONLY || !cl_draw2d->integer)
		return;

	v = 0;
	pos = -1;
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

		if (drawBack)
		{
			re.DrawFill2 (0, 0, viddef.width * 3 / 4, NUM_CON_TIMES * CHAR_HEIGHT + CHAR_HEIGHT/2, RGBA(1,0,0,0.3));
			drawBack = false;
		}
		for (x = 0; x < linewidth; x++)
		{
			char c = con.text[pos];
			int color = con.text[pos + CON_TEXTSIZE];
			if (++pos >= CON_TEXTSIZE) pos -= CON_TEXTSIZE;

			if (c == '\n' || c == WRAP_CHAR) break;
			re.DrawChar ((x + 1) * CHAR_WIDTH, v, c, color);
		}

		v += CHAR_HEIGHT;
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
		if (chat_bufferlen > viddef.width/CHAR_WIDTH - (x + 1))
			s += chat_bufferlen - (viddef.width/CHAR_WIDTH - (x + 1));
		while(*s)
		{
			re.DrawChar (x * CHAR_WIDTH, v, *s++);
			x++;
		}
		// draw cursor
		re.DrawChar (x * CHAR_WIDTH, v, 10 + ((curtime >> 8) & 1));
	}
}


/*
================
Con_DrawConsole

Draws the console with the solid background
================
*/

//#define DEBUG_CONSOLE

void Con_DrawConsole (float frac)
{
	int		i, j, x, y, y0, topline;
	int		row, rows, lines;
	int		dx, dy, color;
	char	c;
	static const char version[] = APPNAME " v" STR(VERSION);
#ifdef DEBUG_CONSOLE
	char	dbgBuf[256];
	dbgBuf[0] = 0;
#define CON_DBG(x)	strncat(dbgBuf,x,sizeof(dbgBuf))
#else
#define CON_DBG(x)
#endif


	lines = appRound (viddef.height * frac);
	con_height = lines;
	if (lines <= 0) return;

	if (lines > viddef.height)
		lines = viddef.height;

	// draw the background
	if (!(*re.flags & REF_CONSOLE_ONLY))
	{
		re.DrawFill2 (0, 0, viddef.width, lines, RGBA(0,0,0.02,0.5));
		if (lines < viddef.height)
			re.DrawFill2 (0, lines - 1, viddef.width, 1, RGBA(0.2,0.2,0.2,0.8));
	}

	// Variables for console-only mode
	dx = viddef.width / CHAR_WIDTH;
	dy = viddef.height / CHAR_HEIGHT;

	// draw version info
	i = sizeof(version) - 1;
	for (x = 0; x < i; x++)
		if (!(*re.flags & REF_CONSOLE_ONLY))
			re.DrawChar (viddef.width - i*CHAR_WIDTH - CHAR_WIDTH/2 + x*CHAR_WIDTH, lines - 12, version[x], C_GREEN);
		else
			re.DrawConChar (dx - i - 1 + x, dy - 1, version[x], C_GREEN);

	// draw the text
	con.vislines = lines;

	if (!(*re.flags & REF_CONSOLE_ONLY))
	{
		rows = (lines - CHAR_HEIGHT/2) / CHAR_HEIGHT - 2;	// rows of text to draw
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

	row = con.display - rows + 1; // top line to display

	CON_DBG(va(" top:%d row:%d",topline,row));
	if (row < topline)
	{
		// row is out of (before) buffer
		j = topline - row;
		CON_DBG(va(" fix_top:%d",j));
		row = topline;
		rows -= j;
	}

	CON_DBG(va(" 1st:%d/count:%d/y1:%d",row,rows,y));
	i = FindLine (row);

	// cache info
	con.disp.line = row;
	con.disp.pos = i;

	y0 = y;							// last console text line
	if (!(*re.flags & REF_CONSOLE_ONLY))
		y -= (rows - 1) * CHAR_HEIGHT;
	else
		y -= rows - 1;
	if (con.totallines && con.display != con.current)
	{
		/*------ draw arrows to show the buffer is backscrolled -------*/
		if (!(*re.flags & REF_CONSOLE_ONLY))
		{
			for (x = 0; x < linewidth; x += 4)
				re.DrawChar ((x + 1) * CHAR_WIDTH, y0, '^');
		}
		else
		{
			for (x = 0; x < linewidth; x += 4)
				re.DrawConChar (x + 1, y0, '^');
		}
		rows--;
	}

	CON_DBG(va("/y2:%d",y));
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

				if (!con_colorText->integer) color = C_WHITE;
				if (!(*re.flags & REF_CONSOLE_ONLY))
					re.DrawChar ((x + 1) * CHAR_WIDTH, y, c, color);
				else
					re.DrawConChar (x + 1, y, c, color);
			}
			if (!(*re.flags & REF_CONSOLE_ONLY))
				y += CHAR_HEIGHT;
			else
				y++;
		}
	}
	CON_DBG(va(" disp:%d total:%d",con.display,con.totallines));
#ifdef DEBUG_CONSOLE
	i = strlen (dbgBuf);
	for (x = 0; x < i; x++)
		if (!(*re.flags & REF_CONSOLE_ONLY))
			re.DrawChar (x*CHAR_WIDTH, lines - 12, dbgBuf[x], C_BLUE);
		else
			re.DrawConChar (x, dy - 1, dbgBuf[x], C_BLUE);
#endif
	// draw the input prompt, user text, and cursor
	DrawInput ();
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
	case MOD_CTRL+'v':		//?? is it really needed ?
		if (s = Sys_GetClipboardData ())
		{
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

	RegisterCommand ("toggleconsole", Con_ToggleConsole_f);
	RegisterCommand ("messagemode", Con_MessageMode_f);
	RegisterCommand ("messagemode2", Con_MessageMode2_f);
	RegisterCommand ("clear", Con_Clear_f);
	RegisterCommand ("condump", Con_Dump_f);

	con_initialized = true;
}
