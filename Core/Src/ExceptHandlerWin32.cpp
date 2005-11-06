#define WIN32_LEAN_AND_MEAN		// exclude rarely-used services from windown headers
#include <windows.h>
#include <float.h>				// for _clearfp()
#include "CorePrivate.h"


//#define LOG_FUNCS_ONLY		1
#define LOG_STRINGS				1
#define UNWIND_EBP_FRAMES		1

#define STACK_UNWIND_DEPTH		80


#define MIN_STRING_WIDTH		3
#define MAX_STRING_WIDTH		32


//?? IsBadReadPtr -> platform-specific; DumpXxxx() -> platform-independent file;
//?? DumpMem(): CONTEXT-stuff -> inline function

static void DumpReg4 (COutputDevice *Out, const char *name, unsigned value)
{
	const char *data = (char*) value;
	Out->Printf ("  %s: %08X  ", name, value);
	if (IsBadReadPtr (data, 16))
		Out->Printf (" <N/A>");
	else
	{
		int		i;
		for (i = 0; i < 16; i++)
			Out->Printf (" %02X", data[i] & 0xFF);

		Out->Printf ("  ");

		for (i = 0; i < 16; i++)
		{
			char c = data[i];
			if (c < ' ') c = '.';		// signed char, allowed 32..127
			Out->Printf ("%c", c);
		}
	}
	Out->Printf ("\n");
}


static void DumpReg2 (COutputDevice *Out, const char *name, DWORD value)
{
	Out->Printf ("  %s: %04X", name, value);
}


#if LOG_STRINGS

static bool IsString (const char *str)
{
	for (int i = 0; i < MAX_STRING_WIDTH; i++, str++)
	{
		if (IsBadReadPtr (str, 1)) return false;

		char c = *str;
		if (c == 0) return i >= MIN_STRING_WIDTH;
		if (c < ' ' && c != '\n') return false;		// signed char, allowed 32..127 + '\n'
	}
	return true;
}


static bool DumpString (COutputDevice *Out, const char *str)
{
	Out->Printf (" = \"");
	//?? can use appQuoteString()
	for (int i = 0; i < MAX_STRING_WIDTH && *str; i++, str++)
	{
		if (*str == '\n')
			Out->Printf ("\\n");
		else
			Out->Printf ("%c", *str);
	}
	Out->Printf ("%s\"", *str ? "..." : "");
	return true;
}

#endif


#define F(r,n)	{ FIELD2OFS(CONTEXT,r), n }
static const struct {
	unsigned ofs;
	const char *name;
} regData[] = {
	F(Eax, "EAX"), F(Ebx, "EBX"), F(Ecx, "ECX"), F(Edx, "EDX"),
	F(Esi, "ESI"), F(Edi, "EDI"),
	F(Esp, "ESP"), F(Ebp, "EBP"), F(Eip, "EIP")
}, regData2[] = {
	F(SegCs, "CS"), F(SegSs, "SS"), F(SegDs, "DS"),
	F(SegEs, "ES"), F(SegFs, "FS"), F(SegGs, "GS")
};
#undef F

static void DumpMem (COutputDevice *Out, const unsigned *data, CONTEXT *ctx)
{
	//!! should try to use address as a symbol
#define STAT_SPACES		"     "
#define WRAP			{ Out->Printf("\n"); n = 0; }
	int n = 0;
	for (int i = 0; i < STACK_UNWIND_DEPTH; i++, data++)
	{
		if (IsBadReadPtr (data, 4))
		{
			Out->Printf ("  <N/A>\n");
			return;
		}

		const char *spaces = STAT_SPACES;
		for (int j = 0; j < ARRAY_COUNT(regData); j++)
			if (OFS2FIELD(ctx, regData[j].ofs, unsigned*) == data)
			{
				if (n) WRAP;
				Out->Printf ("%s->", regData[j].name);
				spaces = "";
				break;
			}

		char symbol[256];
		if (appSymbolName (*data, ARRAY_ARG(symbol)))
#if LOG_FUNCS_ONLY
			if (strchr (symbol, '('))
#endif
			{
				// log as symbol
				Out->Printf ("%s%s%08X = %s",
					n > 0 ? "\n" : "", spaces,
					*data, symbol);
#if !LOG_FUNCS_ONLY && LOG_STRINGS
				if (!strchr (symbol, '(') && IsString ((char*)*data))	// do not test funcs()
					DumpString (Out, (char*)*data);
#endif
				WRAP;
				continue;
			}

#if LOG_STRINGS
		// try to log as string
		if (IsString ((char*)*data))
		{
			Out->Printf ("%s%08X", n > 0 ? "\n" STAT_SPACES : spaces, *data);
			DumpString (Out, (char*)*data);
			WRAP;
			continue;
		}
#endif

		// log as simple number
		Out->Printf ("%s%08X", n > 0 ? "  " : spaces, *data);
		if (++n >= 8) WRAP;
	}
	Out->Printf ("\n");
	return;
#undef STAT_SPACES
#undef WRAP
}

