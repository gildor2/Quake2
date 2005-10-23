/*=============================================================================

	Core definitions for GCC compiler

=============================================================================*/


/*-----------------------------------------------------------------------------
	Type defines
-----------------------------------------------------------------------------*/

#if !__i386__
#error x86-only header (adaptation required)
#endif

#define LITTLE_ENDIAN		1

typedef unsigned char		byte;		//?? change name to BYTE etc
//??typedef unsigned short	word;
//??typedef unsigned int	dword;
typedef unsigned long long	int64;		//?? INT64

typedef unsigned int		address_t;


/*-----------------------------------------------------------------------------
	Miscellaneous
-----------------------------------------------------------------------------*/

#if __GNUC__ >= 4
// nothing exported by default
#pragma GCC visibility push(hidden)
#endif


#define DLL_IMPORT						// nothing?
#define DLL_EXPORT		__attribute__((visibility("default")))
#define NORETURN		__attribute__((noreturn))
#define PRINTF(n,m)		__attribute__((format(__printf__,n,m)))

#define	GET_RETADDR(firstarg)	((address_t)__builtin_return_address(0))

//!! staticAssert()


/*-----------------------------------------------------------------------------
	Some math functions
-----------------------------------------------------------------------------*/

//?? asm version here (check compiled code)
inline int appRound (float f)
{
	return lrintf (f);
}

inline int appFloor (float f)
{
	return lrintf (floorf (f));
}

inline int appCeil (float f)
{
	return lrintf (ceilf (f));
}


/*-----------------------------------------------------------------------------
	Pentium RDTSC timing
-----------------------------------------------------------------------------*/

inline unsigned appCycles ()
{
	unsigned r;
	__asm __volatile__
	("rdtsc" : "=a" (r));
	return r;
}

inline int64 appCycles64 ()
{
	int64 r;
	__asm __volatile__
	("rdtsc" : "=A" (r));	// "A" for x86 means 64-bit value in EDX:EAX
	return r;
}

extern CORE_API double GMSecondsPerCycle;

inline float appDeltaCyclesToMsecf (unsigned timeDelta)
{
	double v = timeDelta;
	return v * GMSecondsPerCycle;
}

inline float appDeltaCyclesToMsecf (int64 &timeDelta)
{
	// no "64-bit unsigned => double" conversion allowed ... use as signed
	double v = reinterpret_cast<__int64&>(timeDelta);
	return v * GMSecondsPerCycle;
}


/*-----------------------------------------------------------------------------
	guard/unguard macros
-----------------------------------------------------------------------------*/

// standard C++ try/catch block

#define guard(func)						\
	{									\
		static const char __FUNC__[] = #func; \
		try {

#define unguard							\
		} catch (...) {					\
			appUnwindThrow (__FUNC__);	\
		}								\
	}

#define unguardf(msg)					\
		} catch (...) {					\
			appUnwindPrefix (__FUNC__);	\
			appUnwindThrow msg;			\
		}								\
	}

#define TRY		try
#define CATCH	catch (...)


// VC compatibility:
#define __try			try
#define __except(x)		catch (...)
