#include "qcommon.h"

//?? NOTE: copied from cmd.cpp
//?? should rework

static char	cmd_args[MAX_STRING_CHARS];
static char	*_argv[MAX_STRING_TOKENS];
static int	_argc;

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
	int		i;
	char	*com_token;

	// clear the args from the last string
	for (i = 0; i < _argc; i++)
		appFree (_argv[i]);
	_argc = 0;
	cmd_args[0] = 0;

	if (!text) return;

	while (1)
	{
		// skip spaces and tabs (char in [1..32]); stop on non-space, null or '\n'
		while (*text && *text <= ' ' && *text != '\n') text++;

		if (!text[0] || text[0] == '\n')
			return;

		// set cmd_args to everything after the first arg
		if (_argc == 1)
		{
			strcpy (cmd_args, text);
			// cut trailing spaces
			int len = strlen (cmd_args) - 1;
			while (len >= 0 && cmd_args[len] <= ' ')
				cmd_args[len--] = 0;
		}

		com_token = COM_Parse (text);
		if (!text) return;

		if (_argc < MAX_STRING_TOKENS)
			_argv[_argc++] = CopyString (com_token);
	}
}

int SV_Argc (void)
{
	return _argc;
}

// NOTE: some commands can ignore Argc() and use Argv(argc+n) -- should return "" in this case (for compatibility)
char *SV_Argv (int arg)
{
	if (arg >= _argc)
		return "";
	return _argv[arg];
}

char *SV_Args (void)
{
	return cmd_args;
}
