#include <time.h>						// for logging time funcs
#include "CorePrivate.h"


int GScreenWidth = 80;					//?? where to place


#if 0	//!!!
TString<512>	GVersion;

static void Cmd_Version()
{
	appPrintf("%s\n", *GVersion);
}
#endif


const char *appTimestamp()
{
	static char ctime[64];
	time_t itime;
	time(&itime);
	strftime(ARRAY_ARG(ctime), "%b %d %Y %H:%M:%S", localtime(&itime));
	return ctime;
}


bool GIsRequestingExit = false;

static void Cmd_Quit()
{
	GIsRequestingExit = true;
}


/*-----------------------------------------------------------------------------
	Little-endian support for big-endian platforms
-----------------------------------------------------------------------------*/

#if !LITTLE_ENDIAN

short LittleShort(short l)
{
	union {
		short	i;
		byte	b[2];
	} d;
	d.i = l;
	Exchange(b[0], b[1]);
	return d.i;
}

int LittieLong(int l)
{
	union {
		int		i;
		byte	b[4];
	} d;
	d.i = l;
	Exchange(b[0], b[3]);
	Exchange(b[1], b[2]);
	return d.i;
}

float LittleFloat(float f)
{
	union {
		float	f;
		byte	b[4];
	} d;
	d.f = f;
	Exchange(b[0], b[3]);
	Exchange(b[1], b[2]);
	return d.f;
}

#endif


void appInit(/*?? const char *_cmdLine */)
{
	guard(appInit);

	// verify fundamental sizes
	staticAssert(sizeof(short)==2, Sizeof_Short_2);
	staticAssert(sizeof(int)==4, Sizeof_Int_4);
	staticAssert((char)127 > (char)128, Char_Should_Be_Signed);

	appInitPlatform();
	appInitMemory();
	appInitError();
	appLoadDebugSymbols();

//!!	GVersion = PkgVersion;
//!!	RegisterCommand("version", Cmd_Version);
	RegisterCommand("quit", Cmd_Quit);		//?? add "exit" as synonym ("quit" came from Quake, but "exit" from bash etc)

	unguard;
}


void appExit()
{
	appShutdownError();
	appShutdownPlatform();
}
