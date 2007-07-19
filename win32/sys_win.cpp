#include "WinPrivate.h"
#include "../client/client.h"		//!! for editLine[], CompleteCommand() + COutputDeviceCon
#if MAX_DEBUG
#	include <float.h>				// for _controlfp()
#endif

//!! TODO: COutputDeviceWin32: derive from COutputDeviceCon

#include "OutputDeviceMem.h"

bool		ActiveApp, MinimizedApp, FullscreenApp;

unsigned	sys_frame_time;			//?? used in cl_input.cpp only

//#define IS_CONSOLE_APP	1

#if !defined(IS_CONSOLE_APP) && DEDICATED_ONLY
#define IS_CONSOLE_APP		1
#endif


static char *ScanForCD()
{
#ifdef CD_PATH
	static TString<MAX_OSPATH> CdDir;
	static char drive[4] = "c:/";

	// no abort/retry/fail errors
	SetErrorMode(SEM_FAILCRITICALERRORS);

	// scan the drives
	for (drive[0] = 'c'; drive[0] <= 'z'; drive[0]++)
	{
		if (GetDriveType(drive) != DRIVE_CDROM) continue;
		if (FILE *f = fopen(va("%s"CD_CHECK, drive), "r"))
		{
			fclose(f);
			CdDir.filename(va("%s"CD_PATH, drive));
			return CdDir;
		}
	}
#endif
	return NULL;
}


/*-----------------------------------------------------------------------------
	Win32 console input/output
-----------------------------------------------------------------------------*/

static HANDLE hConInput;

#define MAX_CMDLINE		256		//?? same as in client/keys.h

static char	console_text[MAX_CMDLINE];
static int	console_textlen = 0;
static bool console_drawInput = true;


class COutputDeviceWin32Console : public COutputDevice
{
protected:
	HANDLE	hOutput;
public:
	void Init()
	{
#if !IS_CONSOLE_APP
		AllocConsole();
#endif
		hConInput = GetStdHandle(STD_INPUT_HANDLE);		//?? not OutputDevice-stuff
		hOutput   = GetStdHandle(STD_OUTPUT_HANDLE);
		Register();
		//!! should set error handler (Ctrl-C etc)
	}
	// writting without processing
	inline void WriteChar(char c)
	{
		DWORD dummy;
		WriteFile(hOutput, &c, 1, &dummy, NULL);
	}
	inline void WriteStr(const char *str)
	{
		DWORD dummy;
		WriteFile(hOutput, str, strlen(str), &dummy, NULL);
	}
	// writting with processing
	void Write(const char *str)
	{
		EraseInput();
		// draw message
		while (char c = *str++)
		{
			// parse color info
			if (c == COLOR_ESCAPE && str[0] >= '0' && str[0] <= '7')
			{
				static const byte colorTable[8] = {0, 4, 2, 6, 1, 5, 3, 7};
				SetConsoleTextAttribute(hOutput, colorTable[str[0] - '0']);
				str++;
				continue;
			}
			WriteChar(c & 0x7F);		// clear 7th bit
		}
		SetConsoleTextAttribute(hOutput, 7);
		// should draw input again
		console_drawInput = true;
	}
	void EraseInput()
	{
		if (console_drawInput) return;	// already erased
		char text[MAX_CMDLINE+2];
		text[0] = '\r';
		memset(&text[1], ' ', console_textlen+1);
		text[console_textlen+2] = '\r';
		text[console_textlen+3] = 0;
		DWORD dummy;
		WriteFile(hOutput, text, console_textlen+3, &dummy, NULL);
	}
};

#if !NO_DEDICATED
static COutputDeviceWin32Console Win32Log;
#endif


char *Sys_ConsoleInput()
{
	guard(Sys_ConsoleInput);

#if !NO_DEDICATED
	if (console_drawInput)
	{
		// display input line
		Win32Log.WriteChar(']');
		if (console_textlen)
		{
			console_text[console_textlen] = 0;
			Win32Log.WriteStr(console_text);
		}
		console_drawInput = false;
	}

	while (true)
	{
		DWORD	numevents, numread;
		GetNumberOfConsoleInputEvents(hConInput, &numevents);
		if (numevents <= 0) break;

		INPUT_RECORD rec;
		ReadConsoleInput(hConInput, &rec, 1, &numread);
		if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown)
		{
			int ch = rec.Event.KeyEvent.uChar.AsciiChar;
			switch (ch)
			{
				case '\r':		// ENTER
					Win32Log.WriteStr("\r\n");
					console_drawInput = true;
					if (console_textlen)
					{
						console_textlen = 0;
						return console_text;
					}
					break;

				case '\b':		// BACKSPACE
					if (console_textlen)
					{
						console_text[--console_textlen] = 0;
						Win32Log.WriteStr("\b \b");
					}
					break;

				case '\t':		// TAB
					{
						appSprintf(ARRAY_ARG(editLine), "]%s", console_text);
						CompleteCommand();
						const char *s = editLine;
						if (s[0] == ']') s++;
						if (s[0] == '/') s++;
						int len = strlen(s);
						if (len > 0)
						{
							console_textlen = min(len, sizeof(console_text)-2);
							appStrncpyz(console_text, s, console_textlen+1);
							Win32Log.EraseInput();
							console_drawInput = true;	// next time ...
						}
					}
					break;

				case '\x1B':	// ESC
					Win32Log.EraseInput();
					console_textlen = 0;
					console_text[0] = 0;
					break;

				default:
					if (ch >= ' ')
					{
						if (console_textlen < sizeof(console_text)-2)
						{
							Win32Log.WriteChar(ch);
							console_text[console_textlen++] = ch;
							console_text[console_textlen] = 0;
						}
					}
//					else
//						appPrintf("%2X\n",ch);
					break;
			}
		}
	}
