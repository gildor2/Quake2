#include "server.h"
#include "../client/ref.h"


//?? create as class with allowed multiple instances of text container
//?? class name: CScreenTextContainer ?
//?? in gl, can use the same, simply export few renderer function for IMMEDIATE text drawing

#define TEXTBUF_SIZE 65536

struct textRec_t
{
	CVec3	pos;
	byte	anchor;		// 0 - left, 1 - right, 2 - 3D pos
	color_t	c;
	char	*text;
	textRec_t *next;
};

static char	textbuf[TEXTBUF_SIZE];
static int	textbufPos;			// position for next record
static bool	textbufFilled;		// count of records in buffer (0 or greater)
static textRec_t *lastTextRec;


void SV_ClearTexts ()
{
	textbufFilled = false;
}


void SV_DrawTexts ()
{
	guard(SV_DrawTexts);

	if (!textbufFilled) return;
	if (DEDICATED) return;

	for (textRec_t *rec = (textRec_t*)textbuf; rec; rec = rec->next)
	{
		switch (rec->anchor)
		{
		case 0:
			RE_DrawTextLeft (rec->text, rec->c.rgba);
			break;
		case 1:
			RE_DrawTextRight (rec->text, rec->c.rgba);
			break;
		default:
			RE_DrawText3D (rec->pos, rec->text, rec->c.rgba);
		}
	}
	unguard;
}


static void AddText (int anchor, const CVec3 &pos, const char *text, unsigned rgba)
{
	guard(SV_AddText);

	if (DEDICATED) return;
	if (!text || !*text) return;	// empty text

	if (!textbufFilled)
	{	// 1st record - perform initialization
		lastTextRec = NULL;
		textbufPos = 0;
		textbufFilled = true;
	}

	textRec_t *rec = (textRec_t*) &textbuf[textbufPos];
	int len = strlen (text);
	if (!len) return;

	int size = sizeof(textRec_t) + len + 1;
	if (size + textbufPos > TEXTBUF_SIZE) return;	// out of buffer space

	char *textCopy = (char*)(rec + 1);
	memcpy (textCopy, text, len);
	textCopy[len] = 0;
	rec->anchor = anchor;
	rec->pos  = pos;
	rec->text = textCopy;
	rec->c.rgba = rgba;
	rec->next = NULL;

	if (lastTextRec) lastTextRec->next = rec;
	lastTextRec = rec;
	textbufPos += size;

	unguard;
}


/*-----------------------------------------------------------------------------
	Drawing
-----------------------------------------------------------------------------*/

void SV_DrawTextLeft (const char *text, unsigned rgba)
{
	AddText (0, nullVec3, text, rgba);
}


void SV_DrawTextRight (const char *text, unsigned rgba)
{
	AddText (1, nullVec3, text, rgba);
}


void SV_DrawText3D (const CVec3 &pos, const char *text, unsigned rgba)
{
	AddText (2, pos, text, rgba);
}
