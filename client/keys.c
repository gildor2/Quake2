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
#include "client.h"
#include "qmenu.h"	// for completion menu only

/*

key up events are sent even if in console mode

*/


#define MAX_HISTORY	32

static char history[MAX_HISTORY][MAXCMDLINE];
static int	historyLine, historyCount;

// current line
char	editLine[MAXCMDLINE];
int		editPos;

int		anykeydown;

char	*keybindings[256];
byte	keydown[256];				// really, it is qboolean, but "byte" is more compact

static int	shift_down = false;
static int	key_waiting;

static byte		consolekeys[256];	// if true, can't be rebound while in console
static byte		menubound[256];		// if true, can't be rebound while in menu
static int		keyshift[256];		// key to map to if shift held down in console
static int		key_repeats[256];	// if > 1, it is autorepeating


typedef struct
{
	char	*name;
	int		keynum;
} keyname_t;

static keyname_t keynames[] =
{
	{"TAB",			K_TAB},
	{"ENTER",		K_ENTER},
	{"ESCAPE",		K_ESCAPE},
	{"SPACE",		K_SPACE},
	{"BACKSPACE",	K_BACKSPACE},
	{"UPARROW",		K_UPARROW},
	{"DOWNARROW",	K_DOWNARROW},
	{"LEFTARROW",	K_LEFTARROW},
	{"RIGHTARROW",	K_RIGHTARROW},

	{"ALT",			K_ALT},
	{"CTRL",		K_CTRL},
	{"SHIFT",		K_SHIFT},

	{"F1",			K_F1},
	{"F2",			K_F2},
	{"F3",			K_F3},
	{"F4",			K_F4},
	{"F5",			K_F5},
	{"F6",			K_F6},
	{"F7",			K_F7},
	{"F8",			K_F8},
	{"F9",			K_F9},
	{"F10",			K_F10},
	{"F11",			K_F11},
	{"F12",			K_F12},

	{"INS",			K_INS},
	{"DEL",			K_DEL},
	{"PGDN",		K_PGDN},
	{"PGUP",		K_PGUP},
	{"HOME",		K_HOME},
	{"END",			K_END},

	{"MOUSE1",		K_MOUSE1},
	{"MOUSE2",		K_MOUSE2},
	{"MOUSE3",		K_MOUSE3},

	{"JOY1",		K_JOY1},
	{"JOY2",		K_JOY2},
	{"JOY3",		K_JOY3},
	{"JOY4",		K_JOY4},

	{"AUX1",		K_AUX1},
	{"AUX2",		K_AUX2},
	{"AUX3",		K_AUX3},
	{"AUX4",		K_AUX4},
	{"AUX5",		K_AUX5},
	{"AUX6",		K_AUX6},
	{"AUX7",		K_AUX7},
	{"AUX8",		K_AUX8},
	{"AUX9",		K_AUX9},
	{"AUX10",		K_AUX10},
	{"AUX11",		K_AUX11},
	{"AUX12",		K_AUX12},
	{"AUX13",		K_AUX13},
	{"AUX14",		K_AUX14},
	{"AUX15",		K_AUX15},
	{"AUX16",		K_AUX16},
	{"AUX17",		K_AUX17},
	{"AUX18",		K_AUX18},
	{"AUX19",		K_AUX19},
	{"AUX20",		K_AUX20},
	{"AUX21",		K_AUX21},
	{"AUX22",		K_AUX22},
	{"AUX23",		K_AUX23},
	{"AUX24",		K_AUX24},
	{"AUX25",		K_AUX25},
	{"AUX26",		K_AUX26},
	{"AUX27",		K_AUX27},
	{"AUX28",		K_AUX28},
	{"AUX29",		K_AUX29},
	{"AUX30",		K_AUX30},
	{"AUX31",		K_AUX31},
	{"AUX32",		K_AUX32},

	{"KP_HOME",		K_KP_HOME},
	{"KP_UPARROW",	K_KP_UPARROW},
	{"KP_PGUP",		K_KP_PGUP},
	{"KP_LEFTARROW",K_KP_LEFTARROW},
	{"KP_5",		K_KP_5},
	{"KP_RIGHTARROW",K_KP_RIGHTARROW},
	{"KP_END",		K_KP_END},
	{"KP_DOWNARROW",K_KP_DOWNARROW},
	{"KP_PGDN",		K_KP_PGDN},
	{"KP_ENTER",	K_KP_ENTER},
	{"KP_INS",		K_KP_INS},
	{"KP_DEL",		K_KP_DEL},
	{"KP_SLASH",	K_KP_SLASH},
	{"KP_MINUS",	K_KP_MINUS},
	{"KP_PLUS",		K_KP_PLUS},

	{"UNK46",		K_UNK46},
	{"UNK59",		K_UNK59},
	{"UNK5A",		K_UNK5A},
	{"UNK5B",		K_UNK5B},
	{"UNK5C",		K_UNK5C},
	{"UNK5D",		K_UNK5D},
	{"UNK5E",		K_UNK5E},
	{"UNK5F",		K_UNK5F},

	{"MWHEELUP",	K_MWHEELUP},
	{"MWHEELDOWN",	K_MWHEELDOWN},

	{"PAUSE",		K_PAUSE},

	{"COMMA",		','},	// because comma used for separating multiple wildcards
	{"SEMICOLON",	';'},	// because a raw semicolon separates commands

	{NULL,			0}		// end marker
};


