#define WIN32_LEAN_AND_MEAN			// exclude rarely-used services from windown headers
#include <windows.h>

#include "Core.h"
#include <time.h>			//?? for logging time funcs


#define CRASH_LOG	"crash.log"


//#define LOG_FUNCS_ONLY
#define LOG_STRINGS
#define STACK_UNWIND_DEPTH		80


#define MIN_STRING_WIDTH		3
#define MAX_STRING_WIDTH		32


//?? IsBadReadPtr -> platform-specific; DumpXxxx() -> platform-independent file;
//?? DumpMem(): CONTEXT-staff -> inline function

//?? should change FILE to local file system (not required) (may be, avoid local FS usage when crashed ...)
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
	for (int i = 0; i < MAX_STRING_WIDTH; i++, str++)
	{
		if (IsBadReadPtr (str, 1)) return false;

		char c = *str;
		if (c == 0) return i >= MIN_STRING_WIDTH;
		if ((c < 32 || c > 127) && c != '\n') return false;
	}
	return true;
}


static bool DumpString (FILE *f, char *str)
{
	fprintf (f, " = \"");
	for (int i = 0; i < MAX_STRING_WIDTH && *str; i++, str++)
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
#define F(r,n)	{ FIELD2OFS(CONTEXT,r), n }
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
			if (OFS2FIELD(ctx, regData[j].ofs, unsigned*) == data)
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

		if (appSymbolName (*data, ARRAY_ARG(symbol)))
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
	static bool dumped = false;

	if (GErr.swError) return EXCEPTION_EXECUTE_HANDLER;		// no interest to thread context when software-generated errors
	if (dumped) return EXCEPTION_EXECUTE_HANDLER;			// error will be handled only once
	dumped = true;

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
		if (FILE *f = fopen (CRASH_LOG, "a+"))
		{
			CONTEXT* ctx = info->ContextRecord;
//			EXCEPTION_RECORD* rec = info->ExceptionRecord;
			time_t	itime;
			char	ctime[256];

			appSprintf (ARRAY_ARG(GErr.message), "%s at \"%s\"", excName, appSymbolName (ctx->Eip));	//?? may be, supply package name

			time (&itime);
			strftime (ARRAY_ARG(ctime), "%a %b %d, %Y (%H:%M:%S)", localtime (&itime));
			fprintf (f, "----- "APPNAME" crash at %s -----\n", ctime);		//!! should use main_package name instead of APPNAME
			fprintf (f, "%s\n\n", GErr.message);

			DumpReg4 (f, "EAX", ctx->Eax); DumpReg4 (f, "EBX", ctx->Ebx); DumpReg4 (f, "ECX", ctx->Ecx); DumpReg4 (f, "EDX", ctx->Edx);
			DumpReg4 (f, "ESI", ctx->Esi); DumpReg4 (f, "EDI", ctx->Edi); DumpReg4 (f, "EBP", ctx->Ebp); DumpReg4 (f, "ESP", ctx->Esp);
			DumpReg4 (f, "EIP", ctx->Eip);
			DumpReg2 (f, "CS", ctx->SegCs); DumpReg2 (f, "SS", ctx->SegSs);
			DumpReg2 (f, "DS", ctx->SegDs); DumpReg2 (f, "ES", ctx->SegEs);
			DumpReg2 (f, "FS", ctx->SegFs); DumpReg2 (f, "GS", ctx->SegGs);
			fprintf (f, "  EFLAGS: %08X\n", ctx->EFlags);

			fprintf (f, "\nStack frame:\n");
			DumpMem (f, (unsigned*) ctx->Esp, ctx);
			//!! should log call history too !! (here?)
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
