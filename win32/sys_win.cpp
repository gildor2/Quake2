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

#include <direct.h>
#include <excpt.h>
#include "winquake.h"

#include "../qcommon/qcommon.h"
#include "../client/client.h"


#define CRASH_LOG	"crash.log"

bool		s_win95;

int			starttime;
bool		ActiveApp, Minimized;

static HANDLE hinput, houtput;

unsigned	sys_msg_time;
unsigned	sys_frame_time;


/*
===============================================================================

SYSTEM IO

===============================================================================
*/


void Sys_Quit (void)
{
	timeEndPeriod (1);

	CL_Shutdown (false);
	QCommon_Shutdown ();
#ifndef DEDICATED_ONLY
	if (DEDICATED)
		FreeConsole ();
#endif

	exit (0);
}


//================================================================


/*
================
Sys_ScanForCD

================
*/
static char *Sys_ScanForCD (void)
{
	static char cddir[MAX_OSPATH];
	static bool done;
#ifdef CD_PATH
	static char drive[4] = "c:/";
	FILE		*f;

	if (done)		// don't re-check
		return cddir;

	// no abort/retry/fail errors
	SetErrorMode (SEM_FAILCRITICALERRORS);

	done = true;

	// scan the drives
	for (drive[0] = 'c'; drive[0] <= 'z'; drive[0]++)
	{
		if (GetDriveType (drive) != DRIVE_CDROM) continue;

		Q_CopyFilename (cddir, va("%s"CD_PATH, drive), sizeof(cddir));
		f = fopen (va("%s"CD_CHECK, drive), "r");
		if (f)
		{
			fclose (f);
			return cddir;
		}
	}
#endif

	cddir[0] = 0;
	return cddir;
}

/*
================
Sys_CopyProtect

================
*/
void Sys_CopyProtect (void)
{
#ifdef CD_PATH
	char	*cddir;

	cddir = Sys_ScanForCD ();
	if (!cddir[0])
		Com_FatalError ("You must have the "APPNAME" CD in the drive to play.");
#endif
}


//================================================================

/* NOTE: If error happens in ref_xxx.dll, error message will contain reference to
 * <not available> module (check this ??)
 */

typedef unsigned address_t;


void Sys_Error (const char *error, ...)
{
	va_list		argptr;
	char		text[8192];

	va_start (argptr, error);
	vsnprintf (ARRAY_ARG(text), error, argptr);
	va_end (argptr);

	GErr.isSoftError = true;
	Com_sprintf (ARRAY_ARG(GErr.history), "Error: %s\n\nHistory: ", text);

	throw 1;
}


// from new Macro.h
#define FIELD_OFS(struc, field)		((unsigned) &((struc *)NULL)->field)		// get offset of the field in struc
#define OFS_FIELD(struc, ofs, type)	(*(type*) ((byte*)(struc) + ofs))			// get field of type by offset inside struc

// from DbgSymbols[Win32].cpp (slightly modified)

static char		module[256];
static HMODULE	hModule;

bool osAddressInfo (address_t address, char *pkgName, int bufSize, int *offset)
{
	MEMORY_BASIC_INFORMATION mbi;
	char	*s;

	hModule = NULL;
	if (!VirtualQuery ((void*) address, &mbi, sizeof(mbi)))
		return false;
	if (!(hModule = (HMODULE)mbi.AllocationBase))
		return false;
	if (!GetModuleFileName (hModule, ARRAY_ARG(module)))
		return false;

	if (s = strrchr (module, '.')) *s = 0;
	if (!(s = strrchr (module, '\\')))
		s = module;
	else
		strcpy (module, s+1);	// remove "path\" part

	*offset = address - (unsigned)hModule;
	Q_strncpyz (pkgName, module, bufSize);

	return true;
}

#define OffsetPointer(p, expr)  ((int)(p) + expr)

