#ifndef __KEYS_INCULDED__
#define __KEYS_INCULDED__


#define NUM_KEYS		512

// 32..127 -- printable keys, 128..255 -- known other keys, 256..511 -- unknown (non-standard) keys
enum {
	K_TAB		= 9,
	K_ENTER		= 13,
	K_ESCAPE	= 27,
	K_SPACE		= 32,
	// normal keys should be passed as lowercased ascii

	K_BACKSPACE	= 128,
	K_UPARROW, K_DOWNARROW, K_LEFTARROW, K_RIGHTARROW,

	K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, K_F7, K_F8, K_F9, K_F10, K_F11, K_F12,
	K_F13, K_F14, K_F15, K_F16, K_F17, K_F18, K_F19, K_F20, K_F21, K_F22, K_F23, K_F24,
	K_ALT, K_CTRL, K_SHIFT,
	K_INS, K_DEL,
	K_PGDN, K_PGUP, K_HOME, K_END,

	// keypad
	K_KP_HOME,
	K_KP_UPARROW,
	K_KP_PGUP,
	K_KP_LEFTARROW,
	K_KP_5,
	K_KP_RIGHTARROW,
	K_KP_END,
	K_KP_DOWNARROW,
	K_KP_PGDN,
	K_KP_ENTER,
	K_KP_INS,
	K_KP_DEL,
	K_KP_SLASH,
	K_KP_MINUS,
	K_KP_PLUS,

	// mouse buttons
	K_MOUSE1,
	K_MOUSE2,
	K_MOUSE3,
	K_MWHEELDOWN,
	K_MWHEELUP,

	// joystick buttons
	K_JOY1, K_JOY2, K_JOY3, K_JOY4,

	// aux keys are for multi-buttoned joysticks
	K_AUX1,	K_AUX2,	K_AUX3,	K_AUX4,	K_AUX5,	K_AUX6,
	K_AUX7,	K_AUX8,	K_AUX9,	K_AUX10, K_AUX11, K_AUX12,
	K_AUX13, K_AUX14, K_AUX15, K_AUX16, K_AUX17, K_AUX18,
	K_AUX19, K_AUX20, K_AUX21, K_AUX22, K_AUX23, K_AUX24,
	K_AUX25, K_AUX26, K_AUX27, K_AUX28, K_AUX29, K_AUX30,
	K_AUX31, K_AUX32,

	K_CAPSLOCK,
	K_NUMLOCK,
	K_SCRLOCK,
	K_PRINTSCRN,
	K_PAUSE
};


extern char		*keybindings[NUM_KEYS];
extern byte		keydown[NUM_KEYS];
extern int		key_repeats[NUM_KEYS];

extern int		anykeydown;

void Key_Event (int key, qboolean down, unsigned time);
void Key_Init (void);
void Key_WriteBindings (FILE *f);
void Key_SetBinding (int keynum, char *binding);
void Key_ClearStates (void);
void Key_ClearTyping (void);

#define		MAXCMDLINE	256
extern char		editLine[MAXCMDLINE];
extern int		editPos;

void CompleteCommand (void);


#endif
