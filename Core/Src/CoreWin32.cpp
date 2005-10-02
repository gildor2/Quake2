#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <mmsystem.h>				// for timeGetTime() stuff
#pragma comment (lib, "winmm.lib")	// ...

#include "CorePrivate.h"


double GSecondsPerCycle;			// really, should use GCyclesPerSecond, but this will require divide operation every
									// time we use this, so GSecondsPerCycle=1/GCyclesPerSecond
double GMSecondsPerCycle;			// == 1000*GSecondsPerCycle
static unsigned timeBase;

static bool IsMMX, IsSSE, IsRDTSC, Is3DNow;

char GMachineOS[64]  = "Unknown Windows variant";
char GMachineCPU[64] = "Unknown 386/486 CPU";
bool GIsWinNT, GIsWin2K;

HINSTANCE hInstance;


/*-----------------------------------------------------------------------------
	Timing
-----------------------------------------------------------------------------*/

// We should subtract timeBase from acquired timeGetTime() value to determine time since
// application startup, this is useful only for improving precision of retreived float
// milliseconds value (large integer parts will reduce precision of float part); for
// the same reason, we use "double" instead of "float" as a result value.

double appSeconds ()
{
	if (IsRDTSC)
	{
		__int64 cycles = appCycles64 ();
		return cycles * GSecondsPerCycle;
	}
	else
		return (timeGetTime () - timeBase) / 1000.0;
}


double appMillisecondsf ()
{
	if (IsRDTSC)
	{
		__int64 cycles = appCycles64 ();
		return cycles * GMSecondsPerCycle;
	}
	else
		return timeGetTime () - timeBase;
}


unsigned appMilliseconds ()
{
	if (IsRDTSC)
		return appFloor (appMillisecondsf ());
	else
		return timeGetTime () - timeBase;
}


/*-----------------------------------------------------------------------------
	CPU identification
-----------------------------------------------------------------------------*/

static unsigned cpuidRegs[4];

void cpuid (unsigned code)
{
	__asm {
		mov		eax,code
		cpuid
		mov		cpuidRegs,eax
		mov		cpuidRegs+4,ebx
		mov		cpuidRegs+8,ecx
		mov		cpuidRegs+12,edx
	}
}

#if 1

#pragma warning(push)
#pragma warning(disable : 4035)		// "no return value"
inline unsigned cpuid0 (unsigned code)
{
	__asm {
		mov		eax,code
		cpuid
	}
}

inline unsigned cpuid3 (unsigned code)
{
	__asm {
		mov		eax,code
		cpuid
		mov		eax,edx
	}
}
#pragma warning(pop)

#else

inline unsigned cpuid0 (unsigned code)
{
	cpuid (code);
	return cpuidRegs[0];
}
inline unsigned cpuid3 (unsigned code)
{
	cpuid (code);
	return cpuidRegs[3];
}
#endif

