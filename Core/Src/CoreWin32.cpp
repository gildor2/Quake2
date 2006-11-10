#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <mmsystem.h>				// for timeGetTime() stuff
#pragma comment (lib, "winmm.lib")	// ...

#include "CorePrivate.h"


char GMachineOS[64] = "Unknown Windows variant";
bool GIsWinNT, GIsWin2K;

HINSTANCE hInstance;


/*-----------------------------------------------------------------------------
	Timing
-----------------------------------------------------------------------------*/

unsigned appMilliseconds()
{
	// use exact value from timeGetTime() to be in sync with other win32
	// time values, comes with win32 messages
	return timeGetTime();
}


void appSleep(unsigned msec)
{
	Sleep(msec);
}


/*-----------------------------------------------------------------------------
	Detecting Windows version
-----------------------------------------------------------------------------*/

static void DetectOs()
{
	struct osInfo_t {
		byte	vMaj, vMin, vPlatf;
		const char *name;
	};
	static const osInfo_t table[] = {
#define W9X		VER_PLATFORM_WIN32_WINDOWS
#define WNT		VER_PLATFORM_WIN32_NT
		// Win9X
		{	4,0,	W9X,	"95"	},
		{	4,10,	W9X,	"98"	},
		{	4,90,	W9X,	"Me"	},
		// WinNT
		{	3,51,	WNT,	"NT 3.51"},
		{	4,0,	WNT,	"NT 4.0"},
		{	5,0,	WNT,	"2000"	},
		{	5,1,	WNT,	"XP"	},
		{	5,2,	WNT,	"2003"	},
		{	6,0,	WNT,	"Vista"	}
	};
	OSVERSIONINFO ver;
	ver.dwOSVersionInfoSize = sizeof(ver);
	if (!GetVersionEx(&ver))
	{
		int err = GetLastError();
		appPrintf("Windows version detection failed:\nError: %d  %s\n", err, appGetSystemErrorMessage(err));
		return;
	}
	GIsWinNT = (ver.dwPlatformId == VER_PLATFORM_WIN32_NT);
	GIsWin2K = (GIsWinNT && ver.dwMajorVersion >= 5);
	if (!GIsWinNT)
		ver.dwBuildNumber &= 0xFFFF;	// win9x: LOWORD contains build number, HIGHWORD contains OS version ...
	for (int i = 0; i < ARRAY_COUNT(table); i++)
	{
		const osInfo_t &v = table[i];
		if (v.vPlatf == ver.dwPlatformId &&
			v.vMaj == ver.dwMajorVersion &&
			v.vMin == ver.dwMinorVersion)
		{
			appSprintf(ARRAY_ARG(GMachineOS), "Windows %s (Build: %d)", v.name, ver.dwBuildNumber);
			appPrintf("Detected OS: %s\n", GMachineOS);
			return;
		}
	}
	const char *platf = "Unknown";
	if (ver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		platf = "9X";
	else if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT)
		platf = "NT";
	appSprintf(ARRAY_ARG(GMachineOS), "Windows %s %d.%d (Build: %d)", platf, ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber);
	appPrintf("Detected OS: %s\n", GMachineOS);
}


/*-----------------------------------------------------------------------------
	Miscellaneous
-----------------------------------------------------------------------------*/

const char *appPackage()
{
	static TString<256> Filename;
	if (Filename[0]) return Filename;	// already computed
	// get executable filename
	GetModuleFileName(NULL, ARRAY_ARG(Filename));
	char *s = Filename.rchr('\\');
	if (s) Filename = s+1;
	s = Filename.rchr('.');
	if (s) *s = 0;
	return Filename;
}


static void SetDefaultDirectory()
{
	TString<256> Filename;
	GetModuleFileName(NULL, ARRAY_ARG(Filename));
	char *s = Filename.rchr('\\');
	if (s) *s = 0;
	SetCurrentDirectory(Filename);
}


const char *appGetSystemErrorMessage(unsigned code)
{
	char buffer[1024];
	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code,
		MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT),
		ARRAY_ARG(buffer), NULL))
	{
		// cut CR/LF
		char *s = strchr(buffer, '\r');
		if (s) s[0] = 0;
		return va("%s", buffer);
	}
	return "unknown error code";
}


void appDisplayError()
{
	// avoid 2 va() calls (may overflow one buffer when filling another)
	TString<256> Title;
	Title.sprintf("%s: fatal error", appPackage());
	// display error
	MessageBox(NULL,
#if DO_GUARD
		va("%s\n\nHistory: %s", *GErr.Message, *GErr.History),
#else
		GErr.Message,
#endif
		Title, MB_OK|MB_ICONSTOP/*|MB_TOPMOST*/|MB_SETFOREGROUND);
}


/*-----------------------------------------------------------------------------
	Initialization/finalization
-----------------------------------------------------------------------------*/

#if __MINGW32__
long WINAPI win32ExceptFilter(struct _EXCEPTION_POINTERS *info);

long WINAPI mingw32ExceptFilter(struct _EXCEPTION_POINTERS *info)
{
	win32ExceptFilter(info);
	THROW;							// OS exception -> C++ exception
}
#endif

void appInitPlatform()
{
#if __MINGW32__
	SetUnhandledExceptionFilter(mingw32ExceptFilter);
#endif
	hInstance = GetModuleHandle(NULL);
	// NOTE: under WinXP (Win2k too?) without timeBeginPeriod(1) Sleep() (and some another time-related
	//	functions too) will work in bad resolution; so, we should use timeBeginPeriod(1) even if don't
	//	use timeGetTime()
	timeBeginPeriod(1);
	// gather system information
	appDetectCPU();				// timeBeginPeriod(1) should be called before this function!
	DetectOs();
	// setup current directory
	SetDefaultDirectory();
}


void appShutdownPlatform()
{
	timeEndPeriod(1);
}
