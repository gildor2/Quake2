#include "client.h"


console_t	con;

cvar_t	*con_notifytime;
cvar_t	*con_wordwrap;
cvar_t	*con_colortext;

// WRAP_CHAR will be placed as "soft" line-feed instead of a space char
#define		WRAP_CHAR	(' ' + 32)

#define		MAXCMDLINE	256
extern char key_lines[32][MAXCMDLINE];
extern int edit_line;
extern int key_linepos;


// Functions DrawString/DrawAltString used for painting HUD
void DrawStringCommon (int x, int y, const char *s, int mask)
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
		if (color != 7 && con_colortext->integer)
		{
			re.DrawCharColor (x, y, c, color);	// no mask if text colorized
		}
		else
			re.DrawCharColor (x, y, c ^ mask, 7);
		x += 8;
	}
}


// DrawString() and DrawAltString() used by HUD routines
void DrawString (int x, int y, const char *s)
{
	if (!cl_draw2d->integer)
		return;
	DrawStringCommon (x, y, s, 0);
}


void DrawAltString (int x, int y, const char *s)
{
	if (!cl_draw2d->integer)
		return;
	DrawStringCommon (x, y, s, 0x80);
}


void Key_ClearTyping (void)
{
	key_lines[edit_line][1] = 0;	// clear any typing
	key_linepos = 1;
}


void Con_ToggleConsole_f (void)
{
	SCR_EndLoadingPlaque ();	// get rid of loading plaque

	if (cl.attractloop)
	{
		Cbuf_AddText ("killserver\n");
		return;
	}

	if (cls.state == ca_disconnected)
	{	// start the demo loop again
		Cbuf_AddText ("d1\n");
		return;
	}

	Key_ClearTyping ();
	Con_ClearNotify ();

	if (cls.key_dest == key_console)
	{
		M_ForceMenuOff ();
		Cvar_Set ("paused", "0");
	}
	else
	{
		M_ForceMenuOff ();
		cls.key_dest = key_console;

		if (Cvar_VariableInt ("maxclients") == 1
			&& Com_ServerState ())
			Cvar_Set ("paused", "1");
	}
}

/*
================
Con_ToggleChat_f
================
*/
static void Con_ToggleChat_f (void)
{
	Key_ClearTyping ();

	if (cls.key_dest == key_console)
	{
		if (cls.state == ca_active)
		{
			M_ForceMenuOff ();
			cls.key_dest = key_game;
		}
	}
	else
		cls.key_dest = key_console;

	Con_ClearNotify ();
}

/*
================
Con_Clear_f
================
*/
static void Con_Clear_f (void)
{
	con.totallines = 1;	// current line, even if empty, encounted
	con.current = con.display = 0;

	con.startpos = con.endpos = 0;
	con.wrapped = false;
	con.text[0] = '\n';	// mark current line end (for correct display)

	if (!con.started)
	{
		con.linewidth = 38;
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
		Com_Printf ("usage: condump <filename>\n");
		return;
	}

	Com_sprintf (name, sizeof(name), "%s/%s.txt", FS_Gamedir(), Cmd_Argv(1));

	Com_Printf ("Dumped console text to %s.\n", name);
	FS_CreatePath (name);
	f = fopen (name, "w");
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open.\n");
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
		c &= 0x7F;			// reset color bit
		buffer[out++] = c;
		if (c == '\n' || out >= sizeof(buffer) - 1 || !size)
		{
			buffer[out] = 0;	// ASCIIZ
			out = 0;
			fprintf (f, "%s", buffer);
		}
	}
	fclose (f);
}


/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify (void)
{
	int		i;

	for (i=0 ; i<NUM_CON_TIMES ; i++)
		con.times[i] = 0;
}


/*
================
Con_MessageMode_f
================
*/
static void Con_MessageMode_f (void)
{
	chat_team = false;
	cls.key_dest = key_message;
}

