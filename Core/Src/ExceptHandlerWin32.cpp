#define WIN32_LEAN_AND_MEAN		// exclude rarely-used services from windown headers
#include <windows.h>
#include <float.h>				// for _clearfp()
#include "CorePrivate.h"
#include "ExceptDumpers.h"


long WINAPI win32ExceptFilter(struct _EXCEPTION_POINTERS *info)
{
	if (GErr.swError) return EXCEPTION_EXECUTE_HANDLER;		// no interest to thread context when software-generated errors

	static bool dumped = false;
	if (dumped) return EXCEPTION_EXECUTE_HANDLER;			// error will be handled only once
	// NOTE: side effect of line above: we will not able to catch GPF-like recursive errors

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
			_clearfp();
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
		TString<1024> Message;
		Message.sprintf("%s in \"%s\"", excName, appSymbolName(ctx->Eip));
		// initiate error + start log
		COutputDevice *Out = appBeginError(Message);

		Out->Printf("Exception code: %08X\n\n", info->ExceptionRecord->ExceptionCode);
		DumpRegs(Out, ctx);
		Out->Printf("  EFLAGS: %08X\n", ctx->EFlags);
		Out->Printf("\nStack:\n");
		DumpMem(Out, (unsigned*) ctx->Esp, ctx);
		Out->Printf("\n");
#if UNWIND_EBP_FRAMES
		Out->Printf("\nCall stack trace:\n");
		UnwindEbpFrame(Out, (unsigned*) ctx->Ebp);
#endif
	} CATCH {
		// do nothing
	} END_CATCH

	return EXCEPTION_EXECUTE_HANDLER;
}


#if _MSC_VER
__declspec(naked) unsigned win32ExceptFilter2()
{
	__asm {
		push	[ebp-0x14]
		call	win32ExceptFilter
		retn			// return value from win32ExceptFilter()
	}
}
#endif
