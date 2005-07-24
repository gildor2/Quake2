#include "Core.h"

//!! TODO: exec, alias, unalias


#define MAX_CMDLINE		1024
#define MAX_ARGS		256


class CCommand : public CStringItem
{
public:
	int		flags;
	void (*func) ();
};


class CAlias : public CStringItem
{
public:
	char	*macro;
	bool	active;			// true when currently executed (used to prevent recursive aliases)
};


static TList<CCommand> CmdList;
static TList<CAlias> AliasList;


/*-----------------------------------------------------------------------------
	Command management
-----------------------------------------------------------------------------*/


bool RegisterCommand (const char *name, void(*func)(), int flags)
{
	guard(RegisterCommand);
	CCommand *pos;
	if (CmdList.Find (name, &pos))
	{
		appWPrintf ("RegisterCommand: \"%s\" is already registered\n", name);
		return false;
	}
	CCommand *cmd = new (name) CCommand;
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


/*-----------------------------------------------------------------------------
	Alias management
-----------------------------------------------------------------------------*/


//!!


/*-----------------------------------------------------------------------------
	Command execution
-----------------------------------------------------------------------------*/


static char lineBuffer[MAX_CMDLINE];
static char *_argv[MAX_ARGS];
static int  _argc;


static void GetArgs (const char *str, bool expandVars)
{
	char	*d, c;

	guard(GetArgs);
	// preparing
	_argc = 0;
	d = lineBuffer;
	for (int i = 0; i < MAX_ARGS; i++) _argv[i] = "";
	// skip leading spaces
	while (*str == ' ') str++;
	// parsing line
	while (c = *str++)
	{
		if (_argc == MAX_ARGS)
		{
			appWPrintf ("GetArgs: MAX_ARGS hit\n");
			return;
		}
		_argv[_argc++] = d;
		//?? need to check lineBuffer overflow
		//?? need to treat '\t' as ' '
		if (c == '\"')
		{
			// quoted argument
			while (c = *str++)
			{
				if (c == '\"')
				{
					if (*str != '\"') break;		// allow quote doubling inside strings
					str++;
				}
//				if (c == '$' && expandVars) expandVars	//?? expand vars "${var_name}" (bash-style)
				*d++ = c;
			}
			if (!c)
			{
				//!! Warn: unmatched quote
				*d = 0;
				return;
			}
		}
//		else if (c == '$' && expandVars)
//		{
			//!! expand cvar value: name till space or 0
//		}
		else
		{
			// unquoted argument
			*d++ = c;
			while (c = *str)
			{
				str++;			// NOTE: when c==0, we will not get here
				if (c == ' ') break;
				*d++ = c;
				// NOTE: if '"' will appear without space (i.e. abc"def), this will not split arg
			}
		}
		*d++ = 0;
		// skip spaces
		while (*str == ' ') str++;
	}
	*d = 0;
	unguard;
}


// Generic command execution
void ExecuteCommand (const char *str)
{
	CAlias	*alias;
	CCommand *cmd;

	guard(ExecuteCommand);
	GetArgs (str, true);
	if (!_argc) return;				// empty string

	alias = AliasList.Find (_argv[0]);
	if (alias && !alias->active)
	{
		alias->active = true;
		//!! exec (requires (global?) command buffer(s) with insertion ability; + may be, source from file (#include))

		return;
	}

	cmd = CmdList.Find (_argv[0]);
	if (cmd && cmd->func)
	{
		bool usage = _argc == 2 && !appStrcmp (_argv[1], "/?");
		if (!(cmd->flags & COMMAND_USAGE) && usage)
		{
			appPrintf ("No usage info for command \"%s\"\n", cmd->name);
			return;
		}
		// execute function
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
		return;
		unguardf(("%s", cmd->name));
	}
	//!! should pass unhandled commands to registered function (when server started, will send to server)
	unguard;
}


bool ExecuteCommand (const char *str, const CSimpleCommand *CmdList, int numCommands)
{
	guard(ExecuteSimpleCommand);
	GetArgs (str, false);
	for (int i = 0; i < numCommands; i++, CmdList++)
		if (!appStricmp (CmdList->name, _argv[0]))
		{
			if (!CmdList->func) return true;		// NULL function
			guard(cmd)
			CmdList->func (_argc, _argv);
			return true;
			unguardf(("%s", CmdList->name))
		}
	return false;
	unguard;
}


/*-----------------------------------------------------------------------------
	Console commands
-----------------------------------------------------------------------------*/


static void Cmd_Echo (int argc, char **argv)
{
	for (int i = 1; i < argc; i++) appPrintf (i > 1 ? " %s" : "%s", argv[i]);
	appPrintf ("\n");
}


static void Cmd_CmdList (bool usage, int argc, char **argv)
{
	int		n, total;

	if (argc > 2 || usage)
	{
		Com_Printf ("Usage: cmdlist [<mask>]\n");
		return;
	}

	const char *mask = (argc == 2) ? argv[1] : NULL;
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


/*-----------------------------------------------------------------------------
	Initialization
-----------------------------------------------------------------------------*/

void appInitCommands ()
{
	RegisterCommand ("echo", Cmd_Echo);
	RegisterCommand ("cmdlist", Cmd_CmdList);
}
