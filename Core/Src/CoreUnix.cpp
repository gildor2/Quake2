#include "CorePrivate.h"
#include <unistd.h>					// syscalls
#include <sys/utsname.h>			// for uname() syscall
#include <time.h>					// nanosleep()
#include <sys/time.h>				// struct timeval, gettimeofday()
#include <termios.h>				// ioctl() codes for tty
#include <sys/ioctl.h>				// ioctl() function itself
#include <signal.h>


char GMachineOS[64] = "Unknown Unix OS";


/*-----------------------------------------------------------------------------
	Timing
-----------------------------------------------------------------------------*/

unsigned appMilliseconds()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	static unsigned secBase = 0;
	if (!secBase) secBase = t.tv_sec;
	return (t.tv_sec - secBase) * 1000 + t.tv_usec / 1000;
}


void appSleep(unsigned msec)
{
#if 0
	struct timespec t;
	t.tv_sec  = msec / 1000;
	t.tv_nsec = msec % 1000 * 1000000;
	nanosleep(&t, NULL);
#else
	// in linux, nanosleep() and usleep() are less precise, than select()-based sleep
	struct timeval t;
	t.tv_sec  = msec / 1000;
	t.tv_usec = msec % 1000 * 1000;
	select(0, NULL, NULL, NULL, &t);
#endif
}


/*-----------------------------------------------------------------------------
	OS detection
-----------------------------------------------------------------------------*/

static void DetectOs()
{
	struct utsname info;
	if (uname(&info) < 0)
	{
		appPrintf("OS version detection failed\n");
		return;
	}
	appSprintf(ARRAY_ARG(GMachineOS), "%s-%s-%s", info.sysname, info.release, info.machine);
	appPrintf("Detected OS: %s\n", GMachineOS);
}


/*-----------------------------------------------------------------------------
	Miscellaneous
-----------------------------------------------------------------------------*/

const char *appPackage()
{
	static TString<256> Filename;
	if (Filename[0]) return Filename;	// already computed
	if (readlink("/proc/self/exe", ARRAY_ARG(Filename)) == -1)
		return "unknown";
	char *s = Filename.rchr('/');
	if (s) strcpy(Filename, s+1);
	return Filename;
}


static void SetDefaultDirectory()
{
	TString<256> Buf;
	if (readlink("/proc/self/exe", ARRAY_ARG(Buf)) == -1)
		return;
	char *s = Buf.rchr('/');
	if (s) *s = 0;
	chdir(Buf);
}


const char *appGetSystemErrorMessage(unsigned code)
{
	const char *msg = strerror(code);
	if (!msg || !msg[0]) return "unknown error code";
	return msg;
}


void appDisplayError()
{
	//?? when $DISPLAY is set:
	//?? GTK: gtk_message_dialog_... <- libgtk-x11-2.0.so
	//??		/usr/share/gtk-2.0/demo/dialog.c
	//??		/usr/share/gtk-doc/html/gtk/GtkMessageDialog.html
	//?? Qt: <qmessagebox.h>: QMessageBox::information() etc
	//?? Can use dlsym(RTLD_DEFAULT,"funcName") -- will return !NULL when linked with such library
	// NOTE: already logged to console via appPrintf() !
#if DO_GUARD
//	printf("%s\n\nHistory: %s", *GErr.Message, *GErr.History);
#else
//	printf("%s\n", *GErr.Message);
#endif
}


void appAllowFpuXcpt(bool allow)
{
	//?? implement
}


/*-----------------------------------------------------------------------------
	Terminal control
-----------------------------------------------------------------------------*/

// window size changed
static void SigWinchHandler(int signum)
{
	//!! on Unix we will always have console; when have GUI+console - should ignore this signal?!
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1)
		GScreenWidth = ws.ws_col;
}


/*-----------------------------------------------------------------------------
	Initialization/finalization
-----------------------------------------------------------------------------*/

void appInitPlatform()
{
	// hook signals
	appHookExceptions();
	// hook SIGWINCH
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SigWinchHandler;
	sa.sa_flags   = 0;
	sigaction(SIGWINCH, &sa, NULL);
	// init console
	SigWinchHandler(SIGWINCH);
	// gather system information
	appDetectCPU();
	DetectOs();
	// setup current directory
	SetDefaultDirectory();
}


void appShutdownPlatform()
{
}
