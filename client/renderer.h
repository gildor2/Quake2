//!! I don't like "Pic" suffix: replace with "Image" ?

unsigned GetCaps ();

// called when the library is loaded
bool	Init ();
// called before the library is unloaded
void	Shutdown (bool complete = false);

void	BeginRegistration (const char *map);	//?? rename
CRenderModel* RegisterModel (const char *name);
CBasicImage* RegisterSkin (const char *name);
CBasicImage* RegisterPic (const char *name);	//?? rename (GetPic() etc)
void	SetSky (const char *name, float rotate, const CVec3 &axis);
void	EndRegistration ();						//?? remove

// frame; rename!!
void	BeginFrame ();
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
void	DrawTextPos (int x, int y, const char *text, unsigned rgba);
void	DrawTextLeft (const char *text, unsigned rgba);
void	DrawTextRight (const char *text, unsigned rgba);

// draw char at (x,y) (char-related coordinates)
void	DrawConChar (int x, int y, int c, int color = C_WHITE);

// lighting
float	GetClientLight ();			// used by server to determine client visibility (AI); change ??