static qboolean IsSimpleKeyName (int keynum)
{
	return keynum > 32 && keynum < 127 && keynum != ',' && keynum != ';';
}

/*
===================
Key_StringToKeynum

Returns a key number to be used to index keybindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int Key_StringToKeynum (char *str)
{
	keyname_t	*kn;

	if (!str || !str[0])
		return -1;
	if (!str[1])
		return str[0];

	for (kn = keynames; kn->name ; kn++)
	{
		if (!Q_strcasecmp (str,kn->name))
			return kn->keynum;
	}
	return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
char *Key_KeynumToString (int keynum)
{
	keyname_t	*kn;
	static char tinystr[2];

	if (keynum == -1)
		return "^1<KEY NOT FOUND>";
	if (IsSimpleKeyName (keynum))
	{	// printable ascii
		tinystr[0] = keynum;
//		tinystr[1] = 0;
		return tinystr;
	}

	for (kn = keynames; kn->name; kn++)
		if (keynum == kn->keynum)
			return kn->name;

	return "^1<UNKNOWN KEYNUM>";
}


/*
==============================================================================

			LINE TYPING INTO THE CONSOLE

==============================================================================
*/

/*
============
Cmd_CompleteCommand
============
*/
static char complete_command[256], completed_name[256], *partial_name;
static int completed_count, partial_len;

#define MAX_COMPLETE_ITEMS	16
static char completeVariants[MAX_COMPLETE_ITEMS][256];


// "display": 0 - complete+count, 1 - display
static void TryComplete (char *full, int display, char mark)
{
	if (!Q_strncasecmp (partial_name, full, partial_len))
	{
		if (display) Com_Printf ("  %c  %s\n", mark + 128, full);

		if (!completed_count)	// have not yet completed - just copy string
			strcpy (completed_name, full);
		else					// already completed - refine string
		{
			char *s, *d;

			s = completed_name;
			d = full;
			while (*s == *d++) s++;
			*s = 0;				// limit with last matched char
		}

		if (completed_count < MAX_COMPLETE_ITEMS)
			strcpy (completeVariants[completed_count], full);
		completed_count++;
	}
}


