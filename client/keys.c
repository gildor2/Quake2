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


int		anykeydown;

char	*keybindings[NUM_KEYS];
byte	keydown[NUM_KEYS];				// really, it is qboolean, but "byte" is more compact

static int	shift_down = false;
static int	key_waiting;

static byte		consolekeys[NUM_KEYS];	// if true, can't be rebound while in console
static byte		menubound[NUM_KEYS];	// if true, can't be rebound while in menu
static int		keyshift[NUM_KEYS];		// key to map to if shift held down in console
static int		key_repeats[NUM_KEYS];	// if > 1, it is autorepeating


/*-----------------------------------------------------------------------------
	Support for kay names
	?? should check for correct UNKxx keys: UNK30 is "correct", but will newer be pressed
-----------------------------------------------------------------------------*/

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

	{"F1",		K_F1},	{"F2",		K_F2},	{"F3",		K_F3},	{"F4",		K_F4},
	{"F5",		K_F5},	{"F6",		K_F6},	{"F7",		K_F7},	{"F8",		K_F8},
	{"F9",		K_F9},	{"F10",		K_F10},	{"F11",		K_F11},	{"F12",		K_F12},
	{"F13",		K_F13},	{"F14",		K_F14},	{"F15",		K_F15},	{"F16",		K_F16},
	{"F17",		K_F17},	{"F18",		K_F18},	{"F19",		K_F19},	{"F20",		K_F20},
	{"F21",		K_F21},	{"F22",		K_F22},	{"F23",		K_F23},	{"F24",		K_F24},

	{"INS",			K_INS},
	{"DEL",			K_DEL},
	{"PGDN",		K_PGDN},
	{"PGUP",		K_PGUP},
	{"HOME",		K_HOME},
	{"END",			K_END},

	// keypad
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

	// mouse
	{"MOUSE1",	K_MOUSE1},	{"MOUSE2",	K_MOUSE2},	{"MOUSE3",	K_MOUSE3},
	{"MWHEELUP",	K_MWHEELUP},	{"MWHEELDOWN",	K_MWHEELDOWN},

	// joystick
	{"JOY1",	K_JOY1},	{"JOY2",	K_JOY2},	{"JOY3",	K_JOY3},	{"JOY4",	K_JOY4},

	{"AUX1",	K_AUX1},	{"AUX2",	K_AUX2},	{"AUX3",	K_AUX3},	{"AUX4",	K_AUX4},
	{"AUX5",	K_AUX5},	{"AUX6",	K_AUX6},	{"AUX7",	K_AUX7},	{"AUX8",	K_AUX8},
	{"AUX9",	K_AUX9},	{"AUX10",	K_AUX10},	{"AUX11",	K_AUX11},	{"AUX12",	K_AUX12},
	{"AUX13",	K_AUX13},	{"AUX14",	K_AUX14},	{"AUX15",	K_AUX15},	{"AUX16",	K_AUX16},
	{"AUX17",	K_AUX17},	{"AUX18",	K_AUX18},	{"AUX19",	K_AUX19},	{"AUX20",	K_AUX20},
	{"AUX21",	K_AUX21},	{"AUX22",	K_AUX22},	{"AUX23",	K_AUX23},	{"AUX24",	K_AUX24},
	{"AUX25",	K_AUX25},	{"AUX26",	K_AUX26},	{"AUX27",	K_AUX27},	{"AUX28",	K_AUX28},
	{"AUX29",	K_AUX29},	{"AUX30",	K_AUX30},	{"AUX31",	K_AUX31},	{"AUX32",	K_AUX32},

	{"PAUSE",		K_PAUSE},
	{"CAPSLOCK",	K_CAPSLOCK},
	{"NUMLOCK",		K_NUMLOCK},
	{"SCRLOCK",		K_SCRLOCK},
	{"PRINTSCRN",	K_PRINTSCRN},	//?? sent only when key is up

	{"KP_STAR",		'*'},		// to resolve "unbind *" ambiguity
	{"COMMA",		','},		// because comma used for separating multiple wildcards
	{"SEMICOLON",	';'}		// because a raw semicolon separates commands
};


static bool IsSimpleKeyName (int keynum)
{
	return keynum > 32 && keynum < 127 && keynum != ',' && keynum != ';';
}

// NOTE: 'c' is lowercase digit
static int HexDigit (char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	else return -1;			// bad digit
}


int Key_StringToKeynum (char *str)
{
	int		i;
	keyname_t *kn;
	char	name[32];

	if (!str || !str[0]) return -1;		// bad string
	if (!str[1]) return str[0];			// simple key

	Q_strncpylower (name, str, sizeof(name)-1);
	// check for 'UNKxx'
	if (name[0] == 'u' && name[1] == 'n' && name[2] == 'k' && name[5] == 0)
	{
		int		n1, n2;

		n1 = HexDigit (name[3]);
		if (n1 < 0) return -1;
		n2 = HexDigit (name[4]);
		if (n2 < 0) return -1;
		return (n1 << 4) + n2 + 256;
	}

	for (i = 0, kn = keynames; i < ARRAY_COUNT(keynames); i++, kn++)
	{
		if (!strcmp (str,kn->name))
			return kn->keynum;
	}
	return -1;
}


