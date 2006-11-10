#if _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#else
#	include "ExceptInfoLinux.h"
#	include <unistd.h>
#endif

#include "CorePrivate.h"
#include "ExceptDumpers.h"


#define MIN_STRING_WIDTH		3
#define MAX_STRING_WIDTH		32


#if _WIN32
#define F(r1,r2,n)	{ FIELD2OFS(CONTEXT,r1), n }
#else
#define F(r1,r2,n)	{ FIELD2OFS(SIGCONTEXT,sc_##r2), n }
#endif

static const struct {
	unsigned ofs;
	const char *name;
} regData[] = {
	F(Eax, eax, "EAX"), F(Ebx, ebx, "EBX"), F(Ecx, ecx, "ECX"), F(Edx, edx, "EDX"),
	F(Esi, esi, "ESI"), F(Edi, edi, "EDI"),
	F(Esp, esp, "ESP"), F(Ebp, ebp, "EBP"), F(Eip, eip, "EIP")
}, regData2[] = {
	F(SegCs, cs, "CS"), F(SegSs, ss, "SS"), F(SegDs, ds, "DS"),
	F(SegEs, es, "ES"), F(SegFs, fs, "FS"), F(SegGs, gs, "GS")
};

#undef F


#if !_WIN32
// unix have no alternative to win32's IsBadReadPtr()
// this function inspired on Wine's implementation
static bool IsBadReadPtr (const void *data, int count)
{
	// NULL is always bad
	if (data == NULL) return true;
	TRY {
		const byte *b = (byte*) data;
		static byte a = 0;
		// quick check 1st and last bytes
		a += b[0] + b[count-1];
		// get OS page size
		static int pageSize = 0;
		if (!pageSize) pageSize = getpagesize ();
		while (count > 0)
		{
			a += *b;
			// iterate 1 byte per page
			b     += pageSize;
			count -= pageSize;
		}
	} CATCH {
		return true;
	} END_CATCH
	return false;
}
#endif


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


static void DumpReg2 (COutputDevice *Out, const char *name, unsigned value)
{
	Out->Printf ("  %s: %04X", name, value);
}


void DumpRegs (COutputDevice *Out, CONTEXT *ctx)
{
	Out->Write ("Registers:\n");
	int j;
	for (j = 0; j < ARRAY_COUNT(regData); j++)
		DumpReg4 (Out, regData[j].name, OFS2FIELD(ctx, regData[j].ofs, unsigned));
	for (j = 0; j < ARRAY_COUNT(regData2); j++)
		DumpReg2 (Out, regData2[j].name, OFS2FIELD(ctx, regData2[j].ofs, unsigned));
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


void DumpMem (COutputDevice *Out, const unsigned *data, CONTEXT *ctx)
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
void UnwindEbpFrame (COutputDevice *Out, const unsigned *data)
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
				Out->Printf ("(loop)");
				break;
			}
			data = (unsigned*)data[0];
			continue;
		}
		else
		{
			Out->Printf ("(wrong frame)");
			break;
		}
	}
	Out->Printf ("\n\n");
}
#endif
