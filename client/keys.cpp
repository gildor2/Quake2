#include "client.h"
#include "qmenu.h"						// for completion menu only


//----------- keyboard state --------------
int		keysDown;
bool	keydown[NUM_KEYS];
static int		key_repeats[NUM_KEYS];	// if > 1, it is autorepeating

//------------- bindings ------------------
// each part is NUM_KEYS size: 1) normal keys 2) Ctrl+Key 3) Alt+Key
#define NUM_BINDINGS	(NUM_KEYS*3)
static char		*keybindings[NUM_BINDINGS];

//----------- some modifiers --------------
static int		keyshift[NUM_KEYS];		// key to map to if shift held down in console


/*-----------------------------------------------------------------------------
	Support for key names
	?? should check for correct UNKxx keys: UNK30 is "correct", but will newer be pressed
-----------------------------------------------------------------------------*/

typedef struct
{
	char	*name;
	int		keynum;
} keyname_t;

//?? make a table keyNames[NUM_KEYS] for faster searching (can store BAD_KEY mark in this table)
static const keyname_t keynames[] =
{
	{"Tab",			K_TAB},
	{"Enter",		K_ENTER},
	{"Escape",		K_ESCAPE},
	{"Space",		K_SPACE},
	{"BackSpace",	K_BACKSPACE},
	{"UpArrow",		K_UPARROW},
	{"DownArrow",	K_DOWNARROW},
	{"LeftArrow",	K_LEFTARROW},
	{"RightArrow",	K_RIGHTARROW},

	{"Alt",			K_ALT},			{"RAlt",		K_RALT},
	{"Ctrl",		K_CTRL},		{"RCtrl",		K_RCTRL},
	{"Shift",		K_SHIFT},

	{"F1",		K_F1},	{"F2",		K_F2},	{"F3",		K_F3},	{"F4",		K_F4},
	{"F5",		K_F5},	{"F6",		K_F6},	{"F7",		K_F7},	{"F8",		K_F8},
	{"F9",		K_F9},	{"F10",		K_F10},	{"F11",		K_F11},	{"F12",		K_F12},
	{"F13",		K_F13},	{"F14",		K_F14},	{"F15",		K_F15},	{"F16",		K_F16},
	{"F17",		K_F17},	{"F18",		K_F18},	{"F19",		K_F19},	{"F20",		K_F20},
	{"F21",		K_F21},	{"F22",		K_F22},	{"F23",		K_F23},	{"F24",		K_F24},

	{"Ins",			K_INS},
	{"Del",			K_DEL},
	{"PgDn",		K_PGDN},
	{"PgUp",		K_PGUP},
	{"Home",		K_HOME},
	{"End",			K_END},

	// keypad
	{"KP_Ins",		K_KP_INS},
	{"KP_End",		K_KP_END},
	{"KP_DownArrow",K_KP_DOWNARROW},
	{"KP_PgDn",		K_KP_PGDN},
	{"KP_LeftArrow",K_KP_LEFTARROW},
	{"KP_5",		K_KP_5},
	{"KP_RightArrow",K_KP_RIGHTARROW},
	{"KP_Home",		K_KP_HOME},
	{"KP_UpArrow",	K_KP_UPARROW},
	{"KP_PgUp",		K_KP_PGUP},
	{"KP_Del",		K_KP_DEL},
	{"KP_Slash",	K_KP_SLASH},
	{"KP_Star",		K_KP_STAR},
	{"KP_Minus",	K_KP_MINUS},
	{"KP_Plus",		K_KP_PLUS},
	{"KP_Enter",	K_KP_ENTER},

	// mouse
	{"Mouse1",	K_MOUSE1},	{"Mouse2",	K_MOUSE2},	{"Mouse3",	K_MOUSE3},	{"Mouse4",	K_MOUSE4},
	{"Mouse5",	K_MOUSE5},	{"Mouse6",	K_MOUSE6},	{"Mouse7",	K_MOUSE7},	{"Mouse8",	K_MOUSE8},
	{"MWheelUp",	K_MWHEELUP},	{"MWheelDown",	K_MWHEELDOWN},

	// joystick
	{"Joy1",	K_JOY1},	{"Joy2",	K_JOY2},	{"Joy3",	K_JOY3},	{"Joy4",	K_JOY4},

	{"Aux1",	K_AUX1},	{"Aux2",	K_AUX2},	{"Aux3",	K_AUX3},	{"Aux4",	K_AUX4},
	{"Aux5",	K_AUX5},	{"Aux6",	K_AUX6},	{"Aux7",	K_AUX7},	{"Aux8",	K_AUX8},
	{"Aux9",	K_AUX9},	{"Aux10",	K_AUX10},	{"Aux11",	K_AUX11},	{"Aux12",	K_AUX12},
	{"Aux13",	K_AUX13},	{"Aux14",	K_AUX14},	{"Aux15",	K_AUX15},	{"Aux16",	K_AUX16},
	{"Aux17",	K_AUX17},	{"Aux18",	K_AUX18},	{"Aux19",	K_AUX19},	{"Aux20",	K_AUX20},
	{"Aux21",	K_AUX21},	{"Aux22",	K_AUX22},	{"Aux23",	K_AUX23},	{"Aux24",	K_AUX24},
	{"Aux25",	K_AUX25},	{"Aux26",	K_AUX26},	{"Aux27",	K_AUX27},	{"Aux28",	K_AUX28},
	{"Aux29",	K_AUX29},	{"Aux30",	K_AUX30},	{"Aux31",	K_AUX31},	{"Aux32",	K_AUX32},

	{"Pause",		K_PAUSE},
	{"CapsLock",	K_CAPSLOCK},
	{"NumLock",		K_NUMLOCK},
	{"ScrLock",		K_SCRLOCK},
	{"PrintScrn",	K_PRINTSCRN},	//?? sent only when key is up

	{"Comma",		','},		// because comma used for separating multiple wildcards
	{"Period",		'.'},
	{"Semicolon",	';'},		// because a raw semicolon separates commands
	{"Minus",		'-'},
	{"Equals",		'='},
	{"LeftBracket",	'['},
	{"RigntBracket",']'},
	{"Slash",		'/'},
	{"BackSlash",	'\\'}
	//?? add:  ' (UT name: SingleQuote)
};


