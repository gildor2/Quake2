#if _WIN32
#include <windows.h>		// here for COutputDeviceCon only!
#endif

#include "Core.h"

#if 0
#include "OutputDeviceFile.h"
static COutputDeviceStdout	Out;
#else
#include "OutputConsole.h"
static COutputDeviceCon		Out;
#endif

#include "FileContainerPak.h"
#include "FileContainerZip.h"


char tests[32] = "TestApp.cpp";
void crash (char *b)
{
	static float f = 13;
	float	z;

	// sprintf overflow test
	char buf[8];
	appSprintf (ARRAY_ARG(buf), "123%s288", "asksjk");

	guard(crash);
	z = f;
	if (strlen (b) > 0) z = 0;
	appPrintf ("%s %f\n", b, z);
	f = f / z;		// should be "float zero divide" ... (not works for WinXP); test for win98!!
	appPrintf ("f=%g\n", f);
	*(byte*)NULL = 0;
	unguard;
}


void cXcpt (int argc, char **argv)
{
	guard(excpt);

	if (argc) appPrintf ("xcpt - args: %d\n", argc);
	int i;
	for (i = 0; i < argc; i++) appPrintf(".. arg[%d] = <%s>\n", i, argv[i]);

	// absolutely crazy function (does nothing useful but different exception tests)
	static int a;
	char c = argc > 0 ? argv[1][0] : '0';
	switch (c)
	{
#define TEST(seq)	appPrintf ("%s", "Testing: \"" STR(seq) "\"\n"); seq;
	case '1':
		TEST(a = i / (argc - 2));				// zero divide
		break;
	case '2':
		TEST(*((byte*)NULL) = 0);				// memory access
		break;
	case '3':
		TEST(crash(tests));						// some tests
		break;
	case '4':
		TEST(((void (*)()) NULL) ());			// jump to null address
		break;
	case '5':
		TEST(appStrcmp (NULL, tests));			// crash in local function
		break;
	case '6':
		TEST(sprintf ((char*)1, "%s", tests));	// crash in kernel/libc
		break;
	case '7':
#if 0
		TEST(throw "exception");				// software exception
#else
		appPrintf ("... disabled\n");
#endif
		break;
	case '8':
		{
			TString<256> Temp; Temp = "temp string";
			TEST(appPrintf ("Temp=%s\n", Temp)); // shound use "*Temp" -- so, error in vsnprintf()
		}
		break;
	case 'a':
		{
			TRY {
				appPrintf ("testing TRY_S/CATCH_S ...\n");
				TEST(*((byte*)NULL) = 0);		// memory access
				appPrintf ("should not be here!\n");
			} CATCH {
				appPrintf ("catched exception!\n");
			} END_CATCH
			appPrintf ("continue!\n");
		}
		break;
	case 'b':
		{
			appPrintf ("testing double TRY_S ...\n");
			TRY {
				TRY {
					TEST(*((byte*)NULL) = 0);	// memory access
				} CATCH {
					appPrintf ("catched at 2\n");
				} END_CATCH
				appPrintf ("cont#2\n");
			} CATCH {
				appPrintf ("catched at 1\n");
			} END_CATCH
			appPrintf ("cont#1\n");
		}
		break;
	default:
		cXcpt(0, NULL);							// infinite recurse
		a = i + 1;
#undef TEST
	}
	unguard;
}


void cAlloc (int argc, char **argv)
{
	if (argc < 2 || argc > 3) return;
	int size = atoi (argv[1]);
	int al = argc == 2 ? DEFAULT_ALIGNMENT : atoi (argv[2]);
	appPrintf ("alloc: %d [%d]\n", size, al);
	void *ptr = appMalloc (size, al);
	appPrintf ("... ret = %X\n", ptr);
}


void cUsage (bool a)
{
	if (a)
		appPrintf ("usage: usage\n");
	else
		appPrintf ("called \"usage\"\n");
}


void cArgs (int argc, char **argv)
{
	appPrintf ("args cmd: %d\n", argc);
	for (int i = 0; i < argc; i++) appPrintf(".. arg[%d] = <%s>\n", i, argv[i]);
}

void cBoth (bool usage, int argc, char **argv)
{
	if (usage)
		appPrintf ("usage: both [args]\n");
	else
	{
		appPrintf ("both cmd: %d\n", argc);
		for (int i = 0; i < argc; i++) appPrintf(".. arg[%d] = <%s>\n", i, argv[i]);
	}
}

void cArgs2 (const char *args)
{
	appPrintf ("args cmd: %s\n", args);
}

void cBoth2 (bool usage, const char *args)
{
	if (usage)
		appPrintf ("usage: both2 [args]\n");
	else
		appPrintf ("both2 cmd: %s\n", args);
}

void cSym (bool usage, int argc, char **argv)
{
	if (usage || argc != 2)
	{
		appPrintf ("Usage: sym <address>\n");
		return;
	}
	unsigned addr = strtol (argv[1], NULL, 0);
	appPrintf ("Info on address %08X : ", addr);
	char info[256];
	if (!appSymbolName (addr, ARRAY_ARG(info)))
		appPrintf ("failed\n");
	else
		appPrintf ("%s\n", info);
}

void cSysErr (int argc, char **argv)
{
	int code = atoi (argv[1]);
	appPrintf ("err(%d) = \"%s\"\n", code, appGetSystemErrorMessage (code));
}

