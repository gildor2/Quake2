#include "qcommon.h"

//!! should be static
TList<CCommand> CmdList;
TList<CAlias> AliasList;

static int cmdWait;

#define	ALIAS_LOOP_COUNT	64
#define MACRO_LOOP_COUNT	64
static int	aliasCount;		// for detecting runaway loops
static cvar_t *cmd_debug;

// for Mingw32 compatibility
#define _argc	cmd_argc
#define _argv	cmd_argv

static char	*_argv[MAX_STRING_TOKENS];
static int	_argc;


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
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText(const char *text)
{
	int len = strlen(text);

	if (cmd_text.cursize + len >= cmd_text.maxsize)
	{
		appWPrintf("Cbuf_AddText: overflow\n");
		return;
	}
	if (cmd_debug->integer & 2)
	{
		if (len > 256)
			appPrintf(S_BLUE"AddText: %d chars\n", len);
		else
			appPrintf(S_BLUE"AddText: \"%s\"\n", text);
	}
	cmd_text.Write(text, len);
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
============
*/
static void Cbuf_InsertText(const char *text)
{
	int len = strlen(text);

	if (cmd_text.cursize + len >= cmd_text.maxsize)
	{
		appWPrintf("Cbuf_InsertText: overflow\n");
		return;
	}
	if (cmd_debug->integer & 2)
	{
		if (len > 256)
			appPrintf(S_BLUE"InsText: %d chars\n", len);
		else
			appPrintf(S_BLUE"InsText: \"%s\"\n", text);
	}
	// NOTE: here no overflow
	// SZ_Insert(&cmd_text, text, len, 0);
	if (cmd_text.cursize > 0) memmove(cmd_text.data + len, cmd_text.data, cmd_text.cursize);
	memcpy(cmd_text.data, text, len);
	cmd_text.cursize += len;
}


/*
============
Cbuf_CopyToDefer
============
*/
void Cbuf_CopyToDefer()
{
	memcpy(defer_text_buf, cmd_text_buf, cmd_text.cursize);
	defer_text_buf[cmd_text.cursize] = 0;
	cmd_text.cursize = 0;
	if (cmd_debug->integer & 2) appPrintf(S_BLUE"DeferCommands\n");
}

/*
============
Cbuf_InsertFromDefer
============
*/
void Cbuf_InsertFromDefer()
{
	if (cmd_debug->integer & 2) appPrintf(S_BLUE"UndeferCommands: ");	// line will be continued from Cbuf_InsertText()
	Cbuf_InsertText(defer_text_buf);
	defer_text_buf[0] = 0;
}


/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute()
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

		memcpy(line, text, len);
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
			memcpy(text, text+i, cmd_text.cursize);
		}

		// execute the command line
		if (!ExecuteCommand(line))
			// send it as a server command if we are connected
			Cmd_ForwardToServer(_argc, _argv);
	}
}


/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/


static void Cmd_Wait_f(bool usage, int argc, char **argv)
{
	if (argc > 2 || usage)
	{
		appPrintf("Usage: wait [<num_frames>]\n");
		return;
	}
	cmdWait = argc == 2 ? atoi(argv[1]) : 1;
}


static void Cmd_Exec_f(bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		appPrintf("Usage: exec <filename>\n");
		return;
	}

	char	*f;
	if (!(f = (char*) GFileSystem->LoadFile(argv[1])))
	{
		appWPrintf("Couldn't exec %s\n", argv[1]);
		return;
	}
	appPrintf("Execing %s\n", argv[1]);

	Cbuf_InsertText(f);

	delete f;
}


static void Cmd_Echo_f(int argc, char **argv)
{
	for (int i = 1; i < argc; i++)
		appPrintf("%s ", argv[i]);
	appPrintf("\n");
}


static void Cmd_Alias_f(bool usage, int argc, char **argv)
{
	char	cmd[1024];

	if (argc == 2 || usage)
	{
		appPrintf("Usage: alias [<name> <value>]\n");
		return;
	}

	if (argc == 1)
	{
		appPrintf("Current alias commands:\n");
		for (TListIterator<CAlias> a = AliasList; a; ++a)
			appPrintf("%s \"%s\"\n", a->name, a->value);
		return;
	}

	const char *name = argv[1];

	// if the alias already exists, reuse it
	CAlias *pos;
	CAlias *a = AliasList.Find(name, &pos);
	if (a)
		appFree(a->value);
	else
	{
		a = new (name) CAlias;
		AliasList.InsertAfter(a, pos);
	}

	// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (int i = 2; i < argc; i++)
	{
		strcat(cmd, argv[i]);
		if (i != (argc - 1))
			strcat(cmd, " ");
	}

	a->value = appStrdup(cmd);
}