// NOTE: 'c' is lowercase digit
static int HexDigit (char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	else return -1;			// bad digit
}


static int Key_StringToKeynum (const char *str)
{
	int		i, mod;
	const keyname_t *kn;
	char	name[32], *key;

	if (!str || !str[0]) return -1;			// bad string

	appStrncpylwr (name, str, sizeof(name));
	if (!memcmp (name, "alt+", 4))
	{
		key = name + 4;
		mod = MOD_ALT;
	}
	else if (!memcmp (name, "ctrl+", 5))
	{
		key = name + 5;
		mod = MOD_CTRL;
	}
	else
	{
		key = name;
		mod = 0;
	}

	if (!key[1]) return key[0] + mod;		// simple key

	// check for 'UNKxx'
	if (key[0] == 'u' && key[1] == 'n' && key[2] == 'k' && key[5] == 0)
	{
		int n1 = HexDigit (name[3]);
		if (n1 < 0) return -1;
		int n2 = HexDigit (name[4]);
		if (n2 < 0) return -1;
		return (n1 << 4) + n2 + 256 + mod;
	}

	for (i = 0, kn = keynames; i < ARRAY_COUNT(keynames); i++, kn++)
	{
		if (!stricmp (key, kn->name))		// names are mixed case
			return kn->keynum + mod;
	}
	return -1;
}


