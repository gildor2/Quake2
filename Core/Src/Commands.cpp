#include "CorePrivate.h"

//!! TODO: exec (outside?!), alias, unalias


#define MAX_CMDLINE		1024
#define MAX_ARGS		256


class CCommand : public CStringItem
{
public:
	unsigned flags;
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

static void InitCommands();

bool RegisterCommand(const char *name, void(*func)(), unsigned flags)
{
	guard(RegisterCommand);
	EXEC_ONCE(InitCommands())

	CCommand *pos;
	if (CmdList.Find(name, &pos))
	{
		appWPrintf("RegisterCommand: \"%s\" is already registered\n", name);
		return false;
	}
	CCommand *cmd = new (name) CCommand;
	cmd->flags = flags;
	cmd->func  = func;
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


/*-----------------------------------------------------------------------------
	Alias management
-----------------------------------------------------------------------------*/


//!!


/*-----------------------------------------------------------------------------
	Command execution
-----------------------------------------------------------------------------*/


static char lineBuffer[MAX_CMDLINE];
static char *c_argv[MAX_ARGS];
static int  c_argc;


static void GetArgs(const char *str, bool expandVars)
{
	guard(GetArgs);
	// preparing
	c_argc = 0;
	char *d = lineBuffer;
	for (int i = 0; i < MAX_ARGS; i++) c_argv[i] = "";
	// skip leading spaces
	while (*str == ' ') str++;
	// parsing line
	while (char c = *str++)
	{
		if (c_argc == MAX_ARGS)
		{
			appWPrintf("GetArgs: MAX_ARGS hit\n");
			return;
		}
		c_argv[c_argc++] = d;
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
bool ExecuteCommand(const char *str)
{
	guard(ExecuteCommand);
	GetArgs(str, true);
	if (!c_argc) return true;			// empty string

	CAlias *alias = AliasList.Find(c_argv[0]);
	if (alias && !alias->active)
	{
		alias->active = true;
		//!! exec (requires (global?) command buffer(s) with insertion ability; + may be, source from file (#include))

		return true;	//?? recurse
	}

	CCommand *cmd = CmdList.Find(c_argv[0]);
	if (cmd && cmd->func)
	{
		bool usage = c_argc == 2 && !appStrcmp(c_argv[1], "/?");
		if (!(cmd->flags & COMMAND_USAGE) && usage)
		{
			appPrintf("No usage info for command \"%s\"\n", cmd->name);
			return true;
		}
		// execute function
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
			((void (*) (int, char**)) cmd->func) (c_argc, c_argv);
			break;
		case COMMAND_USAGE|COMMAND_ARGS:
			((void (*) (bool, int, char**)) cmd->func) (usage, c_argc, c_argv);
			break;
		case COMMAND_ARGS2:
			((void (*) (const char*)) cmd->func) (str);
			break;
		case COMMAND_USAGE|COMMAND_ARGS2:
			((void (*) (bool, const char*)) cmd->func) (usage, str);
			break;
		}
		return true;
		unguardf(("%s", cmd->name));
	}
	//!! should pass unhandled commands to registered function (when server started, will send to server)
	return false;
	unguard;
}


bool ExecuteCommand(const char *str, const CSimpleCommand *CmdList, int numCommands)
{
	guard(ExecuteSimpleCommand);
	GetArgs(str, false);
	for (int i = 0; i < numCommands; i++, CmdList++)
		if (!appStricmp(CmdList->name, c_argv[0]))
		{
			if (!CmdList->func) return true;		// NULL function
			guard(cmd)
			CmdList->func(c_argc, c_argv);
			return true;
			unguardf(("%s", CmdList->name))
		}
	return false;
	unguard;
}


/*-----------------------------------------------------------------------------
	Console commands
-----------------------------------------------------------------------------*/


static void Cmd_Echo(int argc, char **argv)
{
	for (int i = 1; i < argc; i++) appPrintf(i > 1 ? " %s" : "%s", argv[i]);
	appPrintf("\n");
}


static void Cmd_CmdList(bool usage, int argc, char **argv)
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


/*-----------------------------------------------------------------------------
	Initialization
-----------------------------------------------------------------------------*/

static void InitCommands()
{
	RegisterCommand("echo", Cmd_Echo);
	RegisterCommand("cmdlist", Cmd_CmdList);
}