char *Key_KeynumToString (int keynum)
{
	int		i;
	keyname_t *kn;

	if (keynum == -1)
		return "^1<KEY NOT FOUND>";

	if (IsSimpleKeyName (keynum))
	{
		static char tinystr[2];

		// printable ascii
		tinystr[0] = keynum;
		return tinystr;
	}

	if (keynum >= 256)
	{
		static char keyname[] = "UNK00";

		Com_sprintf (ARRAY_ARG(keyname), "UNK%02X", keynum - 256);
		return keyname;
	}

	for (i = 0, kn = keynames; i < ARRAY_COUNT(keynames); i++, kn++)
	{
		if (keynum == kn->keynum)
			return kn->name;
	}

	return "^1<UNKNOWN KEYNUM>";
}


/*-----------------------------------------------------------------------------
	Command completion in console
-----------------------------------------------------------------------------*/

//!! check for overflow of this buffers
static char complete_command[256], completed_name[512], *partial_name;
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
					if (strchr (cvar->string, ' '))
						strcat (completed_name, va("\"%s\"", cvar->string));
					else
						strcat (completed_name, cvar->string);
					return completed_name;
				}
			return NULL;
		}

		// complete "map" or "demomap" with mask/arg*
		filelist = FS_ListFiles (va("%s/*%s", path, ext), NULL, LIST_FILES);
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
		i = Q_round (i * scr_con_current);
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


void CompleteCommand (void)
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


/*-----------------------------------------------------------------------------
	Key bindings
-----------------------------------------------------------------------------*/

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


static void Key_Unbind_f (void)
{
	int		i, n;
	char	*mask;
	keyname_t *kn;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: unbind <key mask>\n");
		return;
	}
	mask = Cmd_Argv (1);

	n = 0;
	for (i = 0; i < NUM_KEYS; i++)
	{
		int		j;
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
			for (j = 0, kn = keynames; j < ARRAY_COUNT(keynames); j++, kn++)
				if (i == kn->keynum)
				{
					keyName = kn->name;
					break;
				}
		}

		if (keyName && MatchWildcard2 (keyName, mask, true))
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

	for (i = 0; i < NUM_KEYS; i++)
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
		Com_Printf ("Usage: bind <key> [command]\n");
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
	for (i = 0; i < NUM_KEYS; i++)
		if (keybindings[i] && keybindings[i][0] && (!mask || MatchWildcard2 (keybindings[i], mask, true)))
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

	for (i = 0; i < NUM_KEYS; i++)
		if (keybindings[i] && keybindings[i][0])
			fprintf (f, "bind %s \"%s\"\n", Key_KeynumToString(i), keybindings[i]);
}


/*-----------------------------------------------------------------------------
	Miscellaneous
-----------------------------------------------------------------------------*/

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

	for (i = 0; i < NUM_KEYS; i++)
		keyshift[i] = i;
	for (i = 'a'; i <= 'z'; i++)
		keyshift[i] = i - 'a' + 'A';
	for (i = 0; i < ARRAY_COUNT(keyShifts); i++)
		keyshift[keyShifts[i].base] = keyShifts[i].shift;

	menubound[K_ESCAPE] = true;
	for (i = 0; i < 24; i++)
		menubound[K_F1+i] = true;

	Cmd_AddCommand ("bind", Key_Bind_f);
	Cmd_AddCommand ("unbind", Key_Unbind_f);
	Cmd_AddCommand ("unbindall", Key_Unbindall_f);
	Cmd_AddCommand ("bindlist", Key_Bindlist_f);
}


void Key_Event (int key, qboolean down, unsigned time)
{
	char	*kb;
	int		rep;
	static char plus_cmd_fired[NUM_KEYS];

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

	// any key during the demoplay mode will bring up the menu
	if (cl.attractloop && cls.key_dest == key_game && !(key >= K_F1 && key <= K_F24 || key == K_PAUSE))
		key = K_ESCAPE;

	// menu key is hardcoded, so the user can never unbind it
	if (key == K_ESCAPE)
	{
		if (!down)
			return;

		if (!cl.attractloop && cls.key_dest == key_game && cl.frame.playerstate.stats[STAT_LAYOUTS])
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
		case key_forcemenu:
			M_Keydown (key);
			break;
		case key_game:
		case key_console:
			M_Menu_Main_f ();
			break;
		default:
			Com_Error (ERR_FATAL, "Bad cls.key_dest %d", cls.key_dest);
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

	// process key bindings
	if ((cls.key_dest == key_menu && menubound[key]) ||
		(cls.key_dest == key_console && !consolekeys[key]) ||
		(cls.key_dest == key_game && (cls.state == ca_active || !consolekeys[key])))
		// here: no key_forcemenu code (will never use bindings in this mode)
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
	case key_forcemenu:
		M_Keydown (key);
		break;

	case key_game:
	case key_console:
		Key_Console (key);
		break;
	default:
		Com_Error (ERR_FATAL, "Bad cls.key_dest %d", cls.key_dest);
	}
}


void Key_ClearStates (void)
{
	int		i;

	anykeydown = false;

	for (i = 0; i < NUM_KEYS; i++)
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