static char *Do_CompleteCommand (char *partial)
{
	int		display, len;
	char	*path, *ext;
	char	*arg1s, arg1[256], *arg2s;	// arg1s -> "arg1 arg2 ..."; arg2s -> "arg2 arg3 ..."
	char	*name, comp_type;
	cvar_t	*cvar;

	complete_command[0] = 0;
	completed_name[0] = 0;
	completed_count = 0;

	/*------ complete argument for variable/map/demomap -------*/

	if (arg1s = strchr (partial, ' '))
	{
		basenamed_t	*filelist, *fileitem;

		if (arg1s == partial)
			return NULL;	// space is first line char!

		len = arg1s - partial;
		strncpy (complete_command, partial, len);
		complete_command[len] = 0;
		arg1s++;							// skip ' '

		if (arg2s = strchr (arg1s, ' '))
		{
			len = arg2s - arg1s;
			strncpy (arg1, arg1s, len);
			arg1[len] = 0;
			arg2s++;
		}
		else
			strcpy (arg1, arg1s);

//		Com_Printf ("cmd: \"%s\"  arg1s: \"%s\"  arg2s: \"%s\"  arg1: \"%s\"\n",command, arg1s, arg2s, arg1);

		if (arg2s)
		{
			if (strlen (arg2s))
				return NULL;		// have non-zero 2nd argument

			// have "command arg1 " (space after arg1)

			// complete "alias name "
			if (!Q_strcasecmp (complete_command, "alias"))
			{
				cmdAlias_t *alias;

				for (alias = cmdAlias; alias; alias = alias->next)
					if (!Q_strcasecmp (alias->name, arg1))
					{
						strcpy (completed_name, va("alias %s \"%s\"", arg1, alias->value));
						return completed_name;
					}
				return NULL;
			}
			// complete "bind key "
			else if (!Q_strcasecmp (complete_command, "bind"))
			{
				int		key;

				key = Key_StringToKeynum (arg1);
				if (key < 0 || !keybindings[key])
					return NULL;

				strcpy (completed_name, va("bind %s \"%s\"", arg1, keybindings[key]));
				return completed_name;
			}
		}

		if (!Q_strcasecmp (complete_command, "alias"))
		{
			// complete alias name
			strcpy (complete_command, "alias");
			for (display = 0; display < 2; display++)
			{
				cmdAlias_t *alias;

				partial_name = arg1s;
				partial_len = strlen (arg1s);
				completed_count = 0;
				for (alias = cmdAlias; alias; alias = alias->next)
					TryComplete (alias->name, display, 'a');
				if (!completed_count)
					return NULL;

				if (completed_count == 1)
				{
					strcat (completed_name, " ");
					break;
				}
				if (!display)
					Com_Printf ("]/%s\n", partial);
			}

			strcpy (completed_name, va("%s %s", complete_command, completed_name));
			return completed_name;
		}

		if (!Q_strcasecmp (complete_command, "map"))
		{
			path = "maps";
			ext = ".bsp";
			comp_type = 'm';
		}
		else if (!Q_strcasecmp (complete_command, "demomap"))
		{
			path = "demos";
			ext = ".dm2";
			comp_type = 'd';
		}
		else // try to complete varname with its value
		{
			if (*arg1s)
				return NULL;				// arg is not empty

			for (cvar = cvar_vars; cvar; cvar = cvar->next)
				if (!Q_strcasecmp (cvar->name, complete_command))
				{
					strcpy (completed_name, partial); // "varname "
					strcat (completed_name, cvar->string);
					return completed_name;
				}
			return NULL;
		}

		// complete "map" or "demomap" with mask/arg*
		filelist = FS_ListFiles (va("%s/*%s", path, ext), NULL, 0, SFF_SUBDIR);
		if (filelist)
		{
			for (fileitem = filelist; fileitem; fileitem = fileitem->next)
			{
				// remove path part
				if (name = strrchr (fileitem->name, '/'))
					fileitem->name = name + 1;
				// refine/remove file extension
				if ((name = strrchr (fileitem->name, '.')) && !Q_strcasecmp (name, ext))
					*name = 0;				// cut extension
				else
					*fileitem->name = 0;	// cut whole filename - refined by ext
			}
			partial_name = arg1;
			partial_len = strlen (arg1);
			for (display = 0; display < 2; display++)
			{
				completed_count = 0;
				for (fileitem = filelist; fileitem; fileitem = fileitem->next)
					TryComplete (fileitem->name, display, comp_type);
				if (!completed_count)
					return NULL;			// not completed

				if (completed_count == 1)
				{
					strcat (completed_name, " ");
					break;
				}
				if (!display)
				{	// at this loop we just see, that we have many matches ...
					Com_Printf ("]/%s\n", partial);
				}
			}
			strcpy (completed_name, va("%s %s", complete_command, completed_name));
			FreeNamedList (filelist);
			return completed_name;
		}
		return NULL;
	}

	/*--------- complete command/variable/alias name ------------*/

	partial_len = strlen (partial);
	if (!partial_len) return NULL;
	partial_name = partial;

	for (display = 0; display < 2; display++)
	{
		cmdFunc_t *cmd;
		cmdAlias_t *a;

		completed_count = 0;

		// check for partial match
		for (cmd = cmdFuncs; cmd; cmd = cmd->next)
			TryComplete (cmd->name, display, 'c');
		for (a = cmdAlias; a; a=a->next)
			TryComplete (a->name, display, 'a');
		for (cvar = cvar_vars; cvar; cvar=cvar->next)
			TryComplete (cvar->name, display, 'v');

		if (!completed_count) return NULL; // not completed

		if (completed_count == 1)
		{	// only one match
			strcat (completed_name, " ");
			return completed_name;
		}
		// many matches
		if (!display)
		{	// at this loop we just see, that we have many matches ...
			Com_Printf ("]/%s\n", partial);
		}
	}
	return completed_name;
}