bool osModuleInfo (address_t address, char *exportName, int bufSize, int *offset)
{
	char	func[256];

	if (!hModule) return false;		// there was no osAddressInfo() call before this

	__try
	{
		WORD	*tmp;
		DWORD	*addrTbl, *nameTbl;
		IMAGE_OPTIONAL_HEADER32 *hdr;
		IMAGE_EXPORT_DIRECTORY* exp;
		int		i;
		unsigned bestRVA = 0;
		int		bestIndex = -1;
		unsigned RVA = address - (unsigned)hModule;

		// we want to walk system memory -- not very safe action
		if (*(WORD*)hModule != 'M'+('Z'<<8)) return false;		// bad MZ header
		tmp = (WORD*) (*(DWORD*) OffsetPointer (hModule, 0x3C) + (char*)hModule);
		if (*tmp != 'P'+('E'<<8)) return false;					// non-PE executable
		hdr = (IMAGE_OPTIONAL_HEADER32*) OffsetPointer (tmp, 4 + sizeof(IMAGE_FILE_HEADER));
		// check export directory
		if (hdr->DataDirectory[0].VirtualAddress == 0 || hdr->DataDirectory[0].Size == 0)
			return false;
		exp = (IMAGE_EXPORT_DIRECTORY*) OffsetPointer (hModule, hdr->DataDirectory[0].VirtualAddress);

		addrTbl = (DWORD*) OffsetPointer (hModule, exp->AddressOfFunctions);
		nameTbl = (DWORD*) OffsetPointer (hModule, exp->AddressOfNames);
		for (i = 0; i < exp->NumberOfFunctions; i++)
		{
			if (addrTbl[i] <= RVA && addrTbl[i] > bestRVA)
			{
				bestRVA = addrTbl[i];
				bestIndex = i;
			}
		}
		if (bestIndex >= 0)
		{
			if (nameTbl[bestIndex])
				Q_strncpyz (func, (char*) OffsetPointer (hModule, nameTbl[bestIndex]), sizeof(func));
			else
				Com_sprintf (ARRAY_ARG(func), "#%d", exp->Base +		// ordinal base
					((WORD*) OffsetPointer (hModule, exp->AddressOfNameOrdinals))[bestIndex]);
			*offset = RVA - bestRVA;
		}
		else
			return false;			// when 0 < RVA < firstFuncRVA (should not happens)
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

	Com_sprintf (exportName, bufSize, "%s!%s", module, func);

	hModule = NULL;					// disallow second subsequentioal call
	return true;
}

bool appSymbolName2 (address_t addr, char *buffer, int size)
{
	char	package[256], *name;
	int		offset;

	if (!osAddressInfo (addr, ARRAY_ARG(package), &offset))
		return false;

	if (osModuleInfo (addr, ARRAY_ARG(package), &offset))
		name = package;

	// package + offset
	name = package;

	if (offset)
		Com_sprintf (buffer, size, "%s+%X", name, offset);
	else
		Q_strncpyz (buffer, name, size);

	return true;
}

char *appSymbolName (address_t addr)
{
	static char	buf[256];

	if (appSymbolName2 (addr, ARRAY_ARG(buf)))
		return buf;
	else
		return va("%08X", addr);
}

// from ExceptFilterWin32.cpp (slightly modified)

#define LOG_STRINGS
#define STACK_UNWIND_DEPTH		80


#define MIN_STRING_WIDTH		3
#define MAX_STRING_WIDTH		32


static void DumpReg4 (FILE *f, char *name, unsigned value)
{
	char	*data;
	int		i;

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


#ifdef LOG_STRINGS

static bool IsString (char *str)
{
	int		i;
	char	c;
	for (i = 0; i < MAX_STRING_WIDTH; i++, str++)
	{
		if (IsBadReadPtr (str, 1)) return false;

		c = *str;
		if (c == 0) return i >= MIN_STRING_WIDTH;
		if ((c < 32 || c > 127) && c != '\n') return false;
	}
	return true;
}


static bool DumpString (FILE *f, char *str)
{
	int		i;

	fprintf (f, " = \"");
	for (i = 0; i < MAX_STRING_WIDTH && *str; i++, str++)
	{
		if (*str == '\n')
			fprintf (f, "\\n");
		else
			fprintf (f, "%c", *str);
	}
	fprintf (f, "%s\"", *str ? "..." : "");
	return true;
}

#endif


static void DumpMem (FILE *f, unsigned *data, CONTEXT *ctx)
{
	static struct {
		unsigned ofs;
		const char *name;
	} regData[] = {
#define F(r,n)	{ FIELD_OFS(CONTEXT,r), n }
		F(Eip, "EIP"), F(Esp, "ESP"), F(Ebp, "EBP"),
		F(Eax, "EAX"), F(Ebx, "EBX"), F(Ecx, "ECX"), F(Edx, "EDX"),
		F(Esi, "ESI"), F(Edi, "EDI")
#undef F
	};
#define STAT_SPACES		"     "
	int		i, j, n;

	//!! should try to use address as a symbol
	n = 0;
	for (i = 0; i < STACK_UNWIND_DEPTH; i++, data++)
	{
		char	symbol[256], *spaces;

		if (IsBadReadPtr (data, 4))
		{
			fprintf (f, "  <N/A>\n");
			return;
		}

		spaces = STAT_SPACES;
		for (j = 0; j < ARRAY_COUNT(regData); j++)
			if (OFS_FIELD(ctx, regData[j].ofs, unsigned*) == data)
			{
				if (n)
				{
					fprintf (f, "\n");
					n = 0;
				}
				fprintf (f, "%s->", regData[j].name);
				spaces = "";
				break;
			}

		if (appSymbolName2 (*data, ARRAY_ARG(symbol)))
#ifdef LOG_FUNCS_ONLY
			if (strchr (symbol, '('))
#endif
				{
					// log as symbol
					fprintf (f, "%s%s%08X = %s",
						n > 0 ? "\n" : "", spaces,
						*data, symbol);
#if !defined(LOG_FUNCS_ONLY) && defined(LOG_STRINGS)
					if (!strchr (symbol, '(') && IsString ((char*)*data))	// do not test funcs()
						DumpString (f, (char*)*data);
#endif
					fprintf (f, "\n");
					n = 0;
					continue;
				}

#ifdef LOG_STRINGS
		// try to log as string
		if (IsString ((char*)*data))
		{
			fprintf (f, "%s%08X", n > 0 ? "\n" STAT_SPACES : spaces, *data);
			DumpString (f, (char*)*data);
			fprintf (f, "\n");
			n = 0;
			continue;
		}
#endif

		// log as simple number
		fprintf (f, "%s%08X", n > 0 ? "  " : spaces, *data);
		if (++n >= 8)
		{
			fprintf (f, "\n");
			n = 0;
		}
	}
	fprintf (f, "\n");
	return;
#undef STAT_SPACES
}


int win32ExceptFilter (struct _EXCEPTION_POINTERS *info)
{
	FILE	*f;

	if (GErr.isSoftError) return EXCEPTION_EXECUTE_HANDLER;		// no interest to thread context when software-generated errors

	if (GErr.contextDumped) return EXCEPTION_EXECUTE_HANDLER;	// context will be dumped only once
	GErr.contextDumped = true;
	GErr.isFatalError = true;

	__try
	{
		char	*excName;

		switch (info->ExceptionRecord->ExceptionCode)
		{
		case EXCEPTION_ACCESS_VIOLATION:
			excName = "Access violation";
			break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			excName = "Float zero divide";
			break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			excName = "Integer zero divide";
			break;
		case EXCEPTION_PRIV_INSTRUCTION:
			excName = "Priveleged instruction";
			break;
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			excName = "Illegal opcode";
			break;
		case EXCEPTION_STACK_OVERFLOW:
			excName = "Stack overflow";
			break;
		default:
			excName = "Exception";
		}

		//?? should make logging a global class (implements opening/logging date/closing)
		// make a log in "crash.log"
		if (f = fopen (CRASH_LOG, "a+"))
		{
			CONTEXT* ctx = info->ContextRecord;
//			EXCEPTION_RECORD* rec = info->ExceptionRecord;
			time_t	itime;
			char	ctime[256];

			Com_sprintf (ARRAY_ARG(GErr.history), "%s at \"%s\"\n", excName, appSymbolName (ctx->Eip));

			time (&itime);
			strftime (ARRAY_ARG(ctime), "%a %b %d, %Y (%H:%M:%S)", localtime (&itime));
			fprintf (f, "----- "APPNAME" crash at %s -----\n", ctime);		//!! should use main_package name instead of APPNAME
			fprintf (f, "%s\n", GErr.history);
			strcat (GErr.history, "\nHistory: ");

			DumpReg4 (f, "EAX", ctx->Eax); DumpReg4 (f, "EBX", ctx->Ebx); DumpReg4 (f, "ECX", ctx->Ecx); DumpReg4 (f, "EDX", ctx->Edx);
			DumpReg4 (f, "ESI", ctx->Esi); DumpReg4 (f, "EDI", ctx->Edi); DumpReg4 (f, "EBP", ctx->Ebp); DumpReg4 (f, "ESP", ctx->Esp);
			DumpReg4 (f, "EIP", ctx->Eip);
			DumpReg2 (f, "CS", ctx->SegCs); DumpReg2 (f, "SS", ctx->SegSs);
			DumpReg2 (f, "DS", ctx->SegDs); DumpReg2 (f, "ES", ctx->SegEs);
			DumpReg2 (f, "FS", ctx->SegFs); DumpReg2 (f, "GS", ctx->SegGs);
			fprintf (f, "  EFLAGS: %08X\n", ctx->EFlags);

			fprintf (f, "\nStack frame:\n");
			DumpMem (f, (unsigned*) ctx->Esp, ctx);
			fprintf (f, "\n");
			fclose (f);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		// do nothing
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

__declspec(naked) int win32ExceptFilter2 (void)
{
	__asm {
		push	[ebp-0x14]
		call	win32ExceptFilter
		add		esp,4
		retn			// return value from win32ExceptFilter()
	}
}


/*
================
CPUID
================
*/

static bool IsMMX, IsSSE, IsRDTSC, Is3DNow;

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
		Com_Printf ("Unknown 386/486 CPU\n");
		return;
	}
	if (r[0] >= 0x80000004)		// extended vendor string available
	{
		unsigned fn;
		int		i = 0;
		char	*s;
		for (fn = 0x80000002; fn <= 0x80000004; fn++)
		{
			cpuid (fn, r);
			t.reg[i++] = r[0];
			t.reg[i++] = r[1];
			t.reg[i++] = r[2];
			t.reg[i++] = r[3];
		}
		t.name[i*4] = 0;
		s = t.name;
		while (*s == ' ') s++;
		Com_Printf ("CPU name: %s\n", s);
	}
	else
	{
		cpuid (0, r);
		t.reg[0] = r[1];
		t.reg[1] = r[3];
		t.reg[2] = r[2];
		t.name[12] = 0;		// ASCIIZ
		Com_Printf ("CPU name: %s\n", t.name);
	}
}


static void CheckCpuCaps (void)
{
	unsigned r[4];

	if (!cpuid (1, r)) return;

	Com_Printf ("CPU caps: [ ");

	if (r[3] & 0x00000001)	Com_Printf ("FPU ");
	if (r[3] & 0x00800000)
	{
		Com_Printf ("MMX ");
		IsMMX = true;
	}
	if (r[3] & 0x02000000)
	{
		Com_Printf ("SSE ");
		IsSSE = true;
	}
	if (r[3] & 0x04000000)	Com_Printf ("SSE2 ");
	if (r[3] & 0x00000010)
	{
		Com_Printf ("RDTSC ");
		IsRDTSC = true;
	}

	// check extended features
	cpuid (0x80000000, r);
	if (r[0] >= 0x80000001)		// largest recognized extended function
	{
		cpuid (0x80000001, r);
		if (r[3] & 0x80000000)
		{
			Com_Printf ("3DNow! ");
			Is3DNow = true;
		}
	}

	Com_Printf ("]\n");
}


#pragma warning (push)
#pragma warning (disable : 4035)
#pragma warning (disable : 4715)
inline __int64 cycles ()
{
	__asm
	{
		rdtsc
	}
}
#pragma warning (pop)


static void CheckCpuSpeed (void)
{
	int		tries;
	unsigned time1, time2;
	__int64	stamp1, stamp2;
	double secPerCycle, secPerCycle1;

	if (!IsRDTSC) return;

	timeBeginPeriod (1);

	secPerCycle = 1.0;							// initial value
	for (tries = 0; tries < 4; tries++)
	{
		stamp1 = cycles ();
		time1 = timeGetTime ();
		while (true)
		{
			time2 = timeGetTime ();
			if (time2 - time1 > 200) break;		// 200ms enough to compute CPU speed
		}
		stamp2 = cycles ();
		secPerCycle1 = (time2 - time1) / (((double)stamp2 - (double)stamp1) * 1000);
		if (secPerCycle1 < secPerCycle) secPerCycle = secPerCycle1;

//		Com_Printf ("try #%d\n", tries);
//		Com_Printf ("stampd: %u %u   timed: %d\n", stamp2 - stamp1, time2 - time1);
//		Com_Printf ("CPU speed: %g MHz\n", 1e-6 / secPerCycle1);
	}
	Com_Printf ("CPU speed: %g MHz\n", 1e-6 / secPerCycle);

	timeEndPeriod (1);
}


/*
================
Sys_Init
================
*/
void Sys_Init (void)
{
	OSVERSIONINFO	vinfo;

	guard(Sys_Init);

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	GetVersionEx (&vinfo);

	if (vinfo.dwMajorVersion < 4)
		Sys_Error ("Requires Windows version 4 or greater");
	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		s_win95 = true;

	if (DEDICATED)
	{
#ifndef DEDICATED_ONLY
		AllocConsole ();
#endif
		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);
	}

	CheckCpuName ();
	CheckCpuCaps ();
	CheckCpuSpeed ();

	//??
	timeBeginPeriod (1);

	unguard;
}


#define MAX_CMDLINE		256		//?? same as in client/keys.h

static char	console_text[MAX_CMDLINE];
static int	console_textlen = 0;
static bool console_drawInput = true;

static void EraseConInput (void)
{
	if (!console_drawInput)
	{
		char	text[MAX_CMDLINE+2];
		DWORD	dummy;

		text[0] = '\r';
		memset (&text[1], ' ', console_textlen+1);
		text[console_textlen+2] = '\r';
		text[console_textlen+3] = 0;
		WriteFile (houtput, text, console_textlen+3, &dummy, NULL);
	}
}

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
	INPUT_RECORD rec;
	int		ch, len;
	DWORD	numevents, dummy, numread;
	char	*s;

	guard(Sys_ConsoleInput);

	if (console_drawInput)
	{
		// display input line
		WriteFile (houtput, "]", 1, &dummy, NULL);
		if (console_textlen)
			WriteFile (houtput, console_text, console_textlen, &dummy, NULL);
		console_drawInput = false;
	}

	while (true)
	{
		GetNumberOfConsoleInputEvents (hinput, &numevents);
		if (numevents <= 0) break;
		ReadConsoleInput (hinput, &rec, 1, &numread);

		if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown)
		{
			ch = rec.Event.KeyEvent.uChar.AsciiChar;
			switch (ch)
			{
				case '\r':		// ENTER
					WriteFile (houtput, "\r\n", 2, &dummy, NULL);
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
						WriteFile (houtput, "\b \b", 3, &dummy, NULL);
					}
					break;

				case '\t':		// TAB
					Com_sprintf (ARRAY_ARG(editLine), "]%s", console_text);
					CompleteCommand ();
					s = editLine;
					if (s[0] == ']') s++;
					if (s[0] == '/') s++;
					len = strlen (s);
					if (len > 0)
					{
						console_textlen = min(len, sizeof(console_text)-2);
						Q_strncpyz (console_text, s, console_textlen+1);
						Sys_ConsoleOutput ("");		// update line
					}
					break;

				case '\x1B':	// ESC
					EraseConInput ();
					console_textlen = 0;
					console_text[0] = 0;
					break;

				default:
					if (ch >= ' ')
					{
						if (console_textlen < sizeof(console_text)-2)
						{
							WriteFile(houtput, &ch, 1, &dummy, NULL);
							console_text[console_textlen++] = ch;
							console_text[console_textlen] = 0;
						}
					}
//					else
//						Com_Printf ("%2X\n",ch);
					break;
			}
		}
	}

	return NULL;

	unguard;
}


