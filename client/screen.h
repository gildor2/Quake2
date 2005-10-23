void SCR_Init ();
void SCR_UpdateScreen ();

void SCR_CenterPrint (const char *str);

//void SCR_BeginLoadingPlaque (void); -- declared in qcommon.h
void SCR_EndLoadingPlaque (bool force);	//?? is "force" needed ?

//void SCR_DebugGraph (float value, int color); -- declared in qcommon.h (for server)

void SCR_TouchPics ();

// here for menu:
extern	cvar_t	*crosshair;
extern	cvar_t	*crosshairColor;

void DrawString (int x, int y, const char *s);	// draw colorized string; used by menus/console
void SCR_SetLevelshot (const char *name = NULL); // give levelshot name (if name not specified - compute from map name)
void SCR_ShowConsole (bool show, bool noAnim);
void SCR_ToggleConsole ();
void Key_Message (int key);

// cl_cin.cpp
void SCR_PlayCinematic (const char *filename);	// begin
bool SCR_DrawCinematic ();						// play/draw
void SCR_StopCinematic ();						// stop