// from menu.c
const char *Default_MenuKey (menuFramework_t *m, int key);
void M_PushMenu (void (*draw) (void), const char *(*key) (int k));

static menuAction_t		completeItems[MAX_COMPLETE_ITEMS];
static menuFramework_t	completeMenu;
static int	complMenu_x, complMenu_y, complMenu_w, complMenu_h;

static void CompleteMenuDraw (void)
{
	re.DrawFill2 (complMenu_x, complMenu_y, complMenu_w, complMenu_h, 0.1, 0.5, 0.5, 0.8);
	Menu_AdjustCursor (&completeMenu, 1);
	Menu_Draw (&completeMenu);
}

static const char *CompleteMenuKey (int key)
{
	int		old_depth;
	const char	*res;

	old_depth = m_menudepth;
	res = Default_MenuKey (&completeMenu, key);
	if (m_menudepth != old_depth)
	{
		// "ESCAPE" (or any other menu-exit-key) pressed
		cls.key_dest = key_console;
		cls.keep_console = false;
	}

	return res;
}

static void CompleteMenuCallback (void *item)
{
	char	*s;

	s = completeVariants[((menuAction_t*) item)->generic.localdata[0]];
	M_ForceMenuOff ();

	editLine[1] = '/';
	if (complete_command[0])
		strcpy (editLine + 2, va("%s %s ", complete_command, s));
	else
		strcpy (editLine + 2, va("%s ", s));
	editPos = strlen (editLine);
	editLine[editPos] = 0;

	cls.key_dest = key_console;
	cls.keep_console = false;
}


static void CompleteMenu (void)
{
	int		i, y, len, maxLen;

	MENU_CHECK

	// init menu
	completeMenu.nitems = 0;
	completeMenu.cursor = 0;
	maxLen = 0;
	for (i = 0, y = 0; i < completed_count; i++, y += 10)
	{
		MENU_ACTION(completeItems[i], y, completeVariants[i], CompleteMenuCallback);
		completeItems[i].generic.flags = QMF_LEFT_JUSTIFY;
		completeItems[i].generic.localdata[0] = i;
		Menu_AddItem (&completeMenu, &completeItems[i]);
		len = strlen (completeVariants[i]);
		if (len > maxLen) maxLen = len;
	}

	i = viddef.height;
	if (cls.state != ca_disconnected && cls.state != ca_connecting)
		i /= 2;
	complMenu_h = completed_count * 10 + 8;
	complMenu_w = maxLen * 8 + 28;
	completeMenu.x = editPos * 8 + 8;
	completeMenu.y = i - 22;
	if (completeMenu.x + complMenu_w > viddef.width)
		completeMenu.x = viddef.width - complMenu_w;
	if (completeMenu.y + complMenu_h > viddef.height)
		completeMenu.y = viddef.height - complMenu_h;
	complMenu_x = completeMenu.x - 28;
	complMenu_y = completeMenu.y - 4;

	cls.keep_console = true;
	M_PushMenu (CompleteMenuDraw, CompleteMenuKey);
}


