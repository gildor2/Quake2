#include <time.h>						// for logging time funcs
#include "CorePrivate.h"


int GScreenWidth = 80;					//?? where to place


#if 0	//!!!
TString<512>	GVersion;

static void Cmd_Version ()
{
	appPrintf ("%s\n", *GVersion);
}
#endif


const char *appTimestamp ()
{
	static char ctime[64];
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

	// verify fundamental sizes
	staticAssert(sizeof(short)==2, Sizeof_Short_2);
	staticAssert(sizeof(int)==4, Sizeof_Int_4);
	staticAssert((char)127 > (char)128, Char_Should_Be_Signed);

	appInitPlatform ();
	appInitMemory ();
	appInitError ();
	appLoadDebugSymbols ();

//!!	GVersion = PkgVersion;
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
		// add CR/LF to error log and close it
		COutputDevice *ErrLog = appGetErrorLog ();
		ErrLog->Write ("\n\n");
		ErrLog->Close ();
	}
	appShutdownPlatform ();
}
