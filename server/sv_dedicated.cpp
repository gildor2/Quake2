#include "qcommon.h"

// some dummy (stub) functions for (small) dedicated server
// NOTE: dedicated server can be compiled from complete Q2 executable source

// cl_scrn.cpp
void SCR_SetLevelshot (const char *name)
{
}

void SCR_DebugGraph (float value, int color)
{
}

// cl_main.cpp
void Cmd_ForwardToServer (int argc, char **argv)
{
	// all unknown commands will go here
	static const char *cmds[] = {"bind", "unbindall", "disconnect"};
	const char *cmd = argv[0];
	for (int i = 0; i < ARRAY_COUNT(cmds); i++)
		if (!stricmp (cmd, cmds[i]))
		{
			Com_DPrintf ("Skipped dummy command \"%s\"\n", cmd);
			return;
		}
	Com_WPrintf ("Unknown command \"%s\"\n", cmd);
}

void CL_WriteConfiguration (char *filename)
{
}

void CL_Shutdown (bool error)
{
}

void Key_Init (void)
{
}

//??? separate CompleteCommand() from keys.cpp
void CompleteCommand (void)
{
}

//?? separate editLine[] from console.cpp
char editLine[1024];
