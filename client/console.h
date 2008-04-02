#ifndef __CONSOLE_INCLUDED__
#define __CONSOLE_INCLUDED__


extern int con_height;


void Con_CheckResize();
void Con_Init();
void Con_DrawConsole(float frac);
void Con_DrawNotify(bool drawBack);
void Con_ClearTyping();
void Con_ClearNotify();

void Key_Console(int key, int modKey);

class COutputDeviceCon : public COutputDevice
{
public:
	void Init()
	{
		// why not constructor: used as static var, but console is not always exists
		// (no for dedicated server) - so, Register() by hands ...
		Register();
	}
	~COutputDeviceCon()
	{
		// ... but unregister automatically
		Unregister();
	}
	virtual void Write(const char *str);
};


#endif
