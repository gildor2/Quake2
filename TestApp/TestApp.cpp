#include "Core.h"

#include "OutputDeviceFile.h"
static COutputDeviceStdout	Out;

#include "FileContainerPak.h"
#include "FileContainerZip.h"


char tests[256] = "TestApp.cpp";
void crash(char *b)
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
	f = f / z;
	appPrintf ("f=%g\n", f);
	*(byte*)NULL = 0;
	unguard;
}


void cmd1 (int argc, char **argv)
{
	guard(excpt);

	if (argc) appPrintf ("cmd1 - args: %d\n", argc);
	for (int i = 0; i < argc; i++) appPrintf(".. arg[%d] = <%s>\n", i, argv[i]);

	// absolutely crazy function (does nothing useful but different exception tests)
	static int a;
	char c = argc > 0 ? argv[1][0] : '0';
	switch (c)
	{
#define TEST(seq)	appPrintf ("Testing: \"" STR(seq) "\"\n"); seq;
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
		TEST(((void (*)(void)) NULL) ());		// jump to null address
		break;
	case '5':
		TEST(appStrcmp (NULL, tests));			// crash in local function
		break;
	case '6':
		TEST(sprintf ((char*)1, "%s", tests));	// crash in kernel
		break;
	case '7':
		TEST(throw "exception");				// software exception
		break;
	case '8':
		{
			TString<256> Temp; Temp = "temp string";
			TEST(appPrintf ("Temp=%s\n", Temp)); // shound use "*Temp" -- so, error in vsnprintf()
		}
		break;
	default:
		cmd1(0, NULL);							// infinite recurse
		a = i + 1;
#undef TEST
	}
	unguard;
}


void cUncol (int argc, char **argv)
{
	if (argc != 2) return;
	char dst[256];
	appUncolorizeString (dst, argv[1]);
	appPrintf("uncol (\"%s\") -> \"%s\"\n", argv[1], dst);
}


void cClen (int argc, char **argv)
{
	if (argc != 2) return;
	appPrintf("clen (\"%s\") = %d\n", argv[1], appCStrlen (argv[1]));
}


static CSimpleCommand commands[] = {
	{"uncol", cUncol},
	{"clen", cClen},
	{"xcpt",cmd1}
};


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


int main (int argc, char** argv)
{
	TRY {
		Out.Register ();
		appInit ();

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
		RegisterCommand ("args", cArgs);
		RegisterCommand ("usage", cUsage);
		RegisterCommand ("both", cBoth);
		RegisterCommand ("args2", cArgs2);
		RegisterCommand ("both2", cBoth2);
		RegisterCommand ("alloc", cAlloc);
		RegisterCommand ("sym", cSym);

		guard(MainLoop);
		while (!GIsRequestingExit)
		{
			appPrintf (">");
			char buf[1024];
			gets (buf);
			if (!buf[0]) continue;

			if (!ExecuteCommand (buf, ARRAY_ARG(commands)))
			{
				if (!ExecuteCommand (buf))
					appWPrintf ("ERROR: bad command\n");
			}
		}
		unguard;
	} CATCH {
		GIsFatalError = true;
	}

	appExit ();
	return 0;
}
