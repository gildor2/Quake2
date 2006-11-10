#include "qcommon.h"

// some dummy (stub) functions for dedicated server (DEDICATED_ONLY)
// NOTE: dedicated server can be compiled from complete Q2 executable source

// cl_scrn.cpp
void SCR_SetLevelshot(const char *name)
{}

void SCR_DebugGraph(float value, int color)
{}

// cl_main.cpp
void Cmd_ForwardToServer(int argc, char **argv)
{
	// all unknown commands will go here
	static const char *cmds[] = {"bind", "unbindall", "disconnect"};
	const char *cmd = argv[0];
	for (int i = 0; i < ARRAY_COUNT(cmds); i++)
		if (!stricmp(cmd, cmds[i]))
		{
			Com_DPrintf("Skipped dummy command \"%s\"\n", cmd);
			return;
		}
	appWPrintf("Unknown command \"%s\"\n", cmd);
}

void CL_WriteConfiguration(const char *filename)
{}

void CL_Shutdown()
{}

void Key_Init()
{}

//??? separate CompleteCommand() from keys.cpp
void CompleteCommand()
{}

//?? separate editLine[] from console.cpp
char editLine[1024];
