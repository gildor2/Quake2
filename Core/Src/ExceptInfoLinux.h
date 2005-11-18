/*=============================================================================

	Internal declarations from linux kernel source

=============================================================================*/


/*-----------------------------------------------------------------------------
	Exception context
-----------------------------------------------------------------------------*/

// this structure sent as 2nd argument to signal handler (when set without
// SA_SIGINFO flag)
// taken from kernel/include/asm-i386/sigcontext.h
struct SIGCONTEXT
{
	unsigned short sc_gs, __gsh;
	unsigned short sc_fs, __fsh;
	unsigned short sc_es, __esh;
	unsigned short sc_ds, __dsh;
	unsigned long sc_edi;
	unsigned long sc_esi;
	unsigned long sc_ebp;
	unsigned long sc_esp;
	unsigned long sc_ebx;
	unsigned long sc_edx;
	unsigned long sc_ecx;
	unsigned long sc_eax;
	unsigned long sc_trapno;		// exception number: T_OFLOW etc
	unsigned long sc_err;
	unsigned long sc_eip;
	unsigned short sc_cs, __csh;
	unsigned long sc_eflags;
	unsigned long esp_at_signal;	// useless ?
	unsigned short sc_ss, __ssh;
	unsigned long i387;				// if != 0 -> pointer to FLOATING_SAVE_AREA (FPU context)
	unsigned long oldmask;			// useless ?
	unsigned long cr2;				// address of fault
};


/*-----------------------------------------------------------------------------
	trap codes (sc_trapno)
-----------------------------------------------------------------------------*/

// really taken from wine/dlls/ntdll/signal_i386.c (possible source: BSD headers?)
// may be gathered from kernel source (note: no symbolic names)
// major place with codes (kernel): kernel/arch/i386/kernel/trap.c
// (seek for DO_XXX_ERROR() -> do_##name(...) defines)

#define T_DIVIDE		0			// Division by zero exception
#define T_TRCTRAP		1			// Single-step exception
#define T_NMI			2			// NMI interrupt
#define T_BPTFLT		3			// Breakpoint exception
#define T_OFLOW			4			// Overflow exception
#define T_BOUND			5			// Bound range exception
#define T_PRIVINFLT		6			// Invalid opcode exception
#define T_DNA			7			// Device not available exception
#define T_DOUBLEFLT		8			// Double fault exception
#define T_FPOPFLT		9			// Coprocessor segment overrun
#define T_TSSFLT		10			// Invalid TSS exception
#define T_SEGNPFLT		11			// Segment not present exception
#define T_STKFLT		12			// Stack fault
#define T_PROTFLT		13			// General protection fault
#define T_PAGEFLT		14			// Page fault
#define T_RESERVED		15			// Unknown exception
#define T_ARITHTRAP		16			// Floating point exception
#define T_ALIGNFLT		17			// Alignment check exception
#define T_MCHK			18			// Machine check exception
#define T_CACHEFLT		19			// Cache flush exception


/*=============================================================================

  Places with useful documentation/sources:
    wine:
		dlls/ntdll/signal_i386.c
		winedocs/en/winedev-kernel.sgml - "Translating signals to exceptions"
	kernel:
		include/asm-i386/sigcontext.h - context structures
		arch/i386/signal.c, sigframe.h: struct sigframe <- struct sigcontext
		arch/i386/kernel/trap.c - trap handlers

  My kernel discovery:
	arch/i386/kernel/sigframe.h:
		struct sigframe - stack frame for calling signal handler w/o SA_SIGINFO flag
		struct rt_sigframe - ... with SA_SIGINFO flag; contains context in nested strucs: "ucontext uc" <- uc_mcontext
	fields in this strucs:
		char *pretcode	- return address
		int sig			- 1st (main) arg of signal handler
		sigcontext sc	- CPU context, struc in stack
		...
		char retcode[]	-

=============================================================================*/