void cSleep (bool usage, int argc, char **argv)
{
	if (usage)
	{
		appPrintf ("Usage: sleep <msec> [<num_times>]\n");
		return;
	}
	int time = atoi (argv[1]);
	if (time < 0) time = 0;
	int times = atoi (argv[2]);
	if (times < 1) times = 1;

	for (int i = 0; i < times; i++)
	{
		int64 t1 = appCycles64 ();
		appSleep (time);
		int64 t2 = appCycles64 ();
		float t3 = appCyclesToMsecf (t2 - t1);
		appPrintf ("%d: %.3f (%+.3f)\n", i, t3, t3 - time);
	}
}

#if !_WIN32
//!!! move most code to CoreUnix.cpp
#include <termios.h>				// tc* functions
#include <unistd.h>
#include <fcntl.h>					// fcntl()

void cInput ()
{
	if (!isatty (STDIN_FILENO))
	{
		appWPrintf ("ERROR: stdin is not a terminal\n");
		//?? work in non-terminal mode?
		return;
	}
#if __CYGWIN__
	//?? cygwin bug: execution will be blocked until input received
	int oldCtl = fcntl (STDIN_FILENO, F_GETFL);
	fcntl (STDIN_FILENO, F_SETFL, oldCtl|O_NONBLOCK);
#endif
	// request attributes
	struct termios tc;
	tcgetattr (STDIN_FILENO, &tc);
	// modify attributes: disable non-canonical terminal mode (no buffered input)
	struct termios tc2 = tc;
	tc2.c_lflag     &= ~(ECHO|ICANON);
	tc2.c_cc[VMIN]  = 0;
	tc2.c_cc[VTIME] = 0;
	tcsetattr (STDIN_FILENO, TCSANOW, &tc2);
	// input loop
	while (true)
	{
		if (GIsRequestingExit) break;
#if 0
		fd_set fd;
		struct timeval tv;
		FD_ZERO(&fd);
		FD_SET(STDIN_FILENO, &fd);
		tv.tv_sec  = 0;
		tv.tv_usec = 0;
		if (select (1, &fd, NULL, NULL, &tv) == -1 || FD_ISSET(0, &fd))
		{
			appSleep (100);
			continue;
		}
#endif

		char buf;
		if (read (STDIN_FILENO, &buf, 1) < 1)
		{
			printf ("*"); fflush (stdout);
			appSleep (200);
			continue;
		}
		appPrintf ("%02X = [%c]\n", buf, buf < ' ' ? '?' : buf);
		if (buf == 4)
		{
			appPrintf ("Ctrl-D: break\n");
			break;
		}
	}
	// restore tc
	//!! WARNING: should exec after Ctrl-C too!
	tcsetattr (STDIN_FILENO, TCSANOW, &tc);
#if __CYGWIN__
	fcntl (STDIN_FILENO, F_SETFL, oldCtl);
#endif
}
#endif


//???????????????????????????????????????????

class CClassEvent
{
private:
	// local types
	class CDummy { /* empty */ };
	typedef void (CDummy::*EventFunc_t)();
	// data fields
	EventFunc_t func;
	CDummy		*obj;
public:
	CClassEvent ()
	{}
	template<class T> CClassEvent (T &object, void (T::*method)())
	:	func((EventFunc_t)method)
	,	obj((CDummy*)&object)
	{}
	template<class T> void set (T &object, void (T::*method)())
	{
		func = (EventFunc_t)method;
		obj  = (CDummy*)&object;
	}
	FORCEINLINE void operator() (void) const
	{
		(obj->*func)();
	}
};


class CTest
{
private:
	int		n;
public:
	CTest (int _n)
	:	n(_n)
	{}
	void OnT ()
	{
		appPrintf ("n=%d\n", n);
	}
};


int main (int argc, char** argv)
{
	TRY_CRASH {
		Out.Register ();
		appInit ();

/*		CTest tst1(13);
		CTest tst2(24);
		CClassEvent evt(tst2, &CTest::OnT);
		evt ();
*/
		// testing file system
		CFileSystem FS;
		appInitFileSystem (FS);
//		CFileSystem::RegisterFormat (CFileContainerPak::Create);
//		CFileSystem::RegisterFormat (CFileContainerZip::Create);
		CFileContainer *Cont = FS.MountDirectory (".");	// standard mount
		Cont->locked = true;
#if 0
		FS.MountDirectory ("../Core", "./A");		// simple mount, dir not exists
		FS.MountDirectory ("../ref_gl/qgl", "B/C");	// 2 nested dirs, not exists
		FS.MountDirectory ("../lib", ".hidden");	// hidden mount (can be listed with "mount")

		FS.MountArchive ("test.pak", "pak");
		FS.MountArchive ("c:/games/quake2/baseq2/pak10.pak");
#endif

		// testing commands
		RegisterCommand ("xcpt", cXcpt);
		RegisterCommand ("args", cArgs);
		RegisterCommand ("usage", cUsage);
		RegisterCommand ("both", cBoth);
		RegisterCommand ("args2", cArgs2);
		RegisterCommand ("both2", cBoth2);
		RegisterCommand ("alloc", cAlloc);
		RegisterCommand ("sym", cSym);
		RegisterCommand ("syserr", cSysErr);
		RegisterCommand ("sleep", cSleep);
#if !_WIN32
		RegisterCommand ("test", cInput);
#endif

		guard(MainLoop);
		while (!GIsRequestingExit)
		{
			appPrintf (">");
			char buf[1024];
			buf[0] = 0;
			gets (buf);
			if (!buf[0]) continue;

			if (!ExecuteCommand (buf))
				appWPrintf ("ERROR: bad command\n");
		}
		unguard;
	} CATCH_CRASH {
		// nothing ... just breaking MainLoop
	} END_CATCH

	appExit ();
	return 0;
}