static void CheckCpuModel ()
{
#define MAX_CPU_NAME_PARTS	12	// 3 times of 4 regs
	union {
		unsigned reg[MAX_CPU_NAME_PARTS];
		char	name[MAX_CPU_NAME_PARTS*4+1];
	} t;

	// cpuid presence
	unsigned cpu0;
	__try
	{
		cpu0 = cpuid0 (0x80000000);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// no CPUID available
		appPrintf ("CPU: %s\n", GMachineCPU);
		return;
	}

	// CPU name
	if (cpu0 >= 0x80000004)		// extended vendor string available
	{
		int i = 0;
		for (unsigned fn = 0x80000002; fn <= 0x80000004; fn++)
		{
			cpuid (fn);
			memcpy (t.reg+i, cpuidRegs, sizeof(cpuidRegs));
			i += 4;
		}
		t.name[i*4] = 0;
		const char *s = t.name;
		while (*s == ' ') s++;	// skip leading spaces
		appStrncpyz (GMachineCPU, s, sizeof(GMachineCPU));
	}
	else
	{
		cpuid (0);
		t.reg[0] = cpuidRegs[1];
		t.reg[1] = cpuidRegs[3];
		t.reg[2] = cpuidRegs[2];
		t.name[12] = 0;			// null-terminated
		appStrncpyz (GMachineCPU, t.name, sizeof(GMachineCPU));
	}
	appPrintf ("CPU name: %s\n", GMachineCPU);

	// caps
	appPrintf ("CPU caps: [ ");

	unsigned tmp = cpuid3 (1);
	if (tmp & 0x00000001)	appPrintf ("FPU ");
	if (tmp & 0x00000010)
	{
		appPrintf ("RDTSC ");
		IsRDTSC = true;
	}
	//!! NOTE: if planning to use MMX/SSE/SSE2/3DNow! - should check OS support for this tech
	if (tmp & 0x00800000)
	{
		appPrintf ("MMX ");
		IsMMX = true;
	}
	if (tmp & 0x02000000)
	{
		appPrintf ("SSE ");
		IsSSE = true;
	}
	if (tmp & 0x04000000)	appPrintf ("SSE2 ");

	// check extended features
	if (cpu0 >= 0x80000001)		// largest recognized extended function
	{
		tmp = cpuid3 (0x80000001);
		if (tmp & 0x80000000)
		{
			appPrintf ("3DNow! ");
			Is3DNow = true;
		}
	}

	appPrintf ("]\n");
}


static void CheckCpuSpeed ()
{
	if (!IsRDTSC) return;

	timeBeginPeriod (1);

	GMSecondsPerCycle = 1.0;	// initial value (1 Hz)
	for (int tries = 0; tries < 3; tries++)
	{
		__int64 stamp1 = appCycles64 ();
		unsigned time1 = timeGetTime ();
		unsigned time2;
		while (true)
		{
			time2 = timeGetTime ();
			if (time2 - time1 > 150) break;
		}
		__int64 stamp2 = appCycles64 ();
		double msecPerCycle = (time2 - time1) / (((double)stamp2 - (double)stamp1));
		if (msecPerCycle < GMSecondsPerCycle) GMSecondsPerCycle = msecPerCycle;

#if 0
		appPrintf ("try #%d\n", tries);
		appPrintf ("  stampd: %u %u   timed: %d\n", stamp2 - stamp1, time2 - time1);
		appPrintf ("  CPU speed: %g MHz\n", 1e-3 / msecPerCycle);
#endif
	}
	GSecondsPerCycle = GMSecondsPerCycle / 1000.0;
	double rate = 1e-6 / GSecondsPerCycle;
	const char *units = "MHz";
	if (rate > 1000)							// modern CPU have GHz clock rates ...
	{
		rate /= 1000;
		units = "GHz";
	}
	appPrintf ("CPU speed: %.2f %s\n", rate, units);

	timeEndPeriod (1);
}


/*-----------------------------------------------------------------------------
	Detecting Windows version
-----------------------------------------------------------------------------*/

