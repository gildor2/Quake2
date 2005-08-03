#include "qcommon.h"
#include "../client/ref.h"


struct CSVText : CTextRec
{
	CVec3	pos;
	byte	anchor;				// 0 - left, 1 - right, 2 - 3D pos
	color_t	c;
};

static TTextContainer<CSVText, 65536> Text;


void SV_DrawTextLeft (const char *text, unsigned rgba)
{
	if (DEDICATED) return;
	CSVText *r = Text.Add (text);
	if (!r) return;
	r->anchor = 0;
	r->c.rgba = rgba;
}


void SV_DrawTextRight (const char *text, unsigned rgba)
{
	if (DEDICATED) return;
	CSVText *r = Text.Add (text);
	if (!r) return;
	r->anchor = 1;
	r->c.rgba = rgba;
}


void SV_DrawText3D (const CVec3 &pos, const char *text, unsigned rgba)
{
	if (DEDICATED) return;
	CSVText *r = Text.Add (text);
	if (!r) return;
	r->anchor = 2;
	r->c.rgba = rgba;
	r->pos    = pos;
}


void SV_ClearTexts ()
{
	Text.Clear ();
}


static void DrawSvText (const CSVText *r)
{
	if (r->anchor == 0)
		RE_DrawTextLeft (r->text, r->c.rgba);
	else if (r->anchor == 1)
		RE_DrawTextRight (r->text, r->c.rgba);
	else
		RE_DrawText3D (r->pos, r->text, r->c.rgba);
}


void SV_DrawTexts ()
{
	if (DEDICATED) return;
	Text.Enumerate (DrawSvText);
}