/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput (char *string)
{
	DWORD	dummy;
	char	c;

	if (!DEDICATED) return;

	EraseConInput ();

	// draw message
	while (c = string[0])
	{
		// parse color info
		if (c == COLOR_ESCAPE && string[1] >= '0' && string[1] <= '7')
		{
			static const byte colorTable[8] = {0, 4, 2, 6, 1, 5, 3, 7};
			SetConsoleTextAttribute (houtput, colorTable[string[1] - '0']);
			string += 2;
			continue;
		}
		c &= 0x7F;		// clear 7th bit
		WriteFile (houtput, &c, 1, &dummy, NULL);
		string++;
	}
	SetConsoleTextAttribute (houtput, 7);

	console_drawInput = true;
}


/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents (void)
{
	MSG		msg;

	guard(Sys_SendKeyEvents);
	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			Com_Quit ();
		sys_msg_time = msg.time;

		guard(TranslateMessage);
		TranslateMessage (&msg);
		unguardf(("msg=%X", msg.message));

		guard(DispatchMessage);
		DispatchMessage (&msg);
		unguardf(("msg=%X", msg.message));
	}

	// grab frame time
	sys_frame_time = timeGetTime();	// FIXME: should this be at start?
	unguard;
}



/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData (void)
{
	char *data = NULL;
	char *cliptext;

	if (OpenClipboard (NULL))
	{
		HANDLE	hClipboardData;

		if (hClipboardData = GetClipboardData (CF_TEXT))
		{
			if (cliptext = (char*)GlobalLock (hClipboardData))
			{
				data = (char*)malloc (GlobalSize (hClipboardData) + 1);
				strcpy (data, cliptext);
				GlobalUnlock (hClipboardData);
			}
		}
		CloseClipboard ();
	}
	return data;
}

