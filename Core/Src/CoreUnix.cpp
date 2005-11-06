#include "CorePrivate.h"
#include <unistd.h>					// for readlink()
#include <sys/utsname.h>			// for uname() syscall

#include <termios.h>				// ioctl() codes for tty
#include <sys/ioctl.h>				// ioctl() function itself

#include <signal.h>


/*!! REST TODO:
	- CPU detect, speed -> vars
	- debug symbols
	- exception handler
	- strerror() (system error message) -- should test this!
*/

//?? init
double GSecondsPerCycle;			// really, should use GCyclesPerSecond, but this will require divide operation every
									// time we use this, so GSecondsPerCycle=1/GCyclesPerSecond
//?? init
double GMSecondsPerCycle;			// == 1000*GSecondsPerCycle

char GMachineOS[64]  = "Unknown Unix OS";
//?? init
char GMachineCPU[64] = "Unknown 386/486 CPU";


/*-----------------------------------------------------------------------------
	OS detection
-----------------------------------------------------------------------------*/

static void DetectOs ()
{
	struct utsname info;
	if (uname (&info) < 0)
	{
		appPrintf ("OS version detection failed\n");
		return;
	}
	appSprintf (ARRAY_ARG(GMachineOS), "%s-%s-%s", info.sysname, info.release, info.machine);
	appPrintf ("Detected OS: %s\n", GMachineOS);
}


/*-----------------------------------------------------------------------------
	Miscellaneous
-----------------------------------------------------------------------------*/

const char *appPackage ()
{
	static TString<256> Filename;
	if (Filename[0]) return Filename;	// already computed
	readlink ("/proc/self/exe", ARRAY_ARG(Filename));
	char *s = Filename.rchr ('/');
	if (s) strcpy (Filename, s+1);
	return Filename;
}


static void SetDefaultDirectory ()
{
	TString<256> Buf;
	readlink ("/proc/self/exe", ARRAY_ARG(Buf));
	char *s = Buf.rchr ('/');
	if (s) *s = 0;
	chdir (Buf);
}


const char *appGetSystemErrorMessage (unsigned code)
{
	return "<unknown error>"; //?? do; strerror()?
}


void appDisplayError ()
{
	//?? find something like msgbox in X11; use when $DISPLAY is set ...
	//?? may be, write to special log (check UT ?); stderr ?
	//?? NOTE: already logged via appPrintf() !
#if DO_GUARD
//	printf ("%s\n\nHistory: %s", *GErr.Message, *GErr.History);
#else
//	printf ("%s\n", *GErr.Message);
#endif
}


/*-----------------------------------------------------------------------------
	Terminal control
-----------------------------------------------------------------------------*/

// window size changed
static void SigWinchHandler (int signum)
{
	//!! on Unix we will always have console; when have GUI/console - should ignore this signal?!
	//?? Qt: QMessageBox_...
	//?? GTK: gtk_message_dialog_... <- libgtk-x11-2.0.so
	//??		/usr/share/gtk-2.0/demo/dialog.c
	//??		/usr/share/gtk-doc/html/gtk/GtkMessageDialog.html
	struct winsize ws;
	if (ioctl (STDOUT_FILENO, TIOCGWINSZ, &ws) != -1)
		GScreenWidth = ws.ws_col;
}


/*-----------------------------------------------------------------------------
	Initialization/finalization
-----------------------------------------------------------------------------*/

void appInitPlatform ()
{
	// hook signals
	appHookExceptions ();
	// hook SIGWINCH
	struct sigaction sa;
	sigemptyset (&sa.sa_mask);
	sa.sa_handler = SigWinchHandler;
	sa.sa_flags   = 0;
	sigaction (SIGWINCH, &sa, NULL);
	// init console
	SigWinchHandler (SIGWINCH);
	// gather system information
//	CheckCpuModel ();
//	CheckCpuSpeed ();
	DetectOs ();
	// setup current directory
	SetDefaultDirectory ();
}


void appShutdownPlatform ()
{
}