#endif
	return NULL;

	unguard;
}


void Sys_ProcessMessages()
{
#if !DEDICATED_ONLY
	guard(Sys_ProcessMessages);
	MSG		msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			GIsRequestingExit = true;

		// TranslateMessage() used for convert VK_XXX messages to WM_CHAR-like
		// We don't need this, but keep TranslateMessage() just in case of compatibility
		// with future Windows versions (at least in Win2K MS implemented "some feature",
		// which requires to call this function -- what feature - they don't tell)
		TranslateMessage(&msg);

		guard(DispatchMessage);
		DispatchMessage(&msg);
		unguardf(("msg=%X", msg.message));
	}

	// grab frame time; this should be in MainLoop ?
	sys_frame_time = appMilliseconds();
	unguard;
#endif
}


/*-----------------------------------------------------------------------------
	Program startup, main loop and exception handling
-----------------------------------------------------------------------------*/

#if !IS_CONSOLE_APP
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, const char **argv) // force to link as console application
#endif
{
	TRY_CRASH
	{
		hInstance = GetModuleHandle(NULL);
		// get command line
		const char *cmdline = GetCommandLine();
		// skip executable name (argv[0])
		//?? NOTE: Linux/Unix have no ability to get full command line - argc/argv[] only!
		cmdline = strchr(cmdline+1, (cmdline[0] == '\"') ? '\"' : ' ');
		if (!cmdline) cmdline = "";
		else cmdline++;

#ifdef CD_PATH
		if (const char *cddir = ScanForCD())
			Cvar_Set("cddir", cddir);
#endif

#if !IS_CONSOLE_APP
		// init-time output
		COutputDeviceMem *TempLog = new COutputDeviceMem(16384);
		TempLog->Register();
#elif !NO_DEDICATED
		Win32Log.Init();
#endif

		appInit();		//!! cmdline should be used here
		Com_Init(cmdline);

#if !IS_CONSOLE_APP && !NO_DEDICATED
		COutputDeviceCon ConLog;
		// create real logger and flush init-time log
		if (DEDICATED)
		{
			Win32Log.Init();
			Win32Log.Write(TempLog->GetText());
		}
		else
		{
			ConLog.Register();
			ConLog.Write(TempLog->GetText());
		}
		// Currently, "logfile" does not contains init-time info: should place its handler into
		// Core (this will allow us to catch init-time info, but requires Core cvar support)
		delete TempLog;
#endif

		int64 oldtime = appCycles64();

		/*--------- main window message loop ------------*/
		guard(MainLoop);
		while (!GIsRequestingExit)
		{
#if MAX_DEBUG
#	if __MINGW32__
			// reinstall exception handler in a case of someone (ATI's OpenGL.dll) replaced it
			// (this is done because GCC have no win32 SEH support)
			long WINAPI mingw32ExceptFilter(struct _EXCEPTION_POINTERS *info);
			SetUnhandledExceptionFilter(mingw32ExceptFilter);
#	endif
			// allow exceptions from FPU
			unsigned fpMask = _controlfp(0, 0);
			_controlfp(fpMask & ~(_EM_ZERODIVIDE|_EM_INVALID), _MCW_EM);
			_clearfp();		// required to prevent re-raising old exceptions on ANY FPU operation
#endif // MAX_DEBUG

			if (DEDICATED)
				Sleep(10);
			else
			{
				if (!ActiveApp)
					Sleep(10);	//?? what about client and server in one place: will server became slower ?
				Sys_ProcessMessages();
			}

			// do not allow Com_Frame(0)
			double timeDelta;
			int64 newtime;
			while (true)
			{
				//?? should process maxfps here (Sleep() required amount of time); problem: ensure server framerate!
				newtime = appCycles64();
				timeDelta = appCyclesToMsecf(newtime - oldtime);
				if (timeDelta < 0 || timeDelta > 10000) // negative number, or greater than 10 sec
				{
					// may be, resumed from sleep mode - fix delta
					//?? may be, implement fix in appMilliseconds() (for global effect) ?
					Com_DPrintf("fixing timeDelta < 0\n");
					timeDelta = 1000;                   // replace delta with 1 sec
					break;
				}
				if (timeDelta > 1.0f) break;	//?? client (or server?) time bugs with ">0" condition & cl_maxfps < realFPS -- net/prediction errors
				Sleep(0);
			}
			TRY {
				Com_Frame(timeDelta);
			} CATCH {
				if (!GErr.nonFatalError) THROW_AGAIN;						// go to outer CATCH{}, outside of MainLoop
				// process DropError()
				// shutdown server
				SV_Shutdown(va("Server crashed: %s\n", *GErr.Message));	// message
				Com_DPrintf("History: %s\n", *GErr.History);				// history
				// drop client
				if (!DEDICATED) CL_Drop(true);
				// and continue execution ...
				GErr.Reset();
			} END_CATCH
			oldtime = newtime;
		}
		unguard;
	} CATCH_CRASH {
#if 0
		extern COutputDevice *debugLog;
		if (debugLog)
			debugLog->Printf("***** CRASH *****\n%s\nHistory: %s\n*****************\n", *GErr.Message, *GErr.History);
#endif
	} END_CATCH

	// shutdown all systems
	TRY {
		SV_Shutdown(GIsFatalError ? va("Server fatal crashed: %s\n", *GErr.Message) : "Server quit\n");
		CL_Shutdown();
#if !IS_CONSOLE_APP
		if (DEDICATED) FreeConsole();
#endif
	} CATCH {
		// nothing ...
	} END_CATCH

	appExit();
	return 0;
}