/*
========================================================================

GAME DLL

========================================================================
*/

static HINSTANCE game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	guard(Sys_UnloadGame);
	if (!FreeLibrary (game_library))
		Com_FatalError ("Cannot unload game library");
	Com_DPrintf ("FreeLibrary(game.dll)\n");
	game_library = NULL;
	unguard;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI (void *parms)
{
	typedef void * (* pGetGameApi_t)(void *);
	pGetGameApi_t pGetGameAPI;
	char	name[MAX_OSPATH], *path;
#if defined _M_IX86
	const char *gamename = "gamex86.dll";
#elif defined _M_ALPHA
	const char *gamename = "gameaxp.dll";
#endif

	guard(Sys_GetGameAPI);

	if (game_library)
		Com_FatalError ("Sys_GetGameAPI() without Sys_UnloadGame()");

	// run through the search paths
	path = NULL;
	while (path = FS_NextPath (path))
	{
		Com_sprintf (ARRAY_ARG(name), "%s/%s", path, gamename);
		if (game_library = LoadLibrary (name))
		{
			Com_DPrintf ("LoadLibrary (%s)\n",name);
			break;
		}
	}
	if (!game_library)
		return NULL;		// couldn't find one anywhere

	pGetGameAPI = (pGetGameApi_t)GetProcAddress (game_library, "GetGameAPI");
	if (!pGetGameAPI)
	{
		Sys_UnloadGame ();
		return NULL;
	}

	guard(GetGameAPI);
	return pGetGameAPI (parms);
	unguard;

	unguard;
}