const char *Key_KeynumToString (int keynum)
{
	if (keynum == -1)
		return S_RED"<KEY NOT FOUND>";

	const char *pref;
	if (keynum >= MOD_ALT)
	{
		keynum -= MOD_ALT;
		pref = "Alt+";
	}
	else if (keynum >= MOD_CTRL)
	{
		keynum -= MOD_CTRL;
		pref = "Ctrl+";
	}
	else
		pref = "";

	// alphanumeric keys
	if (keynum >= '0' && keynum <= '9' || keynum >= 'a' && keynum <= 'z' || keynum >= 'A' && keynum <= 'Z')
		return va("%s%c", pref, keynum);

	// unknown keys
	if (keynum >= 256)
		return va("%sUnk%02X", pref, keynum - 256);

	int		i;
	const keyname_t *kn;
	// named keys
	for (i = 0, kn = keynames; i < ARRAY_COUNT(keynames); i++, kn++)
	{
		if (keynum == kn->keynum)
			return va("%s%s", pref, kn->name);
	}

	// other keys
	if (keynum >= 32 && keynum < 128)	// visible ASCII codes
		return va("%s%c", pref, keynum);

	// no name found for key
	return S_RED"<UNKNOWN KEYNUM>";
}


/*-----------------------------------------------------------------------------
	Command completion in console
-----------------------------------------------------------------------------*/

//!! check for overflow of this buffers
static char complete_command[256], completed_name[512], *partial_name;
static int completed_count, partial_len;

#define MAX_COMPLETE_ITEMS	16
static char completeVariants[MAX_COMPLETE_ITEMS][256];


static void TryComplete (const char *full, int display, char mark)
{
	if (strnicmp (partial_name, full, partial_len)) return;

	if (display)
		appPrintf ("  "S_GREEN"%c"S_WHITE"  %s\n", mark, full);

	if (!completed_count)	// have not yet completed - just copy string
		strcpy (completed_name, full);
	else					// already completed - refine string
	{
		char *s = completed_name;
		const char *d = full;
		while (*s == *d++) s++;
		*s = 0;				// limit with last matched char
	}

	if (completed_count < MAX_COMPLETE_ITEMS)
		strcpy (completeVariants[completed_count], full);
	completed_count++;
}