static void CompleteCommand (void)
{
	char	*cmd, *s;

	s = &editLine[1];
	if (*s == '\\' || *s == '/')
		s++;

	if (!strcmp (completed_name, s) && completed_count > 1 && completed_count <= MAX_COMPLETE_ITEMS)
	{
		CompleteMenu ();
		return;
	}

	cmd = Do_CompleteCommand (s);
	if (cmd)
	{
		editLine[1] = '/';
		strcpy (editLine + 2, cmd);
		editPos = strlen (cmd) + 2;
		editLine[editPos] = 0;
	}
}

/*
====================
Key_Console

Interactive line editing and console scrollback
====================
*/

static qboolean iswordsym(char n)
{
	return (n >= '0' && n <= '9') || (n >= 'A' && n <= 'Z') || (n >= 'a' && n <= 'z') || n == '_';
}

void Key_Console (int key)
{
	int		i, KEY;

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

	KEY = toupper (key);
	if (KEY == 'V' && keydown[K_CTRL] || key == K_INS && keydown[K_SHIFT])	// Ctrl-V or Shift-Ins
	{
		char	*cbd;

		if (cbd = Sys_GetClipboardData ())
		{
//			strtok (cbd, "\n\r\b");
			i = strlen (cbd);
			if (i + editPos > MAXCMDLINE - 1)
				i = MAXCMDLINE - editPos - 1;

			if (i > 0)
			{
				cbd[i] = 0;
				strcat (editLine, cbd);
				editPos += i;
			}
			free (cbd);
		}

		return;
	}

	if (KEY == 'L' && keydown[K_CTRL])		// Ctrl-L
	{
		Cbuf_AddText ("clear\n");
		return;
	}

	if (key == K_ENTER || key == K_KP_ENTER)
	{
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
	}

	if (key == K_TAB)
	{	// command completion
		CompleteCommand ();
		return;
	}

	if (key == K_LEFTARROW || key == K_BACKSPACE)
	{
		if (editPos <= 1) return;
		i = editPos;
		if (keydown[K_CTRL])
		{
			while (editPos > 1 && !iswordsym (editLine[--editPos]));
			while (editPos > 0 && iswordsym (editLine[--editPos]));
			editPos++;
		}
		else
			editPos--;

		if (key == K_BACKSPACE)
			strcpy (&editLine[editPos], &editLine[i]);
		return;
	}

	if (key == K_RIGHTARROW)
	{
		if (keydown[K_CTRL])
		{
			while (editLine[editPos] && iswordsym (editLine[editPos])) editPos++;
			while (editLine[editPos] && !iswordsym (editLine[editPos])) editPos++;
		}
		else
			if (editLine[editPos]) editPos++;
		return;
	}

	if (key == K_DEL)
	{
		char	*s;

		s = &editLine[editPos];
		if (keydown[K_CTRL])
			*s = 0;
		else
			strcpy (s, s + 1);
		return;
	}

	if (key == K_UPARROW || (KEY == 'P' && keydown[K_CTRL]))
	{
		if (!historyLine) return;					// empty or top of history

		strcpy (editLine, history[--historyLine]);
		editPos = strlen (editLine);
		return;
	}

	if (key == K_DOWNARROW || (KEY == 'N' && keydown[K_CTRL]))
	{
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
	}

	if (key == K_MWHEELUP)
	{
		con.display -= 8;
		return;
	}

	if (key == K_MWHEELDOWN)
	{
		con.display += 8;
		return;
	}

	if (key == K_PGUP)
	{
		con.display -= 2;
		if (con.display < con.current - con.totallines + 10)
			con.display = con.current - con.totallines + 10;
		return;
	}

	if (key == K_PGDN)
	{
		con.display += 2;
		return;
	}

	if (key == K_HOME)
	{
		if (keydown[K_CTRL])
			con.display = con.current - con.totallines + 10;
		else
			editPos = 1;
		return;

	}

	if (key == K_END)
	{
		if (keydown[K_CTRL])
			con.display = con.current;
		else
			editPos = strlen (editLine);
		return;
	}

	if (key < 32 || key > 127)
		return;	// non printable

	if (editPos < MAXCMDLINE - 2)
	{
		for (i = MAXCMDLINE - 1; i > editPos; i--) editLine[i] = editLine[i - 1];
		editLine[editPos++] = key;
	}
}


