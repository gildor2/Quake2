#define WIN32_LEAN_AND_MEAN
#include <windows.h>			// for mmsystem.h
#include <mmsystem.h>			// for timeGetTime() staff
#include "Core.h"


double GSecondsPerCycle;		// really, should use GCyclesPerSecond, but this will require divide operation every
								// time we use this, so GSecondsPerCycle=1/GCyclesPerSecond
double GMSecondsPerCycle;		// == 1000*GSecondsPerCycle
static unsigned timeBase;

static bool IsMMX, IsSSE, IsRDTSC, Is3DNow;


// We should subtract timeBase from acquired appCycles64 value to determine time since
// application start, this is useful only for improving precision of retreived float
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


static bool cpuid (unsigned _in, unsigned regs[4])
{
	bool	isOK;

	isOK = true;
	__asm pushad;		// save all regs
	__try
	{
		__asm {
			mov		eax,_in
			cpuid
			mov		esi,regs
			mov		[esi],eax
			mov		[esi+4],ebx
			mov		[esi+8],ecx
			mov		[esi+12],edx
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		isOK = false;
	}
	__asm popad;
	return isOK;
}


static void CheckCpuName (void)
{
#define MAX_CPU_NAME_PARTS	12	// 3 times of 4 regs
	union {
		unsigned reg[MAX_CPU_NAME_PARTS];
		char	name[MAX_CPU_NAME_PARTS*4+1];
	} t;
	unsigned r[4];

	if (!cpuid (0x80000000, r))
	{
		// no CPUID available
		appPrintf ("Unknown 386/486 CPU\n");
		return;
	}
	if (r[0] >= 0x80000004)		// extended vendor string available
	{
		int i = 0;
		for (unsigned fn = 0x80000002; fn <= 0x80000004; fn++)
		{
			cpuid (fn, r);
			t.reg[i++] = r[0];
			t.reg[i++] = r[1];
			t.reg[i++] = r[2];
			t.reg[i++] = r[3];
		}
		t.name[i*4] = 0;
		const char *s = t.name;
		while (*s == ' ') s++;
		appPrintf ("CPU name: %s\n", s);
	}
	else
	{
		cpuid (0, r);
		t.reg[0] = r[1];
		t.reg[1] = r[3];
		t.reg[2] = r[2];
		t.name[12] = 0;		// ASCIIZ
		appPrintf ("CPU name: %s\n", t.name);
	}
}


static void CheckCpuCaps (void)
{
	unsigned r[4];

	if (!cpuid (1, r)) return;

	appPrintf ("CPU caps: [ ");

	if (r[3] & 0x00000001)	Com_Printf ("FPU ");
	if (r[3] & 0x00800000)
	{
		appPrintf ("MMX ");
		IsMMX = true;
	}
	if (r[3] & 0x02000000)
	{
		appPrintf ("SSE ");
		IsSSE = true;
	}
	if (r[3] & 0x04000000)	Com_Printf ("SSE2 ");
	if (r[3] & 0x00000010)
	{
		appPrintf ("RDTSC ");
		IsRDTSC = true;
	}

	// check extended features
	cpuid (0x80000000, r);
	if (r[0] >= 0x80000001)		// largest recognized extended function
	{
		cpuid (0x80000001, r);
		if (r[3] & 0x80000000)
		{
			appPrintf ("3DNow! ");
			Is3DNow = true;
		}
	}

	appPrintf ("]\n");
}


static void CheckCpuSpeed (void)
{
	if (!IsRDTSC) return;

	timeBeginPeriod (1);

	GMSecondsPerCycle = 1.0;					// initial value
	for (int tries = 0; tries < 4; tries++)
	{
		__int64 stamp1 = appCycles64 ();
		unsigned time1 = timeGetTime ();
		unsigned time2;
		while (true)
		{
			time2 = timeGetTime ();
			if (time2 - time1 > 200) break;		// 200ms enough to compute CPU speed
		}
		__int64 stamp2 = appCycles64 ();
		double msecPerCycle = (time2 - time1) / (((double)stamp2 - (double)stamp1));
		if (msecPerCycle < GMSecondsPerCycle) GMSecondsPerCycle = msecPerCycle;

//		Com_Printf ("try #%d\n", tries);
//		Com_Printf ("stampd: %u %u   timed: %d\n", stamp2 - stamp1, time2 - time1);
//		Com_Printf ("CPU speed: %g MHz\n", 1e-6 / secPerCycle1);
	}
	GSecondsPerCycle = GMSecondsPerCycle / 1000.0;
	appPrintf ("CPU speed: %g MHz\n", 1e-6 / GSecondsPerCycle);

	timeEndPeriod (1);
}


void appInitPlatform ()
{
	CheckCpuName ();
	CheckCpuCaps ();
	CheckCpuSpeed ();
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