static char *Do_CompleteCommand (char *partial)
{
	char	*arg1s, arg1[256], *arg2s;	// arg1s -> "arg1 arg2 ..."; arg2s -> "arg2 arg3 ..."

	complete_command[0] = 0;
	completed_name[0] = 0;
	completed_count = 0;

	/*------ complete argument for variable/map/demomap -------*/

	if (arg1s = strchr (partial, ' '))
	{
		if (arg1s == partial)
			return NULL;	// space is first line char!

		int len = arg1s - partial;
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

//		appPrintf ("cmd: \"%s\"  arg1s: \"%s\"  arg2s: \"%s\"  arg1: \"%s\"\n",command, arg1s, arg2s, arg1);

		if (arg2s)
		{
			if (arg2s[0]) return NULL;		// have non-zero 2nd argument

			// have "command arg1 " (space after arg1)

			// complete "alias name "
			if (!stricmp (complete_command, "alias"))
			{
				for (TListIterator<CAlias> alias(AliasList); alias; ++alias)
					if (!stricmp (alias->name, arg1))
					{
						strcpy (completed_name, va("alias %s %s", arg1, COM_QuoteString (alias->value, false)));
						return completed_name;
					}
				return NULL;
			}
			// complete "bind key "
			else if (!stricmp (complete_command, "bind"))
			{
				int key = Key_StringToKeynum (arg1);
				if (key < 0 || !keybindings[key])
					return NULL;

				strcpy (completed_name, va("bind %s %s", arg1, COM_QuoteString (keybindings[key], false)));
				return completed_name;
			}
		}

		if (!stricmp (complete_command, "alias"))
		{
			// complete alias name
			strcpy (complete_command, "alias");
			for (int display = 0; display < 2; display++)
			{
				partial_name = arg1s;
				partial_len = strlen (arg1s);
				completed_count = 0;
				for (TListIterator<CAlias> alias(AliasList); alias; ++alias)
					TryComplete (alias->name, display, 'a');
				if (!completed_count)
					return NULL;

				if (completed_count == 1)
				{
					strcat (completed_name, " ");
					break;
				}
				if (!display)
					appPrintf ("]/%s\n", partial);
			}

			strcpy (completed_name, va("%s %s", complete_command, completed_name));
			return completed_name;
		}

		char comp_type;
		int file_type;
		const char *path, *ext;
		if (!stricmp (complete_command, "map"))
		{
			path = "maps";
			ext = ".bsp";
			comp_type = 'm';
			file_type = FS_FILE|FS_NOEXT;
		}
		else if (!stricmp (complete_command, "demomap"))
		{
			path = "demos";
			ext = ".dm2";
			comp_type = 'd';
			file_type = FS_FILE|FS_NOEXT;
		}
		else if (!stricmp (complete_command, "game"))
		{
			path = "..";
			ext = "";
			comp_type = 'g';
			file_type = FS_DIR|FS_OS;
		}
		else // try to complete varname with its value
		{
			if (*arg1s)
				return NULL;				// arg is not empty

			for (cvar_t *cvar = cvar_vars; cvar; cvar = cvar->next)
				if (!stricmp (cvar->name, complete_command))
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
		CFileList *filelist = GFileSystem->List (va("%s/*%s", path, ext), file_type);
		if (!*filelist)
		{
			delete filelist;
			return NULL;
		}

		partial_name = arg1;
		partial_len = strlen (arg1);
		for (int display = 0; display < 2; display++)
		{
			completed_count = 0;
			for (TListIterator<CFileItem> fileitem = *filelist; fileitem; ++fileitem)
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
				appPrintf ("]/%s\n", partial);
			}
		}
		strcpy (completed_name, va("%s %s", complete_command, completed_name));
		delete filelist;
		return completed_name;
	}

	/*--------- complete command/variable/alias name ------------*/

	partial_len = strlen (partial);
	if (!partial_len) return NULL;
	partial_name = partial;

	for (int display = 0; display < 2; display++)
	{
		completed_count = 0;

		// check for partial match
		for (TListIterator<CCommand> cmd = CmdList; cmd; ++cmd)
			TryComplete (cmd->name, display, 'c');
		for (TListIterator<CAlias> a = AliasList; a; ++a)
			TryComplete (a->name, display, 'a');
		for (cvar_t *var = cvar_vars; var; var = var->next)
			TryComplete (var->name, display, 'v');

		if (!completed_count) return NULL; // not completed

		if (completed_count == 1)
		{	// only one match
			strcat (completed_name, " ");
			return completed_name;
		}
		// many matches
		if (!display)
		{	// at this loop we just see, that we have many matches ...
			appPrintf ("]/%s\n", partial);
		}
	}
	return completed_name;
}


/*--------------- Complete menu --------------------*/

#define CHAR_WIDTH	8
#define CHAR_HEIGHT	8

struct completeMenu_t : menuFramework_t
{
	menuAction_t completeItems[MAX_COMPLETE_ITEMS];
	int		complMenu_x, complMenu_y, complMenu_w, complMenu_h;

	void Draw ()
	{
		RE_Fill (complMenu_x, complMenu_y, complMenu_w, complMenu_h, RGBA(0.1,0.5,0.5,0.8));
		menuFramework_t::Draw ();
	}

	const char *KeyDown (int key)
	{
		const char *res = menuFramework_t::KeyDown (key);
		if (m_current != this)
		{
			// "ESCAPE" (or any other menu-exit-key) pressed
			cls.key_dest = key_console;
			cls.keep_console = false;
		}
		return res;
	}

