/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sys_win.h

#include "../qcommon/qcommon.h"
#include "../client/client.h"
#include "winquake.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include "../win32/conproc.h"

#define MINIMUM_WIN_MEMORY	0x0a00000
#define MAXIMUM_WIN_MEMORY	0x1000000

//#define DEMO

qboolean s_win95;

int			starttime;
qboolean	ActiveApp;
qboolean	Minimized;

static HANDLE		hinput, houtput;

unsigned	sys_msg_time;
unsigned	sys_frame_time;


static HANDLE		qwclsemaphore;

#define	MAX_NUM_ARGVS	128
int			argc;
char		*argv[MAX_NUM_ARGVS];


/*
===============================================================================

SYSTEM IO

===============================================================================
*/


void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	CL_Shutdown ();
	Qcommon_Shutdown ();

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	MessageBox(NULL, text, "Error", 0 /* MB_OK */ );

	if (qwclsemaphore)
		CloseHandle (qwclsemaphore);

	// shut down QHOST hooks if necessary
	DeinitConProc ();

	exit (1);
}

void Sys_Quit (void)
{
	timeEndPeriod( 1 );

	CL_Shutdown();
	Qcommon_Shutdown ();
	CloseHandle (qwclsemaphore);
	if (dedicated && dedicated->integer)
		FreeConsole ();

	// shut down QHOST hooks if necessary
	DeinitConProc ();

	exit (0);
}


void WinError (void)
{
	LPVOID lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL
	);

	// Display the string.
	MessageBox( NULL, lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );

	// Free the buffer.
	LocalFree( lpMsgBuf );
}

//================================================================


/*
================
Sys_ScanForCD

================
*/
char *Sys_ScanForCD (void)
{
	static char	cddir[MAX_OSPATH];
	static qboolean	done;
#ifndef DEMO
	char		drive[4];
	FILE		*f;
	char		test[MAX_QPATH];

	if (done)		// don't re-check
		return cddir;

	// no abort/retry/fail errors
	SetErrorMode (SEM_FAILCRITICALERRORS);

	drive[0] = 'c';
	drive[1] = ':';
	drive[2] = '\\';
	drive[3] = 0;

	done = true;

	// scan the drives
	for (drive[0] = 'c'; drive[0] <= 'z'; drive[0]++)
	{
		// where activision put the stuff...
		Com_sprintf (cddir, sizeof(cddir), "%sinstall\\data", drive);
		Q_CopyFilename (cddir, cddir, sizeof(cddir) - 1);
		Com_sprintf (test, sizeof(test), "%sinstall\\data\\quake2.exe", drive);
		f = fopen (test, "r");
		if (f)
		{
			fclose (f);
			if (GetDriveType (drive) == DRIVE_CDROM)
				return cddir;
		}
	}
#endif

	cddir[0] = 0;

	return NULL;
}

/*
================
Sys_CopyProtect

================
*/
void	Sys_CopyProtect (void)
{
#ifndef DEMO
	char	*cddir;

	cddir = Sys_ScanForCD();
	if (!cddir[0])
		Com_Error (ERR_FATAL, "You must have the Quake2 CD in the drive to play.");
#endif
}


//================================================================

/* If error happens in ref_xxx.dll, error message will contain reference to
 * <not available> module
 */

static void DumpReg4 (FILE *f, char *name, DWORD value)
{
	int i;
	char *data;

	data = (char*) value;
	fprintf (f, "  %s: %08X  ", name, value);
	if (IsBadReadPtr (data, 16))
		fprintf (f, " <N/A>");
	else
	{
		for (i = 0; i < 16; i++)
			fprintf (f, " %02X", data[i] & 0xFF);

		fprintf (f, "  ");

		for (i = 0; i < 16; i++)
		{
			char c;

			c = data[i];
			if (c < ' ' || c > 0x7F) c = '.';
			fprintf (f, "%c", c);
		}
	}
	fprintf (f, "\n");
}

static void DumpReg2 (FILE *f, char *name, DWORD value)
{
	fprintf (f, "  %s: %04X", name, value);
}

static int DumpMem (FILE *f, int *data)
{
	int i;

	if (IsBadReadPtr (data, 32)) return 0;
	for (i = 0; i < 8; i++)
		fprintf (f, "  %08X", data[i]);
	fprintf (f, "\n");
	return 1;
}

static int exception_count = 0;
extern qboolean debugLogged;

