#include "qcommon.h"

//!! should be static
TList<CCommand> CmdList;
TList<CAlias> AliasList;

static int cmdWait;

#define	ALIAS_LOOP_COUNT	64
#define MACRO_LOOP_COUNT	64
static int	aliasCount;		// for detecting runaway loops
static cvar_t *cmd_debug;


static char	*_argv[MAX_STRING_TOKENS];
static int	_argc;

//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
static void Cmd_Wait_f (bool usage, int argc, char **argv)
{
	if (argc > 2 || usage)
	{
		Com_Printf ("Usage: wait [<num_frames>]\n");
		return;
	}
	cmdWait = argc == 2 ? atoi (argv[1]) : 1;
}


/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

#define CBUF_MAXSIZE	8192

static sizebuf_t cmd_text;
static char cmd_text_buf[CBUF_MAXSIZE];

static char defer_text_buf[CBUF_MAXSIZE];

/*
============
Cbuf_Init
============
*/
void Cbuf_Init (void)
{
	SZ_Init (&cmd_text, cmd_text_buf, sizeof(cmd_text_buf));
}

/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText (char *text)
{
	int		len;

	len = strlen (text);

	if (cmd_text.cursize + len >= cmd_text.maxsize)
	{
		Com_WPrintf ("Cbuf_AddText: overflow\n");
		return;
	}
	if (cmd_debug->integer & 2)
	{
		if (len > 256)
			Com_Printf (S_BLUE"AddText: %d chars\n", len);
		else
			Com_Printf (S_BLUE"AddText: \"%s\"\n", text);
	}
	SZ_Write (&cmd_text, text, len);
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
============
*/
void Cbuf_InsertText (char *text)
{
	int		len;

	len = strlen (text);

	if (cmd_text.cursize + len >= cmd_text.maxsize)
	{
		Com_WPrintf ("Cbuf_InsertText: overflow\n");
		return;
	}
	if (cmd_debug->integer & 2)
	{
		if (len > 256)
			Com_Printf (S_BLUE"InsText: %d chars\n", len);
		else
			Com_Printf (S_BLUE"InsText: \"%s\"\n", text);
	}
	SZ_Insert (&cmd_text, text, len, 0);
}


/*
============
Cbuf_CopyToDefer
============
*/
void Cbuf_CopyToDefer (void)
{
	memcpy (defer_text_buf, cmd_text_buf, cmd_text.cursize);
	defer_text_buf[cmd_text.cursize] = 0;
	cmd_text.cursize = 0;
	if (cmd_debug->integer & 2) Com_Printf (S_BLUE"DeferCommands\n");
}

/*
============
Cbuf_InsertFromDefer
============
*/
void Cbuf_InsertFromDefer (void)
{
	if (cmd_debug->integer & 2) Com_Printf (S_BLUE"UndeferCommands: ");	// line will be continued from Cbuf_InsertText()
	Cbuf_InsertText (defer_text_buf);
	defer_text_buf[0] = 0;
}


/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute (void)
{
	int		i, len;
	char	*text;
	char	line[1024];
	int		quotes;

	aliasCount = 0;		// don't allow infinite alias loops

	while (cmd_text.cursize)
	{
		if (cmdWait)
		{
			// skip out while text still remains in buffer, leaving it
			// for next frame
			cmdWait--;
			break;
		}

		// find a \n or ; line break
		text = (char*)cmd_text.data;

		quotes = 0;
		len = cmd_text.cursize;
		for (i = 0; i < cmd_text.cursize; i++)
		{
			if (text[i] == '"')
				quotes ^= 1;

			if (!quotes && text[i] == '/' && i < cmd_text.cursize-1 && text[i+1] == '/')
			{	// remove "//" comments
				len = i;
				while (i < cmd_text.cursize && text[i] != '\n') i++;
				break;
			}
			if (!quotes && text[i] == ';')
			{
				len = i;
				break;	// don't break if inside a quoted string
			}
			if (text[i] == '\n')
			{
				len = i;
				break;
			}
		}

		memcpy (line, text, len);
		line[len] = 0;

		// delete the text from the command buffer and move remaining commands down
		// this is necessary because commands (exec, alias) can insert data at the
		// beginning of the text buffer

		if (i == cmd_text.cursize)
			cmd_text.cursize = 0;
		else
		{
			i++;
			cmd_text.cursize -= i;
			memmove (text, text+i, cmd_text.cursize);
		}

		// execute the command line
		if (!Cmd_ExecuteString (line))
			// send it as a server command if we are connected
			Cmd_ForwardToServer (_argc, _argv);
	}
}


/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/


/*
===============
Cmd_Exec_f
===============
*/
static void Cmd_Exec_f (bool usage, int argc, char **argv)
{
	char	*f;

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: exec <filename>\n");
		return;
	}

	if (!(f = (char*) FS_LoadFile (argv[1])))
	{
		Com_WPrintf ("Couldn't exec %s\n", argv[1]);
		return;
	}
	Com_Printf ("Execing %s\n", argv[1]);

	Cbuf_InsertText (f);

	FS_FreeFile (f);
}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
static void Cmd_Echo_f (int argc, char **argv)
{
	for (int i = 1; i < argc; i++)
		Com_Printf ("%s ", argv[i]);
	Com_Printf ("\n");
}

