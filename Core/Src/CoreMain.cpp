#include "Core.h"
#include "CoreLocal.h"
#include <time.h>						// for logging time funcs


#if 0	//!!!
char	GVersion[512];

static void Cmd_Version ()
{
	appPrintf ("%s\n", GVersion);
}
#endif


const char *appTimestamp ()
{
	static char ctime[256];
	time_t itime;
	time (&itime);
	strftime (ARRAY_ARG(ctime), "%b %d %Y %H:%M:%S", localtime (&itime));
	return ctime;
}


bool GIsRequestingExit = false;

static void Cmd_Quit ()
{
	GIsRequestingExit = true;
}


void appInit (/*?? const char *_cmdLine */)
{
	guard(appInit);

	appInitPlatform ();
	appInitMemory ();
	appInitError ();
	appLoadDebugSymbols ();

//!!	appStrcpy (GVersion, PkgVersion);
//!!	RegisterCommand ("version", Cmd_Version);
	RegisterCommand ("quit", Cmd_Quit);

	unguard;
}


void appExit ()
{
	if (GIsFatalError)
	{
		// to log
		appPrintf ("\n\n"S_RED"--------------------\n%s fatal error: %s\n", appPackage (), *GErr.Message);
#if DO_GUARD
		appPrintf ("History: %s\n\n", *GErr.History);
#endif
		// popup
		appDisplayError ();
	}
	appShutdownPlatform ();
}
