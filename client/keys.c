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

/*

key up events are sent even if in console mode

*/


#define		MAXCMDLINE	256
char	key_lines[32][MAXCMDLINE];
int		key_linepos;
int		anykeydown;

static int	shift_down = false;
static int	history_line = 0;
static int	key_waiting;

static qboolean	consolekeys[256];	// if true, can't be rebound while in console
static qboolean	menubound[256];		// if true, can't be rebound while in menu
static int		keyshift[256];		// key to map to if shift held down in console
static int		key_repeats[256];	// if > 1, it is autorepeating

int		edit_line = 0;
char	*keybindings[256];
qboolean keydown[256];


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
	{"SEMICOLON",	';'},	// because a raw semicolon seperates commands

	{NULL,			0}		// end marker
};


static qboolean IsSimpleKeyName (int keynum)
{
	return keynum > 32 && keynum < 127 && keynum != ',' && keynum != ';';
}

/*
==============================================================================

			LINE TYPING INTO THE CONSOLE

==============================================================================
*/

void CompleteCommand (void)
{
	char	*cmd, *s;

	s = key_lines[edit_line]+1;
	if (*s == '\\' || *s == '/')
		s++;

	cmd = Cmd_CompleteCommand (s);
	if (cmd)
	{
		key_lines[edit_line][1] = '/';
		strcpy (key_lines[edit_line]+2, cmd);
		key_linepos = strlen(cmd)+2;
//		key_lines[edit_line][key_linepos] = ' ';
//		key_linepos++;
		key_lines[edit_line][key_linepos] = 0;
		return;
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
	char	*s;

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
	if (KEY == 'V' && keydown[K_CTRL] || (key == K_INS || key == K_KP_INS) && keydown[K_SHIFT])	// Ctrl-V or Shift-Ins
	{
		char	*cbd;

		if (cbd = Sys_GetClipboardData ())
		{
			int		i;

			strtok (cbd, "\n\r\b");

			i = strlen (cbd);
			if (i + key_linepos > MAXCMDLINE - 1)
				i = MAXCMDLINE - key_linepos - 1;

			if (i > 0)
			{
				cbd[i] = 0;
				strcat (key_lines[edit_line], cbd);
				key_linepos += i;
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
		// backslash text are commands, else chat
		if (key_lines[edit_line][1] == '\\' || key_lines[edit_line][1] == '/')
			Cbuf_AddText (key_lines[edit_line]+2);	// skip the >
		else
			Cbuf_AddText (key_lines[edit_line]+1);	// valid command

		Cbuf_AddText ("\n");
		Com_Printf ("%s\n",key_lines[edit_line]);
		edit_line = (edit_line + 1) & 31;
		history_line = edit_line;
		key_lines[edit_line][0] = ']';
		key_lines[edit_line][1] = 0;
		key_linepos = 1;
		if (cls.state == ca_disconnected)
			SCR_UpdateScreen ();					// force an update, because the command may take some time
		return;
	}

	if (key == K_TAB)
	{	// command completion
		CompleteCommand ();
		return;
	}

	if (key == K_LEFTARROW || key == K_KP_LEFTARROW || key == K_BACKSPACE)
	{
		if (key_linepos <= 1) return;
		s = key_lines[edit_line];
		i = key_linepos;
		if (keydown[K_CTRL])
		{
			while (key_linepos > 1 && !iswordsym(s[--key_linepos]));
			while (key_linepos > 0 && iswordsym(s[--key_linepos]));
			key_linepos++;
		}
		else
			key_linepos--;

		if (key == K_BACKSPACE)
			strcpy (&s[key_linepos], &s[i]);
		return;
	}

	if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW)
	{
		s = key_lines[edit_line];
		if (keydown[K_CTRL])
		{
			while (s[key_linepos] && iswordsym(s[key_linepos])) key_linepos++;
			while (s[key_linepos] && !iswordsym(s[key_linepos])) key_linepos++;
		}
		else
			if (s[key_linepos]) key_linepos++;
		return;
	}

	if (key == K_DEL || key == K_KP_DEL)
	{
		s = &key_lines[edit_line][key_linepos];
		if (!s) return; // nothing to delete
		if (keydown[K_CTRL])
			*s = 0;
		else
			strcpy (s, s + 1);
		return;
	}

	if (key == K_UPARROW || key == K_KP_UPARROW || (KEY == 'P' && keydown[K_CTRL]))
	{
		do
		{
			history_line = (history_line - 1) & 31;
		} while (history_line != edit_line
				&& !key_lines[history_line][1]);
		if (history_line == edit_line)
			history_line = (edit_line+1)&31;
		strcpy(key_lines[edit_line], key_lines[history_line]);
		key_linepos = strlen(key_lines[edit_line]);
		return;
	}

	if (key == K_DOWNARROW || key == K_KP_DOWNARROW || (KEY == 'N' && keydown[K_CTRL]))
	{
		if (history_line == edit_line) return;
		do
		{
			history_line = (history_line + 1) & 31;
		}
		while (history_line != edit_line
			&& !key_lines[history_line][1]);
		if (history_line == edit_line)
		{
			key_lines[edit_line][0] = ']';
			key_lines[edit_line][1] = 0;
			key_linepos = 1;
		}
		else
		{
			strcpy(key_lines[edit_line], key_lines[history_line]);
			key_linepos = strlen(key_lines[edit_line]);
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

	if (key == K_PGUP || key == K_KP_PGUP )
	{
		con.display -= 2;
		if (con.display < con.current - con.totallines + 10)
			con.display = con.current - con.totallines + 10;
		return;
	}

	if (key == K_PGDN || key == K_KP_PGDN )
	{
		con.display += 2;
		return;
	}

	if (key == K_HOME || key == K_KP_HOME)
	{
		if (keydown[K_CTRL])
			con.display = con.current - con.totallines + 10;
		else
			key_linepos = 1;
		return;

	}

	if (key == K_END || key == K_KP_END)
	{
		if (keydown[K_CTRL])
			con.display = con.current;
		else
			key_linepos = strlen (key_lines[edit_line]);
		return;
	}

	if (key < 32 || key > 127)
		return;	// non printable

	if (key_linepos < MAXCMDLINE - 2)
	{
		s = key_lines[edit_line];
		for (i = MAXCMDLINE - 1; i > key_linepos; i--) s[i] = s[i - 1];
		key_lines[edit_line][key_linepos] = key;
		key_linepos++;
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

	for (i = 0; i < 32; i++)
	{
		key_lines[i][0] = ']';
		key_lines[i][1] = 0;
	}
	key_linepos = 1;

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
	char	cmd[1024];
	int	rep;
	static char plus_cmd_fired[256];

	if (re.console_only && key >= 200) // no mouse & joystick in console-only mode
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
			Com_Printf ("%s is unbound, hit F4 to set\n", Key_KeynumToString (key) );
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
		{
			Com_sprintf (cmd, sizeof(cmd), "-%s %i %i\n", kb+1, key, time);
			Cbuf_AddText (cmd);
		}
		if (keyshift[key] != key)	// we can bind to "1" and "!"
		{
			kb = keybindings[keyshift[key]];
			if (kb && kb[0] == '+')
			{
				Com_sprintf (cmd, sizeof(cmd), "-%s %i %i\n", kb+1, key, time);
				Cbuf_AddText (cmd);
			}
		}
		return;
	}

	// at this point, we have only down==true events ...

	//
	// if not a consolekey, send to the interpreter no matter what mode is
	//
	if ( (cls.key_dest == key_menu && menubound[key])
	|| (cls.key_dest == key_console && !consolekeys[key])
	|| (cls.key_dest == key_game && ( cls.state == ca_active || !consolekeys[key] ) ) )
	{
		kb = keybindings[key];
		if (kb)
		{
			if (kb[0] == '+')
			{	// button commands add keynum and time as a parm
				Com_sprintf (cmd, sizeof(cmd), "%s %i %i\n", kb, key, time);
				plus_cmd_fired[key] = 1;	// signal, that command "+cmd" started (for "-cmd" allowing)
				Cbuf_AddText (cmd);
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

	for (i=0 ; i<256 ; i++)
	{
		if ( keydown[i] || key_repeats[i] )
			Key_Event( i, false, 0 );
		keydown[i] = 0;
		key_repeats[i] = 0;
	}
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
