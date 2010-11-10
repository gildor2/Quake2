#include "client.h"
#include "OutputDeviceFile.h"

/*?? TODO:
 *	- console: smooth scroll with adaptive speed (different keys scrolls by different line counts)
 *	- notify: smooth scroll, smooth fade of background when notify area disappears; use separate special logger
 *	  for notify messages, which will contain in memory few last lines, then remove notify support from this file ...
 *	- DrawInput()+chat input -> separate ... (not console stuff!)
 */

#define	NUM_CON_TIMES 6

#define	CON_TEXTSIZE 32768


struct console_t
{
	bool	wrapped;		// will be set to true after 1st buffer wrap
	bool	started;		// "false" if was no access to console
	bool	wordwrap;

	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int		totallines;		// total lines in console scrollback

	int		startpos;		// points to begin of text buffer
	int		endpos;			// text will be placed to endpos

	int		vislines;

	struct {
		int line, pos;
	} disp, notif;

	int		times[NUM_CON_TIMES];	// cls.realtime time the line was generated
	char	text[CON_TEXTSIZE * 2];	// first half - text, second - color
};


static console_t con;

static cvar_t	*con_notifyTime;
static cvar_t	*con_wordWrap;
static cvar_t	*con_colorText;


#define MAX_HISTORY	32

static char history[MAX_HISTORY][MAXCMDLINE];
static int	historyLine, historyCount;

// current line
//?? public vars for command completion and for dedicated server
char	editLine[MAXCMDLINE];
int		editPos;


int		con_height;

// WRAP_CHAR will be placed as "soft" line-feed instead of a space char
#define		WRAP_CHAR	(char)(' ' + 128)


inline CBasicFont *GetConsoleFont()
{
	return RE_RegisterFont("console");
}


static void Con_Clear_f()
{
	con.totallines = 1;		// current line, even if empty, encounted
	con.current = con.display = 0;
	con.x = 0;

	con.startpos = con.endpos = 0;
	con.wrapped = false;
	con.text[0] = '\n';		// mark current line end (for correct display)

	if (!con.started)
	{
		GScreenWidth = 38;
		con.started = true;
	}
	// clear line position cache
	con.disp.line = con.notif.line = -1;
	Con_ClearNotify();
}


static void Con_Dump_f(bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		appPrintf("Usage: condump <filename>\n");
		return;
	}

	TString<MAX_OSPATH> Name;
	Name.sprintf("./%s/%s.txt", FS_Gamedir(), argv[1]);

	COutputDeviceFile Out(Name, true);
	if (!Out.IsOpened()) return;

	int pos = con.startpos;
	int size = con.endpos - pos;
	if (size < 0) size += CON_TEXTSIZE;

	char buffer[1024];
	int out = 0;
	while (size)
	{
		char c = con.text[pos++];
		if (pos >= CON_TEXTSIZE) pos -= CON_TEXTSIZE;
		size--;

		if (c == WRAP_CHAR) c = ' ';	// unwrap words
		c &= 0x7F;						// reset color bit (??)
		buffer[out++] = c;
		if (c == '\n' || out >= sizeof(buffer) - 1 || !size)
		{
			buffer[out] = 0;			// ASCIIZ
			out = 0;
			Out.Write(buffer);
		}
	}
	appPrintf("Console text was dumped to %s\n", *Name);
}


void Con_ClearNotify()
{
	memset(con.times, 0, sizeof(con.times));
}


