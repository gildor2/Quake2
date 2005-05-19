#include "gl_local.h"
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

void DrawChar (int x, int y, int c, int color)
{
	if (c == ' ') return;
	BK_DrawText ((char*)&c, 1, x, y, CHAR_WIDTH, CHAR_HEIGHT, colorTable[color]);
}


void DrawConChar (int x, int y, int c, int color)
{
	if (c == ' ') return;
	BK_DrawText ((char*)&c, 1, x * CHAR_WIDTH, y * CHAR_HEIGHT, CHAR_WIDTH, CHAR_HEIGHT, colorTable[color]);
}


/*-----------------------------------------------------------------------------
	Static (change name?) text output
-----------------------------------------------------------------------------*/

#define TEXTBUF_SIZE 65536

#define TOP_TEXT_POS 64
#define CHARSIZE_X 6
#define CHARSIZE_Y 8


struct textRec_t
{
	short	x, y;
	color_t	c;
	char	*text;
	textRec_t *next;
};


static char	textbuf[TEXTBUF_SIZE];
static int	textbufPos;			// position for next record
static bool	textbufEmpty;		// count of records in buffer (0 or greater)
static textRec_t *lastTextRec;

static int nextLeft_y = TOP_TEXT_POS;
static int nextRight_y = TOP_TEXT_POS;


void ClearTexts ()
{
	nextLeft_y = nextRight_y = TOP_TEXT_POS;
	textbufEmpty = true;
}


//?? later (CFont): implement as CFont method
static void GetTextExtents (const char *s, int &width, int &height)
{
	int		x, w, h;
	x = w = 0;
	h = CHARSIZE_Y;
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


void FlushTexts ()
{
	nextLeft_y = nextRight_y = TOP_TEXT_POS;
	if (textbufEmpty) return;

	for (textRec_t *rec = (textRec_t*)textbuf; rec; rec = rec->next)
	{
		int len = strlen (rec->text);
		if (!len) continue;

		if (gl_logTexts->integer)
			Com_Printf (S_MAGENTA"%s\n", rec->text);
		BK_DrawText (rec->text, len, rec->x, rec->y, CHARSIZE_X, CHARSIZE_Y, rec->c.rgba);
	}
	if (gl_logTexts->integer == 2)				// special value: log only 1 frame
		Cvar_SetInteger ("gl_logTexts", 0);

	textbufEmpty = true;
}


static void DrawTextPos (int x, int y, const char *text, unsigned rgba)
{
	if (!text || !*text) return;	// empty text

	if (textbufEmpty)
	{	// 1st record - perform initialization
		lastTextRec = NULL;
		textbufPos = 0;
		textbufEmpty = false;
	}

	while (true)
	{
		textRec_t *rec = (textRec_t*) &textbuf[textbufPos];
		const char *s = strchr (text, '\n');
		int len = s ? s - text : strlen (text);

		if (len)
		{
			int size = sizeof(textRec_t) + len + 1;
			if (size + textbufPos > TEXTBUF_SIZE) return;	// out of buffer space

			char *textCopy = (char*)(rec + 1);
			memcpy (textCopy, text, len);
			textCopy[len] = 0;
			rec->x = x;
			rec->y = y;
			rec->text = textCopy;
			rec->c.rgba = rgba;
			rec->next = NULL;

			if (lastTextRec) lastTextRec->next = rec;
			lastTextRec = rec;
			textbufPos += size;
		}
		y += CHARSIZE_Y;
		if (s)
			text = s + 1;
		else
			return;
	}
}


void DrawTextLeft (const char *text, unsigned rgba)
{
	int		w, h;
	if (nextLeft_y >= vid_height) return;	// out of screen
	GetTextExtents (text, w, h);
	DrawTextPos (0, nextLeft_y, text, rgba);
	nextLeft_y += h;
}


void DrawTextRight (const char *text, unsigned rgba)
{
	int		w, h;
	if (nextRight_y >= vid_height) return;	// out of screen
	GetTextExtents (text, w, h);
	DrawTextPos (vid_width - w, nextRight_y, text, rgba);
	nextRight_y += h;
}


void DrawText3D (vec3_t pos, const char *text, unsigned rgba)
{
	int		coords[2];

	if (!ProjectToScreen (pos, coords)) return;
	DrawTextPos (coords[0], coords[1], text, rgba);
}


/*-----------------------------------------------------------------------------
	Initialization
-----------------------------------------------------------------------------*/

void InitTexts ()
{
	gl_logTexts = Cvar_Get ("gl_logTexts", "0", 0);
	//?? should init fonts here
}


} // namespace