void Cmd_Unalias_f(bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		appPrintf("Usage: unalias <mask>\n");
		return;
	}

	int n = 0;
	CAlias *alias, *next;
	for (alias = AliasList.First(); alias; alias = next)
	{
		next = AliasList.Next(alias);
		if (appMatchWildcard(alias->name, argv[1], true))
		{
			AliasList.Remove(alias);		// will scan list again
			delete alias->value;
			delete alias;
			n++;
		}
	}
	appPrintf("%d aliases removed\n", n);
}


void Cmd_WriteAliases(COutputDevice *Out)
{
	for (TListIterator<CAlias> alias = AliasList; alias; ++alias)
		Out->Printf("alias %s %s\n", alias->name, COM_QuoteString(alias->value, true));
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
static char *MacroExpandString(const char *text)
{
	static char	buf[MAX_STRING_CHARS];			// will contain result

	int len = strlen(text);
	if (text[len-1] == '\r')					// cut CR (not '\n'!) at end of line (DOS file encoding)
		len--;
	if (len >= sizeof(buf))
	{
		appWPrintf("Line exceeded "STR(MAX_STRING_CHARS)" chars, discarded\n");
		return NULL;
	}
	memcpy(buf, text, len);
	buf[len] = 0;

	int count = 0;								// to prevent infinite recurse
	int quotes = 0;

	for (char *s = buf; *s; s++)
	{
		const char *token;
		char	tmp[MAX_STRING_CHARS];
		int		varLen;

		if (*s == '"') quotes ^= 1;
		if (quotes) continue;					// don't expand inside quotes
		if (*s != '$') continue;

		// scan out the complete macro
		const char *data = s + 1;				// $varname -> varname
		token = COM_Parse(data);
		if (!data) continue;					// null token

		token = Cvar_VariableString(token);
		varLen = strlen(token);

		len = len - (data - s) + varLen;		// update length for overflow checking
		if (len >= sizeof(buf))
		{
			appWPrintf("Expanded line exceeded "STR(MAX_STRING_CHARS)" chars, discarded\n");
			return NULL;
		}

		strcpy(tmp, data);						// save tail
		memcpy(s, token, varLen);				// strcpy
		strcpy(s + varLen, tmp);				// append tail
		s--;									// continue from the same position

		if (++count == MACRO_LOOP_COUNT)
		{
			appWPrintf("Macro expansion loop, discarded\n");
			return NULL;
		}
	}

	if (quotes)
	{
		appWPrintf("Line has unmatched quote, discarded.\n");
		if (cmd_debug->integer)
			appPrintf("bad str: <%s> -> <%s>\n", text, buf);
		return NULL;
	}

	char *s1 = strchr(buf, '=');
	char *s2 = strchr(buf, '\"');
	if (s1 && (!s2 || s2 > s1) &&						// a=b, but '=' not inside quotes
		(s1 > buf && s1[-1] != ' ' && s1[1] != ' '))	// ingnore "a = b" (may be "bind = scr_sizeup" etc; NOTE: keyname already changed !!)
	{
		*s1 = 0;			// cut varName
		s1++;				// skip '='
		if (s2 == s1)		// pos(") is pos(=)+1
		{
			s2 = s1 + strlen(s1) - 1;
			if (*s2 == '\"')
			{
				s1++;		// skip '"'
				*s2 = 0;	// cut trailng '"'
			}
		}
//		appWPrintf("[%s] <- [%s]\n", buf, s1);
		Cvar_Set(buf, s1);
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
static void TokenizeString(const char *text)
{
	int i;
	// clear the args from the last string
	for (i = 0; i < _argc; i++)
		appFree(_argv[i]);
	_argc = 0;

	if (!text) return;

	// split command line
	while (true)
	{
		// skip spaces and tabs (char in [1..32]); stop on non-space, null or '\n'
		while (*text && *text <= ' ' && *text != '\n') text++;

		if (!text[0] || text[0] == '\n')
			break;

		const char *com_token = COM_Parse(text);
		if (!text) break;

		if (_argc < ARRAY_COUNT(_argv))
			_argv[_argc++] = appStrdup(com_token);
	}

	// set rest of argv[] array to ""
	for (i = _argc; i < ARRAY_COUNT(_argv); i++)
		_argv[i] = "";
}


/*-----------------------------------------------------------------------------
	Command management
-----------------------------------------------------------------------------*/

static void Cmd_Init();

bool RegisterCommand(const char *name, void(*func)(), unsigned flags)
{
	guard(RegisterCommand);
	EXEC_ONCE(Cmd_Init())
	CCommand *pos;
	if (CmdList.Find(name, &pos))
	{
		appWPrintf("RegisterCommand: \"%s\" is already registered\n", name);
		return false;
	}
	CCommand *cmd = new (name) CCommand;
	cmd->flags = flags;
	cmd->func = func;
	CmdList.InsertAfter(cmd, pos);

	return true;
	unguard;
}


bool UnregisterCommand(const char *name)
{
	guard(UnregisterCommand);
	CCommand *cmd = CmdList.Find(name);
	if (!cmd)
	{
		appWPrintf("UnregisterCommand: \"%s\" is not registered\n", name);
		return false;
	}
	CmdList.Remove(cmd);
	delete cmd;

	return true;
	unguard;
}


/*
============
ExecuteCommand

$Cvars will be expanded unless they are in a quoted token
============
*/
//#define RECURCE_CHECK		1

#if RECURCE_CHECK
static int recurseCheck = 0;
#endif

bool ExecuteCommand(const char *text)
{
	guard(ExecuteCommand);

	const char *str = MacroExpandString(text);	// may be used for COMMAND_ARGS2
	TokenizeString(str);
#if RECURCE_CHECK
	int check = ++recurseCheck;
#endif

	// execute the command line
	if (!_argc) return true;		// no tokens

	if (cmd_debug->integer & 1)
		appPrintf(S_CYAN"cmd: %s\n", text);

	// check functions
	CCommand *cmd = CmdList.Find(_argv[0]);
	if (cmd)
	{
		bool usage = _argc == 2 && !strcmp(_argv[1], "/?");
		guard(cmd);
		switch (cmd->flags)
		{
		case 0:
			cmd->func();
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
		case COMMAND_ARGS2:
			((void (*) (const char*)) cmd->func) (str);
			break;
		case COMMAND_USAGE|COMMAND_ARGS2:
			((void (*) (bool, const char*)) cmd->func) (usage, str);
			break;
		}
#if RECURCE_CHECK
		if (check != recurseCheck)
			appWPrintf("ExecuteCommand: recursion in \"%s\"\n", text);
#endif
		unguardf(("%s", cmd->name));
		return true;
	}

	// check alias
	CAlias *a = AliasList.Find(_argv[0]);
	if (a)
	{
		if (++aliasCount == ALIAS_LOOP_COUNT)
		{
			appWPrintf("ALIAS_LOOP_COUNT\n");
			return true;
		}
		Cbuf_InsertText(va("%s\n", a->value));
		return true;
	}

	// check cvars
	if (Cvar_Command(_argc, _argv))
	{
#if RECURCE_CHECK
		if (check != recurseCheck)
			appWPrintf("ExecuteCommand: recursion in \"%s\"\n", text);
#endif
		return true;
	}

	unguard;
	return false;
}


bool ExecuteCommand(const char *str, const CSimpleCommand *CmdList, int numCommands)
{
	guard(ExecuteSimpleCommand);
//	GetArgs(str, false);
	TokenizeString(str);
#if RECURCE_CHECK
	int check = ++recurseCheck;
#endif
	for (int i = 0; i < numCommands; i++, CmdList++)
		if (!stricmp(CmdList->name, _argv[0]))
		{
			if (!CmdList->func) return true;		// NULL function
			guard(cmd)
			CmdList->func(_argc, _argv);
#if RECURCE_CHECK
			if (check != recurseCheck)
				appWPrintf("ExecuteCommand: recursion in \"%s\"\n", str);
#endif
			return true;
			unguardf(("%s", CmdList->name))
		}
	return false;
	unguard;
}


static void Cmd_List_f(bool usage, int argc, char **argv)
{
	if (argc > 2 || usage)
	{
		appPrintf("Usage: cmdlist [<mask>]\n");
		return;
	}

	const char *mask = (argc == 2) ? argv[1] : NULL;
	int n = 0, total = 0;
	appPrintf("----i-a-name----------------\n");
	for (TListIterator<CCommand> cmd = CmdList; cmd; ++cmd)
	{
		total++;
		if (mask && !appMatchWildcard(cmd->name, mask, true)) continue;
		appPrintf("%-3d %c %c %s\n", total,
			cmd->flags & COMMAND_USAGE ? 'I' : ' ',
			cmd->flags & (COMMAND_ARGS|COMMAND_ARGS2) ? 'A' : ' ',
			cmd->name);
		n++;
	}
	appPrintf("Displayed %d/%d commands\n", n, total);
}


static void Cmd_Init()
{
	// cbuf
	cmd_text.Init(ARRAY_ARG(cmd_text_buf));
	// commands
	RegisterCommand("cmdlist", Cmd_List_f);
	RegisterCommand("exec", Cmd_Exec_f);
	RegisterCommand("echo", Cmd_Echo_f);
	RegisterCommand("alias", Cmd_Alias_f);
	RegisterCommand("unalias", Cmd_Unalias_f);
	RegisterCommand("wait", Cmd_Wait_f);
	// cvars
	cmd_debug = Cvar_Get("cmd_debug", "0");
}