#if UNWIND_EBP_FRAMES
static void UnwindEbpFrame (COutputDevice *Out, const unsigned *data)
{
	Out->Printf ("  ");
	while (true)
	{
		if (IsBadReadPtr (data, 8))
		{
			Out->Printf ("<N/A>");
			break;
		}
		char symbol[256];
		if (appSymbolName (data[1], ARRAY_ARG(symbol)))
		{
			Out->Printf ("%s <- ", symbol);
			if (data[0] <= (unsigned)data)	// possible loop
			{
				Out->Printf ("(loop?)");
				break;
			}
			data = (unsigned*)data[0];
			continue;
		}
		else
		{
			Out->Printf ("(wrong EBP frame)");
			break;
		}
	}
	Out->Printf ("\n\n");
}
#endif


long WINAPI win32ExceptFilter (struct _EXCEPTION_POINTERS *info)
{
	if (GErr.swError) return EXCEPTION_EXECUTE_HANDLER;		// no interest to thread context when software-generated errors

	static bool dumped = false;
	if (dumped) return EXCEPTION_EXECUTE_HANDLER;			// error will be handled only once
	// WARNING: recursive error will not be found
	// If we will disable line above, will be dumped context for each appUnwind() entry
	dumped = true;

	TRY {
		const char *excName = "Exception";
		switch (info->ExceptionRecord->ExceptionCode)
		{
		case EXCEPTION_ACCESS_VIOLATION:
			excName = "Access violation";
			break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		case EXCEPTION_FLT_INEXACT_RESULT:
		case EXCEPTION_FLT_INVALID_OPERATION:
		case EXCEPTION_FLT_OVERFLOW:
		case EXCEPTION_FLT_STACK_CHECK:
		case EXCEPTION_FLT_UNDERFLOW:
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			// if FPU exception occured, _clearfp() is required (otherwise, exception will be re-raised again)
			_clearfp ();
			excName = "FPU exception";
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
		}

		// log error
		CONTEXT* ctx = info->ContextRecord;
		GErr.Message.sprintf ("%s in \"%s\"", excName, appSymbolName (ctx->Eip));
		COutputDevice *Out = appGetErrorLog ();

		Out->Printf ("Exception code: %08X\n\n", info->ExceptionRecord->ExceptionCode);
		Out->Write ("Registers:\n");
		int j;
		for (j = 0; j < ARRAY_COUNT(regData); j++)
			DumpReg4 (Out, regData[j].name, OFS2FIELD(ctx, regData[j].ofs, unsigned));
		for (j = 0; j < ARRAY_COUNT(regData2); j++)
			DumpReg2 (Out, regData2[j].name, OFS2FIELD(ctx, regData2[j].ofs, unsigned));
		Out->Printf ("  EFLAGS: %08X\n", ctx->EFlags);

		Out->Printf ("\nStack:\n");
		DumpMem (Out, (unsigned*) ctx->Esp, ctx);
		Out->Printf ("\n");
#if UNWIND_EBP_FRAMES
		Out->Printf ("\nCall stack trace:\n");
		UnwindEbpFrame (Out, (unsigned*) ctx->Ebp);
#endif
	} CATCH {
		// do nothing
	} END_CATCH

	return EXCEPTION_EXECUTE_HANDLER;
}


#if _MSC_VER
__declspec(naked) unsigned win32ExceptFilter2 ()
{
	__asm {
		push	[ebp-0x14]
		call	win32ExceptFilter
		retn			// return value from win32ExceptFilter()
	}
}
#endif