//============================================================================

qboolean chat_team;
char	chat_buffer[MAXCMDLINE];
int		chat_bufferlen = 0;

void Key_Message (int key)
{

	if (key == K_ENTER || key == K_KP_ENTER)
	{
		if (chat_team)
			Cbuf_AddText ("say_team \"");
		else
			Cbuf_AddText ("say \"");
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

	if (key < 32 || key > 127)
		return;		// non printable

	if (key == K_BACKSPACE)
	{
		if (chat_bufferlen)
		{
			chat_bufferlen--;
			chat_buffer[chat_bufferlen] = 0;
		}
		return;
	}

	if (chat_bufferlen == sizeof(chat_buffer)-1)
		return;		// all full

	chat_buffer[chat_bufferlen++] = key;
	chat_buffer[chat_bufferlen] = 0;
}


//============================================================================


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding (int keynum, char *binding)
{
	char	*new;
	int		l;

	if (keynum == -1)
		return;

	// free old bindings
	if (keybindings[keynum])
	{
		Z_Free (keybindings[keynum]);
		keybindings[keynum] = NULL;
	}

	l = strlen (binding);
	if (l)
	{
		// allocate memory for new binding
		new = Z_Malloc (l+1);
		strcpy (new, binding);
		new[l] = 0;
		keybindings[keynum] = new;
	}
}


/*-------------- Console commands ------------------*/

static void Key_Unbind_f (void)
{
	int		i, n;
	char	*mask;
	keyname_t *kn;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("unbind <key mask> : remove commands from a key(s)\n");
		return;
	}
	mask = Cmd_Argv (1);

	n = 0;
	for (i = 0; i < 256; i++)
	{
		char	*keyName;
		static char buf[2];

		if (!keybindings[i])
			continue;		// key is not bound

		if (IsSimpleKeyName (i))
		{
			// key name is a single letter
			buf[0] = i;
			keyName = buf;
		}
		else
		{
			// find key name
			keyName = NULL;
			for (kn = keynames; kn->name; kn++)
				if (i == kn->keynum)
				{
					keyName = kn->name;
					break;
				}
		}

		if (keyName && MatchWildcard (keyName, mask))
		{
			Com_DPrintf ("unbind %s\n", keyName);
			Key_SetBinding (i, "");
			n++;
		}
	}

	if (n)
		Com_Printf ("%d key(s) unbound\n", n);
	else
		Com_Printf ("\"%s\" isn't a valid key\n", mask);
}


static void Key_Unbindall_f (void)
{
	int		i;

	for (i=0 ; i<256 ; i++)
		if (keybindings[i])
			Key_SetBinding (i, "");
}