/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ";"-separated)
===============
*/
static void Cmd_Alias_f (bool usage, int argc, char **argv)
{
	char	cmd[1024];
	char	*name;
	CAlias	*a, *pos;

	if (argc == 2 || usage)
	{
		Com_Printf ("Usage: alias [<name> <value>]\n");
		return;
	}

	if (argc == 1)
	{
		Com_Printf ("Current alias commands:\n");
		for (a = AliasList.First(); a; a = AliasList.Next(a))
			Com_Printf ("%s \"%s\"\n", a->name, a->value);
		return;
	}

	name = argv[1];

	// if the alias already exists, reuse it
	a = AliasList.Find (name, &pos);
	if (a)
		appFree (a->value);
	else
	{
		a = new (name) CAlias;
		AliasList.InsertAfter (a, pos);
	}

	// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (int i = 2; i < argc; i++)
	{
		strcat (cmd, argv[i]);
		if (i != (argc - 1))
			strcat (cmd, " ");
	}

	a->value = CopyString (cmd);
}


void Cmd_Unalias_f (bool usage, int argc, char **argv)
{
	CAlias *alias, *next;
	int		n;

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: unalias <mask>\n");
		return;
	}

	n = 0;
	for (alias = AliasList.First(); alias; alias = next)
	{
		next = AliasList.Next(alias);
		if (appMatchWildcard (alias->name, argv[1], true))
		{
			AliasList.Remove(alias);		// will scan list again
			delete alias;
			n++;
		}
	}
	Com_Printf ("%d aliases removed\n", n);
}


void Cmd_WriteAliases (FILE *f)
{
	for (CAlias *alias = AliasList.First(); alias; alias = AliasList.Next(alias))
		fprintf (f, "alias %s %s\n", alias->name, COM_QuoteString (alias->value, true));
}


/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

/*
======================
Cmd_MacroExpandString
======================
*/
static char *MacroExpandString (const char *text)
{
	static char	buf[MAX_STRING_CHARS];			// will contain result

	int len = strlen (text);
	if (len >= sizeof(buf))
	{
		Com_WPrintf ("Line exceeded "STR(MAX_STRING_CHARS)" chars, discarded\n");
		return NULL;
	}

	char *s1 = strchr (text, '=');
	char *s2 = strchr (text, '\"');
	if (s1 && (!s2 || s2 > s1) &&							// a=b, but '=' not inside quotes
		(s1 > text && s1[-1] != ' ' && s1[1] != ' '))		// ingnore "a = b" (may be "bind = scr_sizeup" etc; should change keyname ??)
	{
		// convert to "set a b"
		strcpy (buf, "set ");
		appStrncpyz (buf + 4, text, s1 - text + 1);	// copy "a"
		int i = strlen (buf);
		appSprintf (buf + i, sizeof(buf) - i,
			s1[1] != '\"' ? " \"%s\"\n" : " %s\n",
			s1 + 1);
	}
	else
		memcpy (buf, text, len+1);

	int count = 0;									// to prevent infinite recurse
	int quotes = 0;

	for (char *s = buf; *s; s++)
	{
		char	*data, *token;
		char	tmp[MAX_STRING_CHARS];
		int		varLen;

		if (*s == '"') quotes ^= 1;
		if (quotes) continue;					// don't expand inside quotes
		if (*s != '$') continue;

		// scan out the complete macro
		data = s + 1;							// $varname -> varname
		token = COM_Parse (data);
		if (!data) continue;					// null token

		token = Cvar_VariableString (token);
		varLen = strlen (token);

		len = len - (data - s) + varLen;		// update length for overflow checking
		if (len >= sizeof(buf))
		{
			Com_WPrintf ("Expanded line exceeded "STR(MAX_STRING_CHARS)" chars, discarded\n");
			return NULL;
		}

		strcpy (tmp, data);						// save tail
		memcpy (s, token, varLen);				// strcpy
		strcpy (s + varLen, tmp);				// append tail
		s--;									// continue from the same position

		if (++count == MACRO_LOOP_COUNT)
		{
			Com_WPrintf ("Macro expansion loop, discarded\n");
			return NULL;
		}
	}

	if (quotes)
	{
		Com_WPrintf ("Line has unmatched quote, discarded.\n");
		if (cmd_debug->integer)
			Com_Printf ("bad str: <%s> -> <%s>\n", text, buf);
		return NULL;
	}

	return buf;
}


/*
============
TokenizeString

Parses the given string into command line tokens.
============
*/
static void TokenizeString (const char *text)
{
	int		i;
	char	*com_token;

	// clear the args from the last string
	for (i = 0; i < _argc; i++)
		appFree (_argv[i]);
	_argc = 0;

	if (!text) return;

	// split command line
	while (true)
	{
		// skip spaces and tabs (char in [1..32]); stop on non-space, null or '\n'
		while (*text && *text <= ' ' && *text != '\n') text++;

		if (!text[0] || text[0] == '\n')
			break;

		com_token = COM_Parse (text);
		if (!text) break;

		if (_argc < ARRAY_COUNT(_argv))
			_argv[_argc++] = CopyString (com_token);
	}

	// set rest of argv[] array to ""
	for (i = _argc; i < ARRAY_COUNT(_argv); i++)
		_argv[i] = "";
}