static LONG WINAPI ExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	FILE *f;
	CONTEXT *ctx;
	EXCEPTION_RECORD *rec;
	char ctime[256];
	char module[MAX_OSPATH];
	time_t itime;
	MEMORY_BASIC_INFORMATION mbi;
	int i;
	int *stack;

	if (exception_count++ > 0) return 0; // nested exception

	// make a log in "crush.log"
	if (f = fopen ("crush.log", "a+"))
	{
		time (&itime);
		strftime (ctime, sizeof(ctime), "%a %b %d, %Y (%H:%M:%S)", localtime (&itime));
		fprintf (f, "----- Quake2 crush log on %s -----\n", ctime);
		ctx = ExceptionInfo->ContextRecord;
		rec = ExceptionInfo->ExceptionRecord;
		strcpy (module, "<N/A>");
		if (VirtualQuery ((void*)ctx->Eip, &mbi, sizeof(mbi)))
		{
			if (mbi.State != MEM_COMMIT || !GetModuleFileName (mbi.AllocationBase, module, sizeof(module)));	//??
		}
		fprintf (f, "Exception %08X at address %08X in module %s\n", rec->ExceptionCode, rec->ExceptionAddress, module);
//		fprintf (f, "Base registers:\n");
		DumpReg4 (f, "EAX", ctx->Eax);
		DumpReg4 (f, "EBX", ctx->Ebx);
		DumpReg4 (f, "ECX", ctx->Ecx);
		DumpReg4 (f, "EDX", ctx->Edx);
		DumpReg4 (f, "ESI", ctx->Esi);
		DumpReg4 (f, "EDI", ctx->Edi);
		DumpReg4 (f, "EBP", ctx->Ebp);
		DumpReg4 (f, "ESP", ctx->Esp);
		DumpReg4 (f, "EIP", ctx->Eip);
//		fprintf (f, "Segment registers:\n");
		DumpReg2 (f, "CS", ctx->SegCs); DumpReg2 (f, "SS", ctx->SegSs);
		DumpReg2 (f, "DS", ctx->SegDs); DumpReg2 (f, "ES", ctx->SegEs);
		DumpReg2 (f, "FS", ctx->SegFs); DumpReg2 (f, "GS", ctx->SegGs);
		fprintf (f, "  EFLAGS: %08X\n", ctx->EFlags);
		stack = (int*) ctx->Esp;
		fprintf (f, "Stack frame (CS:%08X):\n", stack);
		for (i = 0; i < 16; i++)
		{
			if (!DumpMem (f, stack)) break;
			stack += 8;
		}
		fprintf (f, "\n");
		fclose (f);
	}
	if (debugLogged)
		DebugPrintf ("***** CRUSH *****\n");
	// really, we can perform CL_Shutdown(), but this will save config - it is not safely ...
	IN_Shutdown ();
	Vid_Shutdown ();
	S_Shutdown ();
	// show exception message
	return 0;
}

/*
================
Sys_Init
================
*/
void Sys_Init (void)
{
	OSVERSIONINFO	vinfo;

#if 0
	// allocate a named semaphore on the client so the
	// front end can tell if it is alive

	// mutex will fail if semephore already exists
    qwclsemaphore = CreateMutex(
        NULL,         /* Security attributes */
        0,            /* owner       */
        "qwcl"); /* Semaphore name      */
	if (!qwclsemaphore)
		Sys_Error ("QWCL is already running on this system");
	CloseHandle (qwclsemaphore);

    qwclsemaphore = CreateSemaphore(
        NULL,         /* Security attributes */
        0,            /* Initial count       */
        1,            /* Maximum count       */
        "qwcl"); /* Semaphore name      */
#endif

	timeBeginPeriod( 1 );

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if (!GetVersionEx (&vinfo))
		Sys_Error ("Couldn't get OS info");

	if (vinfo.dwMajorVersion < 4)
		Sys_Error ("Quake2 requires windows version 4 or greater");
	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("Quake2 doesn't run on Win32s");
	else if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
		s_win95 = true;

	if (dedicated->integer)
	{
		if (!AllocConsole ())
			Sys_Error ("Couldn't create dedicated server console");
		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);

		// let QHOST hook in
		InitConProc (argc, argv);
	}
	SetUnhandledExceptionFilter (ExceptionFilter);
}


static char	console_text[256];
static int	console_textlen;

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
	INPUT_RECORD	recs[1024];
	int		dummy;
	int		ch, numread, numevents;

	if (!dedicated || !dedicated->integer)
		return NULL;


	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Sys_Error ("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, &numread))
			Sys_Error ("Error reading console input");

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
					case '\r':
						WriteFile(houtput, "\r\n", 2, &dummy, NULL);

						if (console_textlen)
						{
							console_text[console_textlen] = 0;
							console_textlen = 0;
							return console_text;
						}
						break;

					case '\b':
						if (console_textlen)
						{
							console_textlen--;
							WriteFile(houtput, "\b \b", 3, &dummy, NULL);
						}
						break;

					default:
						if (ch >= ' ')
						{
							if (console_textlen < sizeof(console_text)-2)
							{
								WriteFile(houtput, &ch, 1, &dummy, NULL);
								console_text[console_textlen] = ch;
								console_textlen++;
							}
						}

						break;

				}
			}
		}
	}

	return NULL;
}


/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput (char *string)
{
	int		dummy;
	char	text[256];

	if (!dedicated || !dedicated->integer)
		return;

	if (console_textlen)
	{
		text[0] = '\r';
		memset(&text[1], ' ', console_textlen);
		text[console_textlen+1] = '\r';
		text[console_textlen+2] = 0;
		WriteFile(houtput, text, console_textlen+2, &dummy, NULL);
	}

	WriteFile(houtput, string, strlen(string), &dummy, NULL);

	if (console_textlen)
		WriteFile(houtput, console_text, console_textlen, &dummy, NULL);
}