static void DetectOs ()
{
	struct osInfo_t {
		byte	vMaj, vMin, vPlatf;
		const char *name;
	};
	static const osInfo_t table[] = {
#define WIN9X	VER_PLATFORM_WIN32_WINDOWS
#define WINNT	VER_PLATFORM_WIN32_NT
		// Win9X
		{	4,0,	WIN9X,	"95"	},
		{	4,10,	WIN9X,	"98"	},
		{	4,90,	WIN9X,	"Me"	},
		// WinNT
		{	3,51,	WINNT,	"NT 3.51"},
		{	4,0,	WINNT,	"NT 4.0"},
		{	5,0,	WINNT,	"2000"	},
		{	5,1,	WINNT,	"XP"	},
		{	5,2,	WINNT,	"2003"	},
		{	6,0,	WINNT,	"Vista"	}
	};
	OSVERSIONINFO ver;
	ver.dwOSVersionInfoSize = sizeof(ver);
	if (!GetVersionEx (&ver))
	{
		int err = GetLastError ();
		appPrintf ("Windows version detection failed:\nError: %d  %s\n", err, appGetSystemErrorMessage (err));
		return;
	}
	GIsWinNT = (ver.dwPlatformId == VER_PLATFORM_WIN32_NT);
	GIsWin2K = (GIsWinNT && ver.dwMajorVersion >= 5);
	for (int i = 0; i < ARRAY_COUNT(table); i++)
	{
		const osInfo_t &v = table[i];
		if (v.vPlatf == ver.dwPlatformId &&
			v.vMaj == ver.dwMajorVersion &&
			v.vMin == ver.dwMinorVersion)
		{
			appSprintf (ARRAY_ARG(GMachineOS), "Windows %s (Build: %d)", v.name, ver.dwBuildNumber);
			appPrintf ("Detected OS: %s\n", GMachineOS);
			return;
		}
	}
	const char *platf = "Unknown";
	if (ver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		platf = "9X";
	else if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT)
		platf = "NT";
	appSprintf (ARRAY_ARG(GMachineOS), "Windows %s %d.%d (Build: %d)", platf, ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber);
	appPrintf ("Detected OS: %s\n", GMachineOS);
}


/*-----------------------------------------------------------------------------
	Miscellaneous
-----------------------------------------------------------------------------*/

const char *appPackage ()
{
	static TString<256> Filename;
	if (Filename[0]) return Filename;	// already computed
	// get executable filename
	GetModuleFileName (NULL, ARRAY_ARG(Filename));
	char *s = Filename.rchr ('\\');
	if (s) Filename = s+1;
	s = Filename.rchr ('.');
	if (s) *s = 0;
	return Filename;
}


static void SetDefaultDirectory ()
{
	TString<256> Filename;
	GetModuleFileName (NULL, ARRAY_ARG(Filename));
	char *s = Filename.rchr ('\\');
	if (s) *s = 0;
	SetCurrentDirectory (Filename);
}


const char *appGetSystemErrorMessage (unsigned code)
{
	char buffer[1024];
	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code,
		MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT),
		ARRAY_ARG(buffer), NULL))
	{
		// cut CR/LF
		char *s = strchr (buffer, '\r');
		if (s) s[0] = 0;
		return va("%s", buffer);
	}
	return "unknown error code";
}


void appDisplayError ()
{
	MessageBox (NULL,
#if DO_GUARD
		va("%s\n\nHistory: %s", *GErr.Message, *GErr.History),
#else
		GErr.Message,
#endif
		va("%s: fatal error", appPackage ()), MB_OK|MB_ICONSTOP/*|MB_TOPMOST*/|MB_SETFOREGROUND);
	// add CR/LF to error log and close it
	COutputDevice *ErrLog = appGetErrorLog ();
	ErrLog->Write ("\n\n");
	ErrLog->Close ();
}


/*-----------------------------------------------------------------------------
	Initialization/finalization
-----------------------------------------------------------------------------*/

void appInitPlatform ()
{
	hInstance = GetModuleHandle (NULL);
	// gather system information
	CheckCpuModel ();
	CheckCpuSpeed ();
	DetectOs ();
	// setup current directory
	SetDefaultDirectory ();
#if 0
	// DEBUG: force mmsystem timing functions
	IsRDTSC = false;
	appWPrintf ("DEBUG: force mmsystem time measuring\n");
#endif
	// NOTE: under WinXP (Win2k too?) without timeBeginPeriod(1) Sleep() (and some another time-related
	//	functions too) will work in bad resolution; so, we should use timeBeginPeriod(1) even if don't
	//	use timeGetTime()
	timeBeginPeriod (1);
	if (!IsRDTSC) timeBase = timeGetTime ();
}


void appShutdownPlatform ()
{
	timeEndPeriod (1);
}
