#include "qcommon.h"

//?? NOTE: copied from cmd.cpp
//?? should rework

static char	cmd_args[MAX_STRING_CHARS];
static char	*cmd_argv[MAX_STRING_TOKENS];
static int	cmd_argc;

/*
server::game:: uses Cmd_Argc()/Argv()/Args():
- sv_user::SV_ExecuteUserCommand():
  ge->ClientCommand()
- sv_cmds::SV_ServerCommand_f():
  ge->ServerCommand()
Implement SplitCommandLine() for use in
  ExecuteCommand() and ParseCommandLine()
  (class CCmdLineParser() with constructor)
*/

void SV_TokenizeString (const char *text)
{
	// clear the args from the last string
	for (int i = 0; i < cmd_argc; i++)
		appFree (cmd_argv[i]);
	cmd_argc = 0;
	cmd_args[0] = 0;

	if (!text) return;

	while (true)
	{
		// skip spaces and tabs (char in [1..32]); stop on non-space, null or '\n'
		while (*text && *text <= ' ' && *text != '\n') text++;

		if (!text[0] || text[0] == '\n')
			return;

		// set cmd_args to everything after the first arg
		if (cmd_argc == 1)
		{
			strcpy (cmd_args, text);
			// cut trailing spaces
			int len = strlen (cmd_args) - 1;
			while (len >= 0 && cmd_args[len] <= ' ')
				cmd_args[len--] = 0;
		}

		const char *com_token = COM_Parse (text);
		if (!text) return;

		if (cmd_argc < MAX_STRING_TOKENS)
			cmd_argv[cmd_argc++] = CopyString (com_token);
	}
}

int SV_Argc ()
{
	return cmd_argc;
}

// NOTE: some commands can ignore Argc() and use Argv(argc+n) -- should return "" in this case (for compatibility)
char *SV_Argv (int arg)
{
	if (arg >= cmd_argc)
		return "";
	return cmd_argv[arg];
}

char *SV_Args ()
{
	return cmd_args;
}