/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents (void)
{
    MSG        msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit ();
		sys_msg_time = msg.time;
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	// grab frame time
	sys_frame_time = timeGetTime();	// FIXME: should this be at start?
}



/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void )
{
	char *data = NULL;
	char *cliptext;

	if (OpenClipboard (NULL))
	{
		HANDLE	hClipboardData;

		if (hClipboardData = GetClipboardData (CF_TEXT))
		{
			if (cliptext = GlobalLock (hClipboardData))
			{
				data = malloc (GlobalSize (hClipboardData) + 1);
				strcpy (data, cliptext);
				GlobalUnlock (hClipboardData);
			}
		}
		CloseClipboard ();
	}
	return data;
}

/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/

/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate (void)
{
	ShowWindow (cl_hwnd, SW_RESTORE);
	SetForegroundWindow (cl_hwnd);
}

/*
========================================================================

GAME DLL

========================================================================
*/

static HINSTANCE	game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	if (!FreeLibrary (game_library))
		Com_Error (ERR_FATAL, "FreeLibrary failed for game library");
	game_library = NULL;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI (void *parms)
{
	void	*(*GetGameAPI) (void *);
	char	name[MAX_OSPATH];
	char	*path;
	char	cwd[MAX_OSPATH];
#if defined _M_IX86
	const char *gamename = "gamex86.dll";

#ifdef NDEBUG
	const char *debugdir = "release";
#else
	const char *debugdir = "debug";
#endif

#elif defined _M_ALPHA
	const char *gamename = "gameaxp.dll";

#ifdef NDEBUG
	const char *debugdir = "releaseaxp";
#else
	const char *debugdir = "debugaxp";
#endif

#endif

	if (game_library)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI() without Sys_UnloadGame()");

	// check the current debug directory first for development purposes
	_getcwd (cwd, sizeof(cwd));
	Com_sprintf (name, sizeof(name), "%s/%s/%s", cwd, debugdir, gamename);
	game_library = LoadLibrary (name);
	if (game_library)
	{
		Com_DPrintf ("LoadLibrary (%s)\n", name);
	}
	else
	{
#ifdef DEBUG
		// check the current directory for other development purposes
		Com_sprintf (name, sizeof(name), "%s/%s", cwd, gamename);
		game_library = LoadLibrary (name);
		if (game_library)
		{
			Com_DPrintf ("LoadLibrary (%s)\n", name);
		}
		else
#endif
		{
			// now run through the search paths
			path = NULL;
			while (1)
			{
				path = FS_NextPath (path);
				if (!path)
					return NULL;		// couldn't find one anywhere
				Com_sprintf (name, sizeof(name), "%s/%s", path, gamename);
				game_library = LoadLibrary (name);
				if (game_library)
				{
					Com_DPrintf ("LoadLibrary (%s)\n",name);
					break;
				}
			}
		}
	}

	GetGameAPI = (void *)GetProcAddress (game_library, "GetGameAPI");
	if (!GetGameAPI)
	{
		Sys_UnloadGame ();
		return NULL;
	}

	return GetGameAPI (parms);
}

//=======================================================================


/*
==================
ParseCommandLine

==================
*/
void ParseCommandLine (LPSTR lpCmdLine)
{
	argc = 1;
	argv[0] = "exe";

	while (*lpCmdLine && (argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[argc] = lpCmdLine;
			argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}

		}
	}
}

/*
==================
WinMain

==================
*/
HINSTANCE	global_hInstance;

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG		msg;
	int		time, oldtime, newtime;
	char	*cddir;

	global_hInstance = hInstance;

	ParseCommandLine (lpCmdLine);

	// if we find the CD, add a "+set cddir xxx" command line
	cddir = Sys_ScanForCD ();
	if (cddir && argc < MAX_NUM_ARGVS - 3)
	{
		int		i;

		// don't override a cddir on the command line
		for (i = 0; i < argc; i++)
			if (!stricmp (argv[i], "cddir")) break;
		if (i == argc)	// not found
		{
			argv[argc++] = "+set";
			argv[argc++] = "cddir";
			argv[argc++] = cddir;
		}
	}

	Qcommon_Init (argc, argv);
	oldtime = Sys_Milliseconds ();

	/*--------- main window message loop ------------*/
	while (1)
	{
		// if at a full screen console, don't update unless needed
		if (Minimized || (dedicated && dedicated->integer))
			Sleep (1);

		while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage (&msg, NULL, 0, 0))
				Com_Quit ();
			sys_msg_time = msg.time;
			TranslateMessage (&msg);
   			DispatchMessage (&msg);
		}

		do
		{
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
		} while (time < 1);
//		Com_Printf ("time:%5.2f - %5.2f = %5.2f\n", newtime, oldtime, time);

//		_controlfp (~( _EM_ZERODIVIDE /*| _EM_INVALID*/ ), _MCW_EM);
//		_controlfp (_PC_24, _MCW_PC);
		Qcommon_Frame (time);

		oldtime = newtime;
	}

	// never gets here
	return TRUE;
}