static void Key_Bind_f (void)
{
	int		i, c, b;
	char	cmd[1024];

	c = Cmd_Argc();

	if (c < 2)
	{
		Com_Printf ("bind <key> [command]\n");
		return;
	}
	b = Key_StringToKeynum (Cmd_Argv(1));
	if (b == -1)
	{
		Com_WPrintf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{	// just print current binding
		if (keybindings[b])
			Com_Printf ("\"%s\" = \"%s\"\n", Cmd_Argv(1), keybindings[b] );
		else
			Com_Printf ("\"%s\" is not bound\n", Cmd_Argv(1) );
		return;
	}

	// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (i = 2; i < c; i++)
	{
		strcat (cmd, Cmd_Argv(i));
		if (i != (c-1))
			strcat (cmd, " ");
	}

	Key_SetBinding (b, cmd);
}


void Key_Bindlist_f (void)
{
	int		i, n;
	char	*mask;

	if (Cmd_Argc () > 2)
	{
		Com_Printf ("Usage: bindlist [<action mask>]\n");
		return;
	}

	if (Cmd_Argc () == 2)
		mask = Cmd_Argv (1);
	else
		mask = NULL;

	n = 0;
	for (i = 0; i < 256; i++)
		if (keybindings[i] && keybindings[i][0] && (!mask || MatchWildcard (keybindings[i], mask)))
		{
			n++;
			Com_Printf ("%s \"%s\"\n", Key_KeynumToString(i), keybindings[i]);
		}
	if (mask)
		Com_Printf ("    %d binds found\n", n);
}


/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings (FILE *f)
{
	int		i;

	for (i = 0; i < 256; i++)
		if (keybindings[i] && keybindings[i][0])
			fprintf (f, "bind %s \"%s\"\n", Key_KeynumToString(i), keybindings[i]);
}


/*
===================
Key_Init
===================
*/
void Key_Init (void)
{
	int		i;
	static byte conKeyCodes[] = {
		K_ENTER,		K_KP_ENTER,			K_TAB,		K_LEFTARROW,	K_KP_LEFTARROW,
		K_RIGHTARROW,	K_KP_RIGHTARROW,	K_UPARROW,	K_KP_UPARROW,	K_DOWNARROW,
		K_KP_DOWNARROW,	K_BACKSPACE,		K_HOME,		K_KP_HOME,		K_END,
		K_KP_END,		K_PGUP,				K_KP_PGUP,	K_PGDN,			K_KP_PGDN,
		K_ALT,			K_CTRL,				K_SHIFT,	K_INS,			K_KP_INS,
		K_DEL,			K_KP_DEL,			K_KP_SLASH,	K_KP_PLUS,		K_KP_MINUS,
		K_KP_5,			K_MWHEELDOWN,		K_MWHEELUP,	K_MOUSE1,		K_MOUSE2
	};
	static struct {byte base; byte shift;} keyShifts[] = {
		{'1', '!'},	{'2', '@'},	{'3', '#'},	{'4', '$'},	{'5', '%'},
		{'6', '^'},	{'7', '&'},	{'8', '*'},	{'9', '('},	{'0', ')'},
		{'-', '_'},	{'=', '+'},	{',', '<'},	{'.', '>'},	{'/', '?'},
		{';', ':'},	{'\'', '"'},{'[', '{'},	{']', '}'},	{'`', '~'},
		{'\\', '|'}
	};

	Key_ClearTyping ();

	// init ascii characters in console mode
	for (i = 32; i < 128; i++)
		consolekeys[i] = true;
	for (i = 0; i < sizeof(conKeyCodes)/sizeof(conKeyCodes[0]); i++)
		consolekeys[conKeyCodes[i]] = true;

	consolekeys['`'] = false;
	consolekeys['~'] = false;

	for (i = 0; i < 256; i++)
		keyshift[i] = i;
	for (i = 'a'; i <= 'z'; i++)
		keyshift[i] = i - 'a' + 'A';
	for (i = 0; i < sizeof(keyShifts)/sizeof(keyShifts[0]); i++)
		keyshift[keyShifts[i].base] = keyShifts[i].shift;

	menubound[K_ESCAPE] = true;
	for (i = 0; i < 12; i++)
		menubound[K_F1+i] = true;

	Cmd_AddCommand ("bind", Key_Bind_f);
	Cmd_AddCommand ("unbind", Key_Unbind_f);
	Cmd_AddCommand ("unbindall", Key_Unbindall_f);
	Cmd_AddCommand ("bindlist", Key_Bindlist_f);
}

/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
void Key_Event (int key, qboolean down, unsigned time)
{
	char	*kb;
	int		rep;
	static char plus_cmd_fired[256];

	if (*re.flags & REF_CONSOLE_ONLY && key >= 200) // no mouse & joystick in console-only mode (??)
		return;

	// hack for modal presses
	if (key_waiting == -1)
	{
		if (down)
			key_waiting = key;
		return;
	}

	// update auto-repeat status
	if (down)
	{
		rep = ++key_repeats[key];
		kb = keybindings[key];
		if (rep > 1 && rep < 5 ||
			(cls.key_dest == key_game && cls.state != ca_disconnected) && rep > 1 && kb && kb[0] == '+')
			return;		// no autorepeats for "+cmd" and make a delay before autorepeat activation

		if (key >= 190 && (!kb || !kb[0]))	// old: key >= 200
			Com_Printf ("%s is unbound, hit F4 to set\n", Key_KeynumToString (key) );	//?? F4
	}
	else
	{
		key_repeats[key] = 0;
	}

	if (key == K_SHIFT)
		shift_down = down;

	// console key is hardcoded, so the user can never unbind it
	if (key == '`' || key == '~')
	{
		if (!down)
			return;
		Con_ToggleConsole_f ();
		return;
	}

	// any key during the attract mode will bring up the menu
	if (cl.attractloop && cls.key_dest != key_menu &&
		!(key >= K_F1 && key <= K_F12))
		key = K_ESCAPE;

	// menu key is hardcoded, so the user can never unbind it
	if (key == K_ESCAPE)
	{
		if (!down)
			return;

		if (cl.frame.playerstate.stats[STAT_LAYOUTS] && cls.key_dest == key_game)
		{	// put away help computer / inventory
			Cbuf_AddText ("cmd putaway\n");
			return;
		}
		switch (cls.key_dest)
		{
		case key_message:
			Key_Message (key);
			break;
		case key_menu:
			M_Keydown (key);
			break;
		case key_game:
		case key_console:
			M_Menu_Main_f ();
			break;
		default:
			Com_Error (ERR_FATAL, "Bad cls.key_dest");
		}
		return;
	}

	// track if any key is down for BUTTON_ANY
	keydown[key] = down;
	if (down)
	{
		if (key_repeats[key] == 1)
			anykeydown++;
	}
	else
	{
		anykeydown--;
		if (anykeydown < 0)
			anykeydown = 0;
	}

	//
	// key up events only generate commands if the game key binding is
	// a button command (leading + sign).  These will occur even in console mode,
	// to keep the character from continuing an action started before a console
	// switch.  Button commands include the kenum as a parameter, so multiple
	// downs can be matched with ups
	//
	if (!down)
	{
		if (!plus_cmd_fired[key])
			return;
		plus_cmd_fired[key] = 0;

		kb = keybindings[key];
		if (kb && kb[0] == '+')
			Cbuf_AddText (va("-%s %i %i\n", kb+1, key, time));

		if (keyshift[key] != key)	// we can bind to "1" and "!"
		{
			kb = keybindings[keyshift[key]];
			if (kb && kb[0] == '+')
				Cbuf_AddText (va("-%s %i %i\n", kb+1, key, time));
		}
		return;
	}

	// at this point, we have only down==true events ...

	// if not a consolekey, send to the interpreter no matter what mode is
	if ((cls.key_dest == key_menu && menubound[key])
		|| (cls.key_dest == key_console && !consolekeys[key])
		|| (cls.key_dest == key_game && ( cls.state == ca_active || !consolekeys[key])))
	{
		kb = keybindings[key];
		if (kb)
		{
			if (kb[0] == '+')
			{	// button commands add keynum and time as a parm
				plus_cmd_fired[key] = 1;	// signal, that command "+cmd" started (for "-cmd" allowing)
				Cbuf_AddText (va("%s %i %i\n", kb, key, time));
			}
			else
			{
				Cbuf_AddText (kb);
				Cbuf_AddText ("\n");
			}
		}
		return;
	}

	if (shift_down)
		key = keyshift[key];

	switch (cls.key_dest)
	{
	case key_message:
		Key_Message (key);
		break;
	case key_menu:
		M_Keydown (key);
		break;

	case key_game:
	case key_console:
		Key_Console (key);
		break;
	default:
		Com_Error (ERR_FATAL, "Bad cls.key_dest");
	}
}

/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates (void)
{
	int		i;

	anykeydown = false;

	for (i = 0; i < 256; i++)
	{
		if (keydown[i] || key_repeats[i])
			Key_Event (i, false, 0);
		keydown[i] = 0;
		key_repeats[i] = 0;
	}
}


void Key_ClearTyping (void)
{
	editLine[0] = ']';
	editLine[1] = 0;
	editPos = 1;
}


/*
===================
Key_GetKey
===================
*/
int Key_GetKey (void)
{
	key_waiting = -1;

	while (key_waiting == -1)
		Sys_SendKeyEvents ();

	return key_waiting;
}