void Con_CheckResize()
{
	bool w = (con_wordWrap && con_wordWrap->integer);
	if (w != con.wordwrap)
	{
		GScreenWidth = -1;				// force resize
		con.wordwrap = con_wordWrap->integer != 0;
	}

	CBasicFont *font = GetConsoleFont();

	int width = (viddef.width / font->charWidth) - 2;
	if (width < 1) width = 38;			// wideo hash't initialized

	if (width == GScreenWidth)
		return;

	GScreenWidth = width;

	int size = con.endpos - con.startpos; // size of data in buffer
	if (size < 0) size+= CON_TEXTSIZE;	// wrap buffer: endpos < startpos

	int i = con.startpos;
	int x = 0;
	int line = 0;
	while (size--)
	{
		char c = con.text[i];
		if (c == WRAP_CHAR) c = ' ';				// ignore old WRAP_CHARs
		con.text[i] = c;
		if (++i >= CON_TEXTSIZE) i -= CON_TEXTSIZE;

		if (c != '\n' && ++x < width) continue;		// no line feed or line wrap

		x = 0;
		line++;

		if (!con.wordwrap || c == '\n') continue;	// no word wrap here

		// make a word wrap
		int i1 = i;									// seek back to find a space char
		int x1 = -1;
		while (++x1 < width)
		{
			if (--i1 < 0) i1 += CON_TEXTSIZE;
			char c1 = con.text[i1];

			if (c1 == '\n' || c1 == WRAP_CHAR) break; // wrap found - word is too long
			if (c1 == ' ')
			{
				con.text[i1] = WRAP_CHAR;
				x = x1;
				break;
			}
		}
	}
	con.totallines = line + 1;
	con.current = line;
	con.display = line;
	// clear cache
	con.disp.line = con.notif.line = -1;
}


/*-----------------------------------------------------------------------------
	Printing text to console
-----------------------------------------------------------------------------*/

static int FindLine(int lineno)
{
	// try to get line info from cache
	if (lineno == con.disp.line)
		return con.disp.pos;
	else if (lineno == con.notif.line)
		return con.notif.pos;

	if (lineno == con.current)
	{
		int i = con.endpos - con.x;
		if (i < 0) i += CON_TEXTSIZE;
		return i;
	}

	int line = con.current - con.totallines + 1; // number of 1st line in buffer
	if (lineno < line || lineno > con.current) return -1; // this line is out of buffer
	if (lineno == line) return con.startpos;	// first line in buffer

	int size = con.endpos - con.startpos;		// number of bytes in buffer
	if (size < 0) size += CON_TEXTSIZE;			// wrap buffer: endpos < startpos

	if (!size) return -1;						// no text in buffer

	int pos = con.startpos;
	int x = 0;
	while (size--)
	{
		char c = con.text[pos++];
		if (pos >= CON_TEXTSIZE) pos -= CON_TEXTSIZE;

		if (c == '\n' || c == WRAP_CHAR || ++x >= GScreenWidth)
		{
			x = 0;
			line++;
			if (line == lineno) return pos;
		}
	}
	return -1; // should not happen
}