/*
================
Con_MessageMode2_f
================
*/
static void Con_MessageMode2_f (void)
{
	chat_team = true;
	cls.key_dest = key_message;
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
	qboolean w;

	w = (con_wordwrap && con_wordwrap->integer);
	if (w != con.wordwrap)
	{
		con.linewidth = -1; // force resize
		con.wordwrap = w;
	}

	width = (viddef.width >> 3) - 2;
	if (width < 1) width = 38;	// wideo hash't initialized

	if (width == con.linewidth)
		return;

	con.linewidth = width;

	size = con.endpos - con.startpos;	// size of data in buffer
	if (size < 0) size+= CON_TEXTSIZE; // wrap buffer: endpos < startpos

//	if (!size) return;		// no text in buffer

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

				i1 = i;	// seek back to find space
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


/*
================
Con_Init
================
*/
void Con_Init (void)
{
	if (con.initialized) return;

	con.linewidth = -1;		// force Con_CheckResize()

	if (!con.started) Con_Clear_f ();

	Con_CheckResize ();

	Com_Printf ("Console initialized.\n");

	/*-------- register our commands/cvars ----------*/
	con_notifytime = Cvar_Get ("con_notifytime", "3", 0);
	con_wordwrap = Cvar_Get ("con_wordwrap", "1", CVAR_ARCHIVE);
	con_colortext = Cvar_Get ("con_colortext", "1", CVAR_ARCHIVE);

	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand ("togglechat", Con_ToggleChat_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand ("clear", Con_Clear_f);
	Cmd_AddCommand ("condump", Con_Dump_f);

	con.initialized = true;
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
	if (lineno == line) return con.startpos;		// first line in buffer

	size = con.endpos - con.startpos;		// number of bytes in buffer
	if (size < 0) size += CON_TEXTSIZE;	// wrap buffer: endpos < startpos

	if (!size) return -1;			// no text in buffer

	i = con.startpos;
	x = 0;
	while (size--)
	{
		char	c;

		c = con.text[i++];
		if (i >= CON_TEXTSIZE) i -= CON_TEXTSIZE;

		if (c == '\n' || c == WRAP_CHAR || ++x >= con.linewidth)
		{
			x = 0;
			line++;
			if (line == lineno) return i;
		}
	}
	return -1; // should not happen
}


/*
===============
Con_Linefeed
===============
*/

static void PlaceChar (int c, int color)
{
	int		size, i, x;

	// disable coloring if needed
	if (con.initialized && !con_colortext->integer)
		color = 7;

	if (color != 7)
		c &= 127;	// disable mask for colorized text

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

			if (t == '\n' || t == WRAP_CHAR || ++x >= con.linewidth) break; // killed
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

//	if (!con.x)	//?? is it works?
//	{
		// mark time for transparent overlay
		con.times[con.current % NUM_CON_TIMES] = cls.realtime;
//	}

	con.text[con.endpos] = c;
	con.text[con.endpos + CON_TEXTSIZE] = color;
	if (++con.endpos >= CON_TEXTSIZE)
	{
		con.endpos -= CON_TEXTSIZE;
		con.wrapped = true;
	}
	con.text[con.endpos] = '\n';	// mark (temporary) end of line

	if (c == '\n' || ++con.x >= con.linewidth)
	{	// new line
		con.x = 0;
		if (con.wordwrap && c != '\n')
		{	// make a word wrap
			i = con.endpos;	// seek back to find space
			x = -1;
			while (++x < con.linewidth)
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

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void Con_Print (char *txt)
{
	int		mask, c, color;

//	if (!con.initialized) return;
	if (!con.started) Con_Clear_f (); // return

	color = 7;

	if (txt[0] == 1 || txt[0] == 2)
	{
		mask = 128;
		txt++;
	}
	else
		mask = 0;

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
			c = ' '; // force WRAP_CHAR (or'ed space) to be a space
		else if (c != '\r' && c != '\n' && c != ' ')	// do not OR this chars
			c = c|mask|con.ormask;

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

	l = strlen(text);
	l = (con.linewidth-l)/2;
	if (l < 0)
		l = 0;
	memset (buffer, ' ', l);
	strcpy (buffer + l, text);
	strcat (buffer, "\n");
	Con_Print (buffer);
}

/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
static void Con_DrawInput (void)
{
	int		y, i;
	char	*text, c;
	qboolean eoln;

	if (cls.key_dest == key_menu)
		return;
	if (cls.key_dest != key_console && cls.state == ca_active)
		return;		// don't draw anything (always draw if not active)

	text = key_lines[edit_line];

	// prestep if horizontally scrolling
	if (key_linepos >= con.linewidth)
		text += 1 + key_linepos - con.linewidth;

	// draw it
	if (!(re.flags & REF_CONSOLE_ONLY))
		y = con.vislines-22;
	else
		y = (viddef.height >> 3) - 2;

	eoln = false;
	for (i=0 ; i<con.linewidth ; i++)
	{
		if (!eoln)
		{
			c = text[i];
			if (!c) eoln = true;
		}
		if (eoln) c = ' ';
		if (i == key_linepos && (cls.realtime >> 8) & 1)
			c = 11 + 128;		// cursor char

		if (!(re.flags & REF_CONSOLE_ONLY))
			re.DrawCharColor ((i+1)<<3, y, c, 7);	// do not colorize input
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
void Con_DrawNotify (void)
{
	int		x, v, i, pos;
	char	*s;

	if (re.flags & REF_CONSOLE_ONLY)
		return;

	v = 0;
	pos = -1;
	for (i = con.current - NUM_CON_TIMES + 1 ;i <= con.current; i++)
	{
		int		time;

		if (i < 0) continue;

		time = con.times[i % NUM_CON_TIMES];
		if (time == 0) continue;

		time = cls.realtime - time;
		if (time > con_notifytime->value * 1000) continue;

		if (pos == -1)
		{
			pos = FindLine (i);		// else (pos!=-1) - already searched on previous loop
			// cache info
			con.notif.line = i;
			con.notif.pos = pos;
		}

		if (pos == -1) continue;	// should not happen

		for (x = 0; x < con.linewidth; x++)
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
		int		skip;

		if (chat_team)
		{
			DrawString (8, v, "say_team:");
			skip = 11;
		}
		else
		{
			DrawString (8, v, "say:");
			skip = 5;
		}

		s = chat_buffer;
		if (chat_bufferlen > (viddef.width >> 3) - (skip + 1))
			s += chat_bufferlen - ((viddef.width >> 3) - (skip + 1));
		x = 0;
		while(s[x])
		{
			re_DrawCharColor ((x + skip) << 3, v, s[x], 7);
			x++;
		}
		// draw cursor
		re_DrawCharColor ((x + skip) << 3, v, 10 + ((cls.realtime >> 8) & 1), 7);
		v += 8;
	}

	if (v)
	{
		SCR_AddDirtyPoint (0, 0);
		SCR_AddDirtyPoint (viddef.width-1, v);
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
	char	*text, c, version[64], dlbar[1024];

	lines = viddef.height * frac;
	if (lines <= 0)
		return;

	if (lines > viddef.height)
		lines = viddef.height;

	/*---------- draw the background ---------------*/
//	re.DrawStretchPic (0, -viddef.height+lines, viddef.width, viddef.height, "conback");
	re.DrawStretchPic (0, 0, viddef.width, lines, "conback");
	SCR_AddDirtyPoint (0, 0);
	SCR_AddDirtyPoint (viddef.width - 1, lines - 1);

	// Variables for console-only mode
	dx = viddef.width >> 3;
	dy = viddef.height >> 3;

	/*------------ draw version info ---------------*/
	Com_sprintf (version, sizeof(version), "v%4.2f", VERSION);
	for (x = 0; x < 5; x++)
		if (!(re.flags & REF_CONSOLE_ONLY))
			re.DrawCharColor (viddef.width - 44 + x * 8, lines - 12, 128 + version[x], 7);
		else
			re.DrawConCharColor (dx - 6 + x, dy - 1, 128 + version[x], 7);

	// draw the text
	con.vislines = lines;

	if (!(re.flags & REF_CONSOLE_ONLY))
	{
#if 0
		rows = (lines - 8) >> 3;		// rows of text to draw
		y = lines - 24;
#else
		rows = (lines - 22) >> 3;		// rows of text to draw
		y = lines - 30;
#endif
	}
	else
	{
		rows = dy - 2;
		y = rows - 1;
	}

	topline = con.current - con.totallines + 1; // number of top line in buffer

	/*----------- fix con.display if out of buffer -------------*/
	if (con.display < topline)
		con.display = topline;
	else if (con.display > con.current)
		con.display = con.current;

	if (con.totallines && con.display != con.current)
	{
		/*------ draw arrows to show the buffer is backscrolled -------*/
		if (!(re.flags & REF_CONSOLE_ONLY))
		{
			y = (rows - 1) * 8;
			for (x = 0; x < con.linewidth; x += 4)
				re.DrawCharColor ((x + 1) * 8, y, '^', 7);
			y -= 8;
		}
		else
		{
			y = rows - 1;
			for (x = 0; x < con.linewidth; x += 4)
				re.DrawConCharColor (x + 1, y, '^', 7);
			y--;
		}
		rows--;
	}

//	if (rows > con.totallines)
//	{	// buffer have a text with size less than a page
//		rows = con.totallines;
//		row = topline; // top line to display
//	}
//	else
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

	if (!(re.flags & REF_CONSOLE_ONLY))
		y -= (rows - 1) * 8;
	else
		y -= rows - 1;
	if (rows > 0 && i != -1)
	{
		while (rows--)
		{
			for (x = 0; x < con.linewidth; x++)
			{
				c = con.text[i];
				color = con.text[i + CON_TEXTSIZE];
				if (++i >= CON_TEXTSIZE) i -= CON_TEXTSIZE;

				if (c == '\n' || c == WRAP_CHAR) break;

				if (!(re.flags & REF_CONSOLE_ONLY))
					re.DrawCharColor ((x + 1) * 8, y, c, color);
				else
					re.DrawConCharColor (x + 1, y, c, color);
			}
			if (!(re.flags & REF_CONSOLE_ONLY))
				y += 8;
			else
				y++;
		}
	}

	//ZOID
	// draw the download bar
	// figure out width
	if (!(re.flags & REF_CONSOLE_ONLY) && cls.download)
	{
		if ((text = strrchr (cls.downloadname, '/')) != NULL)
			text++;
		else
			text = cls.downloadname;

		x = con.linewidth - ((con.linewidth * 7) / 40);
		y = x - strlen (text) - 8;
		i = con.linewidth / 3;
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

		Com_sprintf (dlbar + strlen (dlbar), sizeof(dlbar), " %02d%%", cls.downloadpercent);

		// draw it
		y = con.vislines - 12;
		for (i = 0; i < strlen (dlbar); i++)
			re.DrawCharColor ((i + 1) * 8, y, dlbar[i], 7);
	}
	//ZOID

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ();
}
