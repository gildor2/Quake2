#include "OpenGLDrv.h"
#include "gl_backend.h"
#include "gl_math.h"		// for ProjectToScreen()


namespace OpenGLDrv {


static cvar_t	*gl_logTexts;


/*-----------------------------------------------------------------------------
	Text output
-----------------------------------------------------------------------------*/

//?? should be synched with console+menu char sizes
#define CHAR_WIDTH	8
#define CHAR_HEIGHT	8

void DrawChar(int x, int y, int c, int color)
{
	if (c == ' ') return;
	BK_DrawText((char*)&c, 1, x, y, CHAR_WIDTH, CHAR_HEIGHT, colorTable[color]);
}


/*-----------------------------------------------------------------------------
	Static (change name?) text output
-----------------------------------------------------------------------------*/

#define TOP_TEXT_POS	64
#define CHARSIZE_X		6
#define CHARSIZE_Y		8


struct CRText : public CTextRec
{
	short	x, y;
	color_t	c;
};

static TTextContainer<CRText, 65536> Text;

static int nextLeft_y  = TOP_TEXT_POS;
static int nextRight_y = TOP_TEXT_POS;


void ClearTexts()
{
	nextLeft_y = nextRight_y = TOP_TEXT_POS;
	Text.Clear();
}


//?? later (CFont): implement as CFont method
static void GetTextExtents(const char *s, int &width, int &height)
{
	int x = 0, w = 0;
	int h = CHARSIZE_Y;
	while (char c = *s++)
	{
		if (c == '\n')
		{
			if (x > w) w = x;
			x = 0;
			h += CHARSIZE_Y;
			continue;
		}
		x += CHARSIZE_X;
	}
	width = max(x, w);
	height = h;
}


static void DrawText(const CRText *rec)
{
	if (gl_logTexts->integer)
		appPrintf(S_MAGENTA"%s\n", rec->text);

	int y = rec->y;
	const char *text = rec->text;

	while (true)
	{
		const char *s = strchr(text, '\n');
		int len = s ? s - text : strlen(text);

		BK_DrawText(text, len, rec->x, y, CHARSIZE_X, CHARSIZE_Y, rec->c.rgba);
		if (!s) return;							// all done

		y += CHARSIZE_Y;
		text = s + 1;
	}
}


void FlushTexts()
{
	Text.Enumerate(DrawText);
	if (gl_logTexts->integer == 2)				// special value: log only 1 frame
		Cvar_SetInteger("gl_logTexts", 0);
	nextLeft_y = nextRight_y = TOP_TEXT_POS;
	ClearTexts();
}


void DrawTextPos(int x, int y, const char *text, unsigned rgba)
{
	CRText *rec = Text.Add(text);
	if (!rec) return;
	rec->x      = x;
	rec->y      = y;
	rec->c.rgba = rgba;
}


void DrawTextLeft(const char *text, unsigned rgba)
{
	int w, h;
	if (nextLeft_y >= vid_height) return;	// out of screen
	GetTextExtents(text, w, h);
	DrawTextPos(0, nextLeft_y, text, rgba);
	nextLeft_y += h;
}


void DrawTextRight(const char *text, unsigned rgba)
{
	int w, h;
	if (nextRight_y >= vid_height) return;	// out of screen
	GetTextExtents(text, w, h);
	DrawTextPos(vid_width - w, nextRight_y, text, rgba);
	nextRight_y += h;
}


void DrawText3D(const CVec3 &pos, const char *text, unsigned rgba)
{
	int coords[2];
	if (!ProjectToScreen(pos, coords)) return;
	DrawTextPos(coords[0], coords[1], text, rgba);
}


/*-----------------------------------------------------------------------------
	Initialization
-----------------------------------------------------------------------------*/

void InitTexts()
{
	gl_logTexts = Cvar_Get("gl_logTexts", "0");
	//?? should init fonts here
}


} // namespace