static void PlaceChar(char c, byte color)
{
	// calculate free space in buffer
	int size = con.startpos - con.endpos;
	if (size < 0) size += CON_TEXTSIZE;					// size of free space in buffer

	if (!size && !con.wrapped) size = CON_TEXTSIZE;
	if (size <= 2)
	{
		// kill first line in buffer
		int x = 0;
		while (true)
		{
			char t = con.text[con.startpos++];
			if (con.startpos >= CON_TEXTSIZE) con.startpos -= CON_TEXTSIZE;

			if (t == '\n' || t == WRAP_CHAR || ++x >= GScreenWidth) break; // killed
		}
		con.totallines--;
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
	con.text[con.endpos] = '\n';						// mark (temporary) end of line

	if (c == '\n' || ++con.x >= GScreenWidth)
	{	// new line
		con.x = 0;
		if (con.wordwrap && c != '\n')
		{	// make a word wrap
			int i = con.endpos;							// seek back to find space
			int x = -1;
			while (++x < GScreenWidth)
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


void COutputDeviceCon::Write(const char *str)
{
	if (!con.started) Con_Clear_f();

	byte color = C_WHITE;
	while (char c = *str++)
	{
		if (c == COLOR_ESCAPE)
		{
			int col = *str - '0';
			if (col >= 0 && col <= 7)
			{
				color = col;
				str++;
				continue;
			}
		}
		if (c == '\r' && *str == '\n')					// handle CR+LF correctly
		{
			c = '\n';
			str++;
		}
		else if (c == WRAP_CHAR)
			c = ' ';									// force WRAP_CHAR (== space|0x80) to be a space

		PlaceChar(c, color);
	}
}


/*-----------------------------------------------------------------------------
	Drawing console
-----------------------------------------------------------------------------*/

static void DrawInput()
{
	const char *text = editLine;

	// prestep if horizontally scrolling
	int shift;
	if (editPos >= GScreenWidth)
	{
		shift = 1 + editPos - GScreenWidth;
		text += shift;
	}
	else
		shift = 0;

	CBasicFont *font = GetConsoleFont();

	// draw it
	int y = con.vislines - font->charHeight * 2 + 2;

	bool eoln = false;
	for (int i = 0; i < GScreenWidth; i++, text++)
	{
		char	c;

		int x = (i+1) * font->charWidth;
		if (!eoln)
		{
			c = *text;
			if (!c) eoln = true;
		}
		if (eoln) c = ' ';
		// draw char
		RE_DrawChar(font, x, y, c);
		// draw blinking cursor
		if ((i == editPos  - shift) && (cls.realtime >> 8) & 1 && cls.key_dest == key_console)
		{
			int cursorHeight = font->charHeight / 4;
			if (cursorHeight < 2) cursorHeight = 2;
			RE_Fill(x, y + font->charHeight - cursorHeight, font->charWidth, cursorHeight, RGBA(1,0.2,0.3,0.8));
		}
	}
}


void Con_DrawNotify(bool drawBack)
{
	int v = 0;
	int pos = -1;
	CBasicFont *font = GetConsoleFont();

	for (int i = con.current - NUM_CON_TIMES + 1; i <= con.current; i++)
	{
		if (i < 0) continue;

		int time = con.times[i % NUM_CON_TIMES];
		if (time == 0) continue;

		time = cls.realtime - time;
		if (time > con_notifyTime->value * 1000) continue;

		if (pos == -1)
		{
			pos = FindLine(i);		// else (pos!=-1) - already searched on previous loop
			// cache info
			con.notif.line = i;
			con.notif.pos  = pos;
		}

		if (pos == -1) continue;	// should not happen

		if (drawBack)
		{
#if 0
			RE_Fill(0, 0, viddef.width * 3 / 4, NUM_CON_TIMES * font->charHeight + font->charHeight / 2, RGBA(1,0,0,0.3));
#else
			RE_DrawStretchPic(0, 0, viddef.width * 3 / 4, NUM_CON_TIMES * font->charHeight + font->charHeight / 2, "pics/notify");
#endif
			drawBack = false;
		}
		for (int x = 0; x < GScreenWidth; x++)
		{
			char c = con.text[pos];
			int color = con.text[pos + CON_TEXTSIZE];
			if (++pos >= CON_TEXTSIZE) pos -= CON_TEXTSIZE;

			if (c == '\n' || c == WRAP_CHAR) break;
			RE_DrawChar(font, (x + 1) * font->charWidth, v, c, color);
		}

		v += font->charHeight;
	}
}


//#define DEBUG_CONSOLE		1

void Con_DrawConsole(float frac)
{
	int		i, x;
#if DEBUG_CONSOLE
	TString<256> DbgBuf;
	DbgBuf[0] = 0;
#define CON_DBG(x)	DbgBuf += x
#else
#define CON_DBG(x)
#endif

	int lines = appRound(viddef.height * frac);
	con_height = lines;
	if (lines <= 0) return;

	if (lines > viddef.height)
		lines = viddef.height;

	CBasicFont *font = GetConsoleFont();
	int charWidth  = font->charWidth;
	int charHeight = font->charHeight;

	// draw the background
#if 0
//	RE_Fill(0, 0, viddef.width, lines, RGBA(0,0,0.02,Cvar_VariableValue("con_alpha")));
	RE_Fill(0, 0, viddef.width, lines, RGBA(0,0,0.02,0.75));
#else
	RE_DrawStretchPic(0, 0, viddef.width, lines, "pics/console");
#endif
	if (lines < viddef.height)
		RE_Fill(0, lines - 1, viddef.width, 1, RGBA(0.2,0.2,0.2,0.8));

	// draw version info
#if 1
	TString<64> Version;
	i = Version.sprintf("%s v" STR(VERSION), appPackage());
	for (x = 0; x < i; x++)
		RE_DrawChar(font, viddef.width - i * charWidth - charWidth / 2 + x * charWidth, lines - charHeight - 4, Version[x], C_GREEN);
#else
	static const char version[] = APPNAME " v" STR(VERSION);
	i = sizeof(version) - 1;
	for (x = 0; x < i; x++)
		RE_DrawChar(font, viddef.width - i * charWidth - charWidth / 2 + x * charWidth, lines - charHeight - 4, version[x], C_GREEN);
#endif

	// draw the text
	con.vislines = lines;

	int rows = (lines - charHeight / 2) / charHeight - 2;	// rows of text to draw
	int y = lines - charHeight * 3;

	int topline = con.current - con.totallines + 1;			// number of top line in buffer

	// fix con.display if out of buffer
	if (con.display < topline+10)	con.display = topline+10;
	// when console buffer contains leas than 10 lines, require next line ...
	if (con.display > con.current)	con.display = con.current;

	int row = con.display - rows + 1; // top line to display

	CON_DBG(va(" top:%d row:%d",topline,row));
	if (row < topline)
	{
		// row is out of (before) buffer
		i = topline - row;
		CON_DBG(va(" fix_top:%d",i));
		row = topline;
		rows -= i;
	}

	int y0 = y;												// last console text line
	y -= (rows - 1) * charHeight;
	if (con.totallines && con.display != con.current)
	{
		/*------ draw arrows to show the buffer is backscrolled -------*/
		for (x = 0; x < GScreenWidth; x += 4)
			RE_DrawChar(font, (x + 1) * charWidth, y0, '^');
		rows--;
	}

	CON_DBG(va(" 1st:%d/count:%d/y1:%d",row,rows,y));
	i = FindLine(row);
	// cache info
	con.disp.line = row;
	con.disp.pos = i;
	// draw console text
	CON_DBG(va("/y2:%d",y));
	if (rows > 0 && i != -1)
	{
		while (rows--)
		{
			for (x = 0; x < GScreenWidth; x++)
			{
				char c = con.text[i];
				int color = con.text[i + CON_TEXTSIZE];
				if (++i >= CON_TEXTSIZE) i -= CON_TEXTSIZE;

				if (c == '\n' || c == WRAP_CHAR) break;

				if (!con_colorText->integer) color = C_WHITE;
				RE_DrawChar(font, (x + 1) * charWidth, y, c, color);
			}
			y += charHeight;
		}
	}
	CON_DBG(va(" disp:%d total:%d",con.display,con.totallines));
#if DEBUG_CONSOLE
	i = DbgBuf.len();
	for (x = 0; x < i; x++)
		RE_DrawChar(font, x * charWidth, lines - font->charHeight - 2, DbgBuf[x], C_BLUE);
#endif
	// draw the input prompt, user text, and cursor
	DrawInput();
}


/*-----------------------------------------------------------------------------
	Keyboard handling
-----------------------------------------------------------------------------*/


static bool iswordsym(char n)
{
	return (n >= '0' && n <= '9') || (n >= 'A' && n <= 'Z') || (n >= 'a' && n <= 'z') || n == '_';
}

void Con_ClearTyping()
{
	editLine[0] = ']';
	editLine[1] = 0;
	editPos = 1;
}

void Key_Console(int key, int modKey)
{
	int		i;
	char	*s;

	// numpad keys -> ASCII
	static const char *kpString = KEYPAD_STRING;
	if (key >= KEYPAD_FIRST && key <= KEYPAD_LAST)
	{
		key = kpString[key - KEYPAD_FIRST];
		// fix modKey too
		modKey = (modKey & ~KEYS_MASK) | key;
	}

	switch (modKey)
	{
	case MOD_CTRL+'l':
//		Cbuf_AddText("clear\n");
		Con_Clear_f();
		return;

	//------------- input line editing ------------------
	case K_ENTER:
#if 0
		// trim spaces at line end
		for (i = strlen(editLine) - 1; i >= 0; i--)
			if (editLine[i] == ' ')
				editLine[i] = 0;
			else
				break;
#endif
		if (editLine[1] == '\\' || editLine[1] == '/')
			Cbuf_AddText(editLine + 2);			// skip the prompt
		else
			Cbuf_AddText(editLine + 1);			// valid command
		Cbuf_AddText("\n");
		appPrintf("%s\n", editLine);

		if (editLine[1])							// editLine[0] is input prompt
		{
			// find the same line in history and remove it
			for (i = 0; i < historyCount; i++)
				if (!strcmp(history[i], editLine))
				{
					if (i != historyCount - 1) memcpy(history[i], history[i+1], sizeof(history[0]) * (historyCount - 1 - i));
					historyCount--;
					break;
				}

			// add current line to history
			if (historyCount < MAX_HISTORY)
				historyCount++;
			else
				memcpy(history[0], history[1], sizeof(history[0]) * (MAX_HISTORY - 1));
			memcpy(history[historyCount-1], editLine, sizeof(editLine));
			historyLine = historyCount;
		}
		// prepare new line
		Con_ClearTyping();
		if (cls.loading)
			SCR_UpdateScreen();					// force an update, because the command may take some time
		return;

	case K_TAB:
		// command completion
		CompleteCommand();
		return;

	case K_LEFTARROW:
	case K_BACKSPACE:
	case MOD_CTRL+K_LEFTARROW:
	case MOD_CTRL+K_BACKSPACE:
		if (editPos <= 1) return;
		i = editPos;
		if (modKey >= MOD_CTRL)			// CTRL_PRESSED
		{
			while (editPos > 1 && !iswordsym(editLine[--editPos]));
			while (editPos > 0 && iswordsym(editLine[--editPos]));
			editPos++;
		}
		else
			editPos--;

		if (key == K_BACKSPACE)		// not modKey ! (i.e. without modifier)
			strcpy(&editLine[editPos], &editLine[i]);
		return;

	case K_RIGHTARROW:
		if (editLine[editPos]) editPos++;
		return;

	case MOD_CTRL+K_RIGHTARROW:
		while (editLine[editPos] && iswordsym(editLine[editPos])) editPos++;
		while (editLine[editPos] && !iswordsym(editLine[editPos])) editPos++;
		return;

	case K_DEL:
		s = &editLine[editPos];
		strcpy(s, s + 1);
		return;

	case MOD_CTRL+K_DEL:
		s = &editLine[editPos];
		*s = 0;
		return;

	case K_HOME:
		editPos = 1;
		return;

	case K_END:
		editPos = strlen(editLine);
		return;

	//---------------- history -----------------------
	case K_UPARROW:
//	case MOD_CTRL+'p':
		if (!historyLine) return;					// empty or top of history

		strcpy(editLine, history[--historyLine]);
		editPos = strlen(editLine);
		return;

	case K_DOWNARROW:
//	case MOD_CTRL+'n':
		if (historyLine >= historyCount) return;	// bottom of history

		historyLine++;
		if (historyLine == historyCount)
			Con_ClearTyping();
		else
		{
			strcpy(editLine, history[historyLine]);
			editPos = strlen(editLine);
		}
		return;

	//-------------- console scrolling ---------------------
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
		return;

	case K_PGDN:
	case MOD_CTRL+K_MWHEELDOWN:
		con.display += 2;
		return;

	case MOD_CTRL+K_HOME:
		con.display = con.current - con.totallines;
		return;

	case MOD_CTRL+K_END:
		con.display = con.current;
		return;
	}

	if (modKey >= NUM_KEYS)
		return;					// do not allow CTRL/ALT+key to produce chars

	if (key < 32 || key >= 128)
		return;		// non printable

	if (editPos < MAXCMDLINE - 2)
	{
		for (i = MAXCMDLINE - 1; i > editPos; i--) editLine[i] = editLine[i - 1];
		editLine[editPos++] = key;
	}
}

/*-----------------------------------------------------------------------------
	Initialization
-----------------------------------------------------------------------------*/

void Con_Init()
{
CVAR_BEGIN(vars)
	CVAR_VAR(con_notifyTime, 3, 0),
	CVAR_VAR(con_wordWrap, 1, CVAR_ARCHIVE),
	CVAR_VAR(con_colorText, 1, CVAR_ARCHIVE)
CVAR_END

	EXEC_ONCE(
		Cvar_GetVars(ARRAY_ARG(vars));
		GScreenWidth = -1;		// force Con_CheckResize()

		if (!con.started) Con_Clear_f();

//		Con_CheckResize();

		RegisterCommand("clear", Con_Clear_f);
		RegisterCommand("condump", Con_Dump_f);

		Con_ClearTyping();
	)
}
