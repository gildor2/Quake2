//!! I don't like "Pic" suffix: replace with "Image" ?

// called when the library is loaded
bool	Init ();
// called before the library is unloaded
void	Shutdown (bool complete = false);

void	LoadNewWorld (const char *map);
void	FinishLoadingWorld ();
CRenderModel* RegisterModel (const char *name);
CBasicImage* RegisterSkin (const char *name, bool force = false);
CBasicImage* RegisterPic (const char *name);	//?? rename (GetPic() etc)
void	SetSky (const char *name, float rotate, const CVec3 &axis);

// frame; rename!!
void	BeginFrame (double time);
void	RenderFrame (refdef_t *fd);
void	EndFrame ();

// drawing
void	DrawPic (int x, int y, const char *pic, int anchor = ANCHOR_TOP_LEFT, int color = C_WHITE);
void	DrawStretchPic (int x, int y, int w, int h, const char *name);
void	DrawDetailedPic (int x, int y, int w, int h, const char *name);
void	DrawChar (int x, int y, int c, int color = C_WHITE);
void	TileClear (int x, int y, int w, int h, const char *name);
void	Fill (int x, int y, int w, int h, unsigned rgba);
void	Fill8 (int x, int y, int w, int h, int c);
void	DrawStretchRaw8 (int x, int y, int w, int h, int cols, int rows, byte *data, unsigned *palette);

void	AppActivate (bool activate);
void	Screenshot (int flags, const char *name);

// drawing colored text in any screen position
void	DrawTextPos (int x, int y, const char *text, unsigned rgba = 0xFFFFFFFF);
void	DrawTextLeft (const char *text, unsigned rgba = 0xFFFFFFFF);
void	DrawTextRight (const char *text, unsigned rgba = 0xFFFFFFFF);
void	DrawText3D (const CVec3 &pos, const char *text, unsigned rgba = 0xFFFFFFFF);

// lighting
float	GetClientLight ();			// used by server to determine client visibility (AI); change ??

// debugging
#if !NO_DEBUG
void	DrawBrush (const CBrush *brush, const char *label, int num);
#endif