//=======================================================================

extern bool debugLogged;

/*
==================
WinMain

==================
*/
HINSTANCE	global_hInstance;

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int		time, oldtime, newtime;
	char	*cmdline;
#ifdef CD_PATH
	char	*cddir, cmdline2[1024];
#endif

	GUARD_BEGIN
	{
		global_hInstance = hInstance;

		cmdline = lpCmdLine;
#ifdef CD_PATH
		// if we find the CD, add a "-cddir=xxx" command line
		cddir = Sys_ScanForCD ();
		if (cddir && cddir[0])
		{
			// add to the end of cmdline, so, if already specified - will not override option
			Com_sprintf (ARRAY_ARG(cmdline2), "%s -cddir=\"%s\"", lpCmdLine, cddir);
			cmdline = cmdline2;
		}
#endif

		QCommon_Init (cmdline);
		oldtime = Sys_Milliseconds ();

		/*--------- main window message loop ------------*/
		guard(MainLoop);
		while (true)
		{
#ifndef DEDICATED_ONLY
			MSG		msg;

			if (!ActiveApp || DEDICATED)
				Sleep (10);		//?? what about client and server in one place: will server become slower ?

			while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
					Com_Quit ();
				sys_msg_time = msg.time;

				guard(TranslateMessage);
				TranslateMessage (&msg);
				unguardf(("msg=%X", msg.message));

				guard(DispatchMessage);
				DispatchMessage (&msg);
				unguardf(("msg=%X", msg.message));
			}
#else
			Sleep (10);
#endif

			// do not allow Qcommon_Frame(0)
			while (true)
			{
				newtime = Sys_Milliseconds ();
				time = newtime - oldtime;
				if (time >= 1) break;
				Sleep (1);
			}
			GUARD_BEGIN
			{
				QCommon_Frame (time);
			}
			GUARD_CATCH
			{
				if (GErr.isFatalError)
					throw;
				else
				{
					Com_DPrintf ("History: %s\n", GErr.history);
					Com_ResetErrorState ();
				}
			}
			oldtime = newtime;
		}
		unguard;
	}
	GUARD_CATCH
	{
		FILE *f;
		// log error
		if (f = fopen (CRASH_LOG, "a+"))
		{
			if (!GErr.contextDumped)
				fprintf (f, "----- " APPNAME " software exception -----\n%s\n\n", GErr.history);
			else
				fprintf (f, "%s\n\n", strrchr (GErr.history, '\n') + 1);
			fclose (f);
		}
		if (debugLogged)
			DebugPrintf ("***** CRASH *****\n%s\n*****************\n", GErr.history);

		// shutdown all subsystems
		//?? SV_Shutdown()
		CL_Shutdown (true);
		QCommon_Shutdown ();

		// display error
#ifndef DEDICATED_ONLY
		MessageBox (NULL, GErr.history, APPNAME ": fatal error", MB_OK|MB_ICONSTOP/*|MB_TOPMOST*/|MB_SETFOREGROUND);
#else
		Sys_ConsoleOutput ("\n\n"S_RED"--------------------\n" APPNAME " fatal error\n");
		Sys_ConsoleOutput (GErr.history);
#endif
	}
	// never gets here
	return TRUE;
}
