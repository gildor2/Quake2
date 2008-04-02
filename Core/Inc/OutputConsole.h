#if _WIN32

// win32 standard console

//?? problems:
//?? - should not get hOutput in constructor
//?? - may be, should call AllocConsole()
//?? - should include <windows.h> here, but it should be included before "Core.h", otherwise will be conflicts with
//??   some declarations from "CoreWin32.h"
//?? may be, separate as .cpp file - will avoid some problems ...
//?? - best way: create funcs appConWrite() and appConRead() -- place to Core[Win32|Unix].cpp, and simply create wrapper
//??   here (derived from COutputDevice)
//?? - problems here:
//??   - key names for appConRead(): should provide Core-level key names for cursor keys etc (not all keys!)
//?? ? create CInputDevice, derive console input from it (and make such device for quake console, which will catch input
//??   from GUI keyboard when console is active) -- OR -- for GUI console, add field, containing pointer to func, which
//??   will return console key (appConRead()) ... --- should think about this ...

class COutputDeviceCon : public COutputDevice
{
public:
	virtual void Write(const char *str)
	{
		HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE); // fast function ...
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
			DWORD dummy;
			WriteFile(hOutput, &c, 1, &dummy, NULL);
		}
		SetConsoleTextAttribute(hOutput, 7);
	}
};

#else

// Unix: vt100 or linux terminal

class COutputDeviceCon : public COutputDevice
{
public:
#if __CYGWIN__
	COutputDeviceCon()
	{
		setbuf(stdout, NULL);						// disable buffering; required for cygwin, when connected to remote terminal
	}
#endif
	virtual void Write(const char *str)
	{
		byte color = 0;
		// draw message
		while (char c = *str++)
		{
			// parse color info
			if (c == COLOR_ESCAPE && str[0] >= '0' && str[0] <= '7')
			{
				static char buf[] = "\033[30m";
				buf[3] = str[0];
				color = str[0] - '0';
				fwrite(buf, 5, 1, stdout);
				str++;
				continue;
			}
			fwrite(&c, 1, 1, stdout);
		}
		if (color)
			fwrite("\033[0m", 4, 1, stdout);		// reset color attribute
	}
};

#endif
