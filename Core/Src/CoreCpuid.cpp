#include "CorePrivate.h"


/*-----------------------------------------------------------------------------
	Global variables
-----------------------------------------------------------------------------*/

// really, should use GCyclesPerMSecond, but this will require divide operation every
// time we use this, so GMSecondsPerCycle=1/GCyclesPerMSecond
double GMSecondsPerCycle;

char GMachineCPU[64] = "Unknown 386/486 CPU";

// currently static; use outside??
static bool IsMMX, IsSSE, Is3DNow;
static bool IsRDTSC;				// is there any Pentium-class CPU w/o RDTSC support?


/*-----------------------------------------------------------------------------
	CPU identification
-----------------------------------------------------------------------------*/

static unsigned cpuidRegs[4];


#if _MSC_VER

static void cpuid(unsigned code)
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

#pragma warning(push)
#pragma warning(disable : 4035)		// "no return value"
inline unsigned cpuid0(unsigned code)
{
	__asm {
		mov		eax,code
		cpuid
	}
}

inline unsigned cpuid2(unsigned code)
{
	__asm {
		mov		eax,code
		cpuid
		mov		eax,ecx
	}
}

inline unsigned cpuid3(unsigned code)
{
	__asm {
		mov		eax,code
		cpuid
		mov		eax,edx
	}
}
#pragma warning(pop)


#else // !_MSC_VER => __GNUC__


static void cpuid(unsigned code)
{
	__asm __volatile__
	("cpuid"
	: "=a" (cpuidRegs[0]), "=b" (cpuidRegs[1]), "=c" (cpuidRegs[2]), "=d" (cpuidRegs[3])
	: "a" (code));
}

inline unsigned cpuid0(unsigned code)
{
#if 0
	cpuid(code);
	return cpuidRegs[0];
#else
	unsigned r0, r1, r2, r3;
	__asm __volatile__
	("cpuid"
	: "=a" (r0), "=b" (r1), "=c" (r2), "=d" (r3)
	: "a" (code));
	return r0;
#endif
}

inline unsigned cpuid2(unsigned code)
{
#if 0
	cpuid(code);
	return cpuidRegs[2];
#else
	unsigned r0, r1, r2, r3;
	__asm __volatile__
	("cpuid"
	: "=a" (r0), "=b" (r1), "=c" (r2), "=d" (r3)
	: "a" (code));
	return r2;
#endif
}

inline unsigned cpuid3(unsigned code)
{
#if 0
	cpuid(code);
	return cpuidRegs[3];
#else
	unsigned r0, r1, r2, r3;
	__asm __volatile__
	("cpuid"
	: "=a" (r0), "=b" (r1), "=c" (r2), "=d" (r3)
	: "a" (code));
	return r3;
#endif
}

#endif // _MSC_VER


static void CheckCpuModel()
{
#define MAX_CPU_NAME_PARTS	12		// 3 times of 4 regs
	union {
		unsigned reg[MAX_CPU_NAME_PARTS];
		char	name[MAX_CPU_NAME_PARTS*4+1];
	} t;

	// cpuid presence
	unsigned cpu0;
	TRY {
		cpu0 = cpuid0(0x80000000);
	} CATCH {
		// no CPUID available
		appPrintf("CPU: %s\n", GMachineCPU);
		return;
	} END_CATCH

	// CPU name
	if (cpu0 >= 0x80000004)			// extended vendor string available
	{
		int i = 0;
		for (unsigned fn = 0x80000002; fn <= 0x80000004; fn++)
		{
			cpuid(fn);
			memcpy(t.reg+i, cpuidRegs, sizeof(cpuidRegs));
			i += 4;
		}
		t.name[i*4] = 0;
		const char *s = t.name;
		while (*s == ' ') s++;		// skip leading spaces
		appStrncpyz(GMachineCPU, s, sizeof(GMachineCPU));
	}
	else
	{
		cpuid(0);
		t.reg[0] = cpuidRegs[1];
		t.reg[1] = cpuidRegs[3];
		t.reg[2] = cpuidRegs[2];
		t.name[12] = 0;				// null-terminated
		appStrncpyz(GMachineCPU, t.name, sizeof(GMachineCPU));
	}
	appPrintf("CPU name: %s\n", GMachineCPU);

	// caps
	appPrintf("CPU caps: [ ");

	unsigned tmp  = cpuid3(1);
	unsigned tmp2 = cpuid2(1);
	if (tmp & 0x00000001)	appPrintf("FPU ");
	if (tmp & 0x00000010)
	{
		appPrintf("RDTSC ");
		IsRDTSC = true;
	}
	//!! NOTE: if planning to use MMX/SSE/SSE2/3DNow! - should check OS support for this tech
	if (tmp & 0x00800000)
	{
		appPrintf("MMX ");
		IsMMX = true;
	}
	if (tmp & 0x02000000)
	{
		appPrintf("SSE ");
		IsSSE = true;
	}
	if (tmp  & 0x04000000)	appPrintf("SSE2 ");
	if (tmp2 & 0x00000001)	appPrintf("SSE3 ");
	if (tmp2 & 0x00080000)	appPrintf("SSE4.1 ");
	if (tmp2 & 0x00100000)	appPrintf("SSE4.2 ");

	// check extended features
	if (cpu0 >= 0x80000001)			// largest recognized extended function
	{
		tmp = cpuid3(0x80000001);
		if (tmp & 0x80000000)
		{
			appPrintf("3DNow! ");
			Is3DNow = true;
		}
	}

	appPrintf("]\n");
}


static void CheckCpuSpeed()
{
	if (!IsRDTSC) return;

	GMSecondsPerCycle = 1.0;		// initial value (1 Hz)
	for (int tries = 0; tries < 3; tries++)
	{
#if 1
		unsigned time1 = appMilliseconds();
#else
		unsigned time1 = appMilliseconds();
		// catch time change in OS resolution
		while (true)
		{
			unsigned time3 = appMilliseconds();
			if (time3 != time1)
			{
				time1 = time3;
				break;
			}
		}
#endif
		int64 stamp1 = appCycles64();
		unsigned time2;
		while (true)
		{
			time2 = appMilliseconds();
			if (time2 - time1 > 150) break;
			//?? may be, appSleep() when time distance is long (>20ms)
		}
		int64 stamp2 = appCycles64();
		double msecPerCycle = (time2 - time1) / double(stamp2 - stamp1);
		if (msecPerCycle < GMSecondsPerCycle) GMSecondsPerCycle = msecPerCycle;

#if 0
		appPrintf("try #%d\n", tries);
		appPrintf("  stampd: %u %u   timed: %d\n", stamp2 - stamp1, time2 - time1);
		appPrintf("  CPU speed: %g MHz\n", 0.001 / msecPerCycle);
#endif
	}
	double rate = 0.001 / GMSecondsPerCycle;
	const char *units = "MHz";
	if (rate > 1000)				// modern CPU have GHz clock rates ...
	{
		rate /= 1000;
		units = "GHz";
	}
	appPrintf("CPU speed: %.2f %s\n", rate, units);
}


void appDetectCPU()
{
	CheckCpuModel();
	CheckCpuSpeed();
}