	static void CompleteMenuCallback (void *item)
	{
		char *s = completeVariants[((menuAction_t*) item)->localData];
		m_current->Pop ();

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

	bool Init ()
	{
		nitems = 0;
		cursor = 0;
		int maxLen = 0;
		int		i, y1;
		for (i = 0, y1 = 0; i < completed_count; i++, y1 += 10)
		{
			MENU_ACTION(completeItems[i], y1, completeVariants[i], CompleteMenuCallback, NULL);
			completeItems[i].flags = QMF_LEFT_JUSTIFY;
			completeItems[i].localData = i;
			AddItem (&completeItems[i]);
			int len = strlen (completeVariants[i]);
			if (len > maxLen) maxLen = len;
		}

		complMenu_h = completed_count * 10 + CHAR_HEIGHT;
		complMenu_w = (maxLen + 3) * CHAR_WIDTH + 4;
		x = editPos * CHAR_WIDTH + CHAR_WIDTH;
		if (x + complMenu_w > viddef.width)
			x = viddef.width - complMenu_w;
		complMenu_x = x - 28;
		y = con_height - 22;
		if (y + complMenu_h > viddef.height)
			y = viddef.height - complMenu_h;
		complMenu_y = y - 4;

		cls.keep_console = true;
		return true;
	}
};
static completeMenu_t completeMenu;


void CompleteCommand (void)
{
	guard(CompleteCommand);

	char *s = &editLine[1];
	if (*s == '\\' || *s == '/')
		s++;

	if (!strcmp (completed_name, s) && completed_count > 1 && completed_count <= MAX_COMPLETE_ITEMS && !DEDICATED)
	{
		completeMenu.Push ();
		return;
	}

	char *cmd = Do_CompleteCommand (s);
	if (cmd)
	{
		editLine[1] = '/';
		strcpy (editLine + 2, cmd);
		editPos = strlen (cmd) + 2;
		editLine[editPos] = 0;
	}

	unguard;
}


/*-----------------------------------------------------------------------------
	Key bindings
-----------------------------------------------------------------------------*/

void Key_SetBinding (int keynum, const char *binding)
{
	if (keynum == -1)
		return;

	// free old bindings
	if (keybindings[keynum])
	{
		appFree (keybindings[keynum]);
		keybindings[keynum] = NULL;
	}

	if (binding && binding[0])
		keybindings[keynum] = CopyString (binding);
}


int Key_FindBinding (const char *str, int *keys, int maxKeys)
{
	int numKeys = 0;
	for (int i = 0; i < NUM_BINDINGS; i++)
		if (keybindings[i] && !stricmp (keybindings[i], str))
		{
			if (numKeys < maxKeys)
				keys[numKeys] = i;		// return correct number of bound keys even when maxKeys is too small
			numKeys++;
		}
	return numKeys;
}


static void Key_Unbind_f (bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		appPrintf ("Usage: unbind <key mask>\n");
		return;
	}
	const char *mask = argv[1];

	int n = 0;
	bool found = false;
	for (int i = 0; i < NUM_BINDINGS; i++)
	{
		const char *keyName = Key_KeynumToString (i);
		if (appMatchWildcard (keyName, mask, true))
		{
			found = true;
			if (keybindings[i])
			{
				Com_DPrintf ("unbind %s\n", keyName);
				Key_SetBinding (i, NULL);
				n++;
			}
		}
	}

	if (found)
		appPrintf ("%d key(s) unbound\n", n);
	else
		appWPrintf ("\"%s\" isn't a valid key\n", mask);
}


static void Key_Unbindall_f (void)
{
	for (int i = 0; i < NUM_BINDINGS; i++)
		if (keybindings[i])
			Key_SetBinding (i, NULL);
}


static void Key_Bind_f (bool usage, int argc, char **argv)
{
	if (argc < 2 || usage)
	{
		appPrintf ("Usage: bind <key> [command]\n");
		return;
	}
	int b = Key_StringToKeynum (argv[1]);
	if (b == -1)
	{
		appWPrintf ("\"%s\" isn't a valid key\n", argv[1]);
		return;
	}

	if (argc == 2)
	{	// just print current binding
		if (keybindings[b])
			appPrintf ("\"%s\" = \"%s\"\n", argv[1], keybindings[b] );
		else
			appPrintf ("\"%s\" is not bound\n", argv[1] );
		return;
	}

	// copy the rest of the command line
	char cmd[1024];
	cmd[0] = 0;		// start out with a null string
	for (int i = 2; i < argc; i++)
	{
		strcat (cmd, argv[i]);
		if (i != argc-1) strcat (cmd, " ");
	}

	Key_SetBinding (b, cmd);
}


