#ifndef __CONSOLE_INCLUDED__
#define __CONSOLE_INCLUDED__

#define	NUM_CON_TIMES 4

#define	CON_TEXTSIZE 32768


typedef struct
{
	int		line;
	int		pos;
} con_cache_t;

typedef struct
{
	qboolean initialized;
	qboolean wrapped;		// will be set to true after 1st buffer wrap
	qboolean started;		// "false" if was no access to console
	qboolean wordwrap;

	char	text[CON_TEXTSIZE * 2];	// first half - text, second - color
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int		ormask;			// high bit mask for colored characters

	int		linewidth;		// characters across screen
	int		totallines;		// total lines in console scrollback

	int		startpos;		// points to begin of text buffer
	int		endpos;			// text will be placed to endpos

	float	cursorspeed;

	int		vislines;

	con_cache_t	disp;
	con_cache_t	notif;

	float	times[NUM_CON_TIMES];	// cls.realtime time the line was generated
									// for transparent notify lines
} console_t;

extern console_t con;


void Con_DrawCharacter (int cx, int line, int num);

void Con_CheckResize (void);
void Con_Init (void);
void Con_DrawConsole (float frac);
void Con_Print (char *txt);
void Con_CenteredPrint (char *text);
void Con_Clear_f (void);
void Con_DrawNotify (void);
void Con_ClearNotify (void);
void Con_ToggleConsole_f (void);


#endif
