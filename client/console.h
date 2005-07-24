#ifndef __CONSOLE_INCLUDED__
#define __CONSOLE_INCLUDED__


extern int con_height;


void Con_CheckResize ();
void Con_Init ();
void Con_DrawConsole (float frac);
//void Con_Print (const char *txt); -- defined in qcommon.h
void Con_Clear_f ();
void Con_DrawNotify (bool drawBack);
void Con_ClearTyping ();
void Con_ClearNotify ();

void Key_Console (int key, int modKey);


#endif
