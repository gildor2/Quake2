#include "Core.h"
#include "CoreLocal.h"


#if 0	//!!!
char	GVersion[512];

static void Cmd_Version (void)
{
	appPrintf ("%s\n", GVersion);
}
#endif


void appInit (/*?? const char *_cmdLine, COutputDevice *_log */)
{
	guard(appInit);

	//!!
//??	GLog = _log;
	appInitPlatform ();
	appInitMemory ();
//!!	appInitCommands ();
	appInitError ();
	appLoadDebugSymbols ();

//!!	appStrcpy (GVersion, PkgVersion);
//!!	RegisterCommand ("version", Cmd_Version);

	unguard;
}


void appExit (void)
{
	appShutdownPlatform ();
}