static void Key_Bindlist_f (bool usage, int argc, char **argv)
{
	if (argc > 2 || usage)
	{
		appPrintf ("Usage: bindlist [<action mask>]\n");
		return;
	}

	const char *mask = argc == 2 ? argv[1] : NULL;

	int n = 0;
	appPrintf ("---key----action---------\n");
	for (int i = 0; i < NUM_BINDINGS; i++)
		if (keybindings[i] && (!mask || appMatchWildcard (keybindings[i], mask, true)))
		{
			n++;
			appPrintf (S_YELLOW"%-9s "S_WHITE"%s\n", Key_KeynumToString(i), keybindings[i]);
		}
	if (mask)
		appPrintf ("    %d binds found\n", n);
}


/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings (COutputDevice *Out)
{
	for (int i = 0; i < NUM_BINDINGS; i++)
		if (keybindings[i] && keybindings[i][0])
			Out->Printf ("bind %s %s\n", Key_KeynumToString (i), COM_QuoteString (keybindings[i], true));
}


/*-----------------------------------------------------------------------------
	Miscellaneous
-----------------------------------------------------------------------------*/

void Key_Init (void)
{
	static const struct {char base; char shift;} keyShifts[] = {
		{'1', '!'},	{'2', '@'},	{'3', '#'},	{'4', '$'},	{'5', '%'},
		{'6', '^'},	{'7', '&'},	{'8', '*'},	{'9', '('},	{'0', ')'},
		{'-', '_'},	{'=', '+'},	{',', '<'},	{'.', '>'},	{'/', '?'},
		{';', ':'},	{'\'', '"'},{'[', '{'},	{']', '}'},	{'`', '~'},
		{'\\', '|'}
	};
	int		i;

	for (i = 0; i < ARRAY_COUNT(keyShifts); i++)
		keyshift[i] = i;
	for (i = 'a'; i <= 'z'; i++)
		keyshift[i] = i - 'a' + 'A';
	for (i = 0; i < ARRAY_COUNT(keyShifts); i++)
		keyshift[keyShifts[i].base] = keyShifts[i].shift;

	RegisterCommand ("bind", Key_Bind_f);
	RegisterCommand ("unbind", Key_Unbind_f);
	RegisterCommand ("unbindall", Key_Unbindall_f);
	RegisterCommand ("bindlist", Key_Bindlist_f);
}