/*-----------------------------------------------------------------------------
	Command management
-----------------------------------------------------------------------------*/


bool RegisterCommand (const char *name, void(*func)(), int flags)
{
	CCommand *cmd, *pos;

	guard(RegisterCommand);
	if (CmdList.Find (name, &pos))
	{
		appWPrintf ("RegisterCommand: \"%s\" is already registered\n", name);
		return false;
	}
	cmd = new (name) CCommand;
	cmd->flags = flags;
	cmd->func = func;
	CmdList.InsertAfter (cmd, pos);

	return true;
	unguard;
}


bool UnregisterCommand (const char *name)
{
	guard(UnregisterCommand);
	CCommand *cmd = CmdList.Find (name);
	if (!cmd)
	{
		appWPrintf ("UnregisterCommand: \"%s\" is not registered\n", name);
		return false;
	}
	CmdList.Remove (cmd);
	delete cmd;

	return true;
	unguard;
}


/*
============
Cmd_ExecuteString

$Cvars will be expanded unless they are in a quoted token
============
*/
bool Cmd_ExecuteString (const char *text)
{
	guard(Cmd_ExecuteString);

	TokenizeString (MacroExpandString (text));

	// execute the command line
	if (!_argc) return true;		// no tokens

	if (cmd_debug->integer & 1)
		Com_Printf (S_CYAN"cmd: %s\n", text);

	// check functions
	CCommand *cmd = CmdList.Find (_argv[0]);
	if (cmd)
	{
		if (!cmd->func)
		{	// forward to server command
			Cmd_ExecuteString (va("cmd %s", text));
		}
		else
		{
			bool usage = _argc == 2 && !strcmp (_argv[1], "/?");
			guard(cmd);
			switch (cmd->flags)
			{
			case 0:
				cmd->func ();
				break;
			case COMMAND_USAGE:
				((void (*) (bool)) cmd->func) (usage);
				break;
			case COMMAND_ARGS:
				((void (*) (int, char**)) cmd->func) (_argc, _argv);
				break;
			case COMMAND_USAGE|COMMAND_ARGS:
				((void (*) (bool, int, char**)) cmd->func) (usage, _argc, _argv);
				break;
			}
			unguardf(("%s", cmd->name));
		}
		return true;
	}

	// check alias
	CAlias *a = AliasList.Find (_argv[0]);
	if (a)
	{
		if (++aliasCount == ALIAS_LOOP_COUNT)
		{
			Com_WPrintf ("ALIAS_LOOP_COUNT\n");
			return true;
		}
		Cbuf_InsertText (va("%s\n", a->value));
		return true;
	}

	// check cvars
	if (Cvar_Command (_argc, _argv))
		return true;

	unguard;
	return false;
}


bool ExecuteCommand (const char *str, const CSimpleCommand *CmdList, int numCommands)
{
	guard(ExecuteSimpleCommand);
//	GetArgs (str, false);
	TokenizeString (str);
	for (int i = 0; i < numCommands; i++, CmdList++)
		if (!stricmp (CmdList->name, _argv[0]))
		{
			guard(cmd)
			CmdList->func (_argc, _argv);
			return true;
			unguardf(("%s", CmdList->name))
		}
	return false;
	unguard;
}


/*
============
Cmd_List_f
============
*/
static void Cmd_List_f (bool usage, int argc, char **argv)
{
	int		n, total;
	char	*mask;

	if (argc > 2 || usage)
	{
		Com_Printf ("Usage: cmdlist [<mask>]\n");
		return;
	}

	if (argc == 2)
		mask = argv[1];
	else
		mask = NULL;

	n = total = 0;
	Com_Printf ("----i-a-name----------------\n");
	for (CCommand *cmd = CmdList.First(); cmd; cmd = CmdList.Next(cmd))
	{
		total++;
		if (mask && !appMatchWildcard (cmd->name, mask, true)) continue;
		Com_Printf ("%-3d %c %c %s\n", total,
			cmd->flags & COMMAND_USAGE ? 'I' : ' ',
			cmd->flags & COMMAND_ARGS ? 'A' : ' ',
			cmd->name);
		n++;
	}
	Com_Printf ("Displayed %d/%d commands\n", n, total);
}

/*
============
Cmd_Init
============
*/
void Cmd_Init (void)
{
	RegisterCommand ("cmdlist", Cmd_List_f);
	RegisterCommand ("exec", Cmd_Exec_f);
	RegisterCommand ("echo", Cmd_Echo_f);
	RegisterCommand ("alias", Cmd_Alias_f);
	RegisterCommand ("unalias", Cmd_Unalias_f);
	RegisterCommand ("wait", Cmd_Wait_f);
	cmd_debug = Cvar_Get ("cmd_debug", "0", 0);
}