void Key_Event (int key, bool down)
{
	int		modKey, bindKey, rep;
	char	*kb;
	static bool plus_cmd_fired[NUM_BINDINGS];		//?? make global, clear with Key_ClearStates(); rename
	static int modKeyDown;							//?? global
	bool	skip;

	// update auto-repeat status
	if (down)
		rep = ++key_repeats[key];
	else
		key_repeats[key] = rep = 0;

	// '~' ('`') key is hardcoded
	if (key == '`')
	{
		if (rep == 1) SCR_ToggleConsole ();
		return;
	}

	// any key during the demoplay mode will act as ESC
	if (cl.attractloop && cls.key_dest == key_game && !(key >= K_F1 && key <= K_F24 || key == K_PAUSE))
		key = K_ESCAPE;

	// ESCAPE key is hardcoded
	if (key == K_ESCAPE)
	{
		if (rep != 1) return;

		if (!cl.attractloop && cls.key_dest == key_game && cl.frame.playerstate.stats[STAT_LAYOUTS])
		{
			// put away help computer / inventory (need another way ??)
			Cbuf_AddText ("cmd putaway\n");
			return;
		}
		if (/* cl.attractloop && */ cl.cinematicActive)
		{
			SCR_StopCinematic ();		// stop cinematic with ESC while playing game or with any key when playing demo
			return;
		}
		switch (cls.key_dest)
		{
		case key_message:
			Key_Message (key);
			break;
		case key_menu:
		case key_bindingMenu:
			M_Keydown (key);
			break;
		case key_game:
			M_Menu_Main_f ();
			break;
		case key_console:
			SCR_ShowConsole (false, false);
			break;
		default:
			Com_FatalError ("Bad cls.key_dest %d", cls.key_dest);
		}
		return;
	}

	// make a delay before autorepeat
	if (rep > 1 && rep < 5) return;

	// track number of keys down
	keydown[key] = down;
	if (down)
	{
		if (key_repeats[key] == 1)
			keysDown++;
	}
	else
		if (--keysDown < 0) keysDown = 0;

	// process Ctrl+Key and Alt+Key
	bindKey = modKey = key;
	if (down && keysDown == 2 && key != K_CTRL && key != K_RCTRL && key != K_ALT && key != K_RALT)
	{
		if (CTRL_PRESSED)
			modKey = key + MOD_CTRL;
		else if (ALT_PRESSED)
			modKey = key + MOD_ALT;
		else
			modKey = key;
		bindKey = modKey;

		if (!keybindings[bindKey])
			bindKey = key;								// no binding for modified key -- use unmodified
		if (bindKey != key)
			modKeyDown = bindKey;
	}
	if (modKeyDown && (!down || bindKey == key))		// unpress previously pressed Alt/Ctrl+Key
	{
		modKey = bindKey = modKeyDown;
		modKeyDown = 0;
		down = false;									// force "up" even when +Ctrl/Alt > +Key > +AnotherKey
	}
	kb = keybindings[bindKey];

	// no autorepeats for "+cmd"-like bindings
	if (down && (cls.key_dest == key_game && cls.state != ca_disconnected) && rep > 1 && kb && kb[0] == '+')
		return;

	skip = false;
	if (cls.key_dest == key_bindingMenu)
	{
		bool needDown = true;
		if (key == K_CTRL || key == K_RCTRL || key == K_ALT || key == K_RALT)
			needDown = false;
		if (down == needDown)
			M_Keydown (modKey);
		skip = true;									// cls.key_dest was modified in M_Keydown() - so, store internal flag
	}

	// key up events only generate commands if the game key binding is
	// a button command (leading '+').  These will occur even in console mode,
	// to keep the character from continuing an action started before a console
	// switch.  Button commands include the kenum as a parameter, so multiple
	// downs can be matched with ups
	unsigned time = appMilliseconds ();

	if (!down)
	{
		if (!plus_cmd_fired[bindKey])
			return;
		plus_cmd_fired[bindKey] = false;

		if (kb && kb[0] == '+')
			Cbuf_AddText (va("-%s %d %d\n", kb+1, bindKey, time));
		return;
	}

	if (skip) return;									// completed

	/*---------- at this point, we have only down==true events ... -----------*/

	// process key bindings
	if ((cls.key_dest == key_menu && key >= K_F1 && key <= K_F24) ||	// in menus only Fnn keys used for bindings
		(cls.key_dest == key_console && key >= FIRST_NONCONSOLE_KEY) ||	// in console
		(cls.key_dest == key_game && (cls.state == ca_active || key >= FIRST_NONCONSOLE_KEY)))	// in game or non-game console
	{
		if (kb)
		{
			if (kb[0] == '+')
			{	// button commands add keynum and time as a parm
				plus_cmd_fired[bindKey] = true;		// signal, that command "+cmd" started (for "-cmd" allowing)
				Cbuf_AddText (va("%s %d %d\n", kb, bindKey, time));
			}
			else
			{
				Cbuf_AddText (kb);
				Cbuf_AddText ("\n");
			}
		}
		return;
	}

	if (SHIFT_PRESSED)
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
		Key_Console (key, modKey);
		break;
	default:
		Com_FatalError ("Bad cls.key_dest %d", cls.key_dest);
	}
}


void Key_ClearStates (void)
{
	for (int key = 0; key < NUM_KEYS; key++)
	{
//		if (key == '`' || key == K_ESCAPE) continue;	// hardcoded keys - do not clear states
		if (keydown[key])
			Key_Event (key, false);
		key_repeats[key] = 0;
	}
	keysDown = 0;
}
