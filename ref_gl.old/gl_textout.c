// gl_textout.c

#include "gl_local.h"

#define TEXTBUF_SIZE 65536

#define TOP_TEXT_POS 64
#define CHARSIZE_X 6
#define CHARSIZE_Y 8

typedef struct textrec_s
{
	// screen position
	int	x;
	int	y;
	// color
	float	r;
	float	g;
	float	b;
	// text
	char	*text;
	// other
	struct textrec_s *next;
} textrec_t;

char gl_textbuf[TEXTBUF_SIZE];
int gl_textbuf_pos;	// position for next record
int gl_textbuf_count;	// count of records in buffer (0 or greater)
textrec_t *gl_last_rec;

int gl_next_left_y = TOP_TEXT_POS;
int gl_next_right_y = TOP_TEXT_POS;

extern	image_t	*draw_chars;


void DrawColoredText (int x, int y, char *text, float r, float g, float b)
{
	char	c;
	float	frow, fcol, size;

	if (!text || !*text) return; // nothing to out
	GL_Bind (draw_chars->texnum);
	qglColor3f (r, g, b);	// make colored chars
	GL_TexEnv(GL_MODULATE);
	qglBegin (GL_QUADS);

	size = 0.0625;

	while (c = *text++)
	{
		frow = (c >> 4) * 0.0625;
		fcol = (c & 15) * 0.0625;

		qglTexCoord2f (fcol, frow);
		qglVertex2f (x, y);
		qglTexCoord2f (fcol + size, frow);
		qglVertex2f (x+CHARSIZE_X, y);
		qglTexCoord2f (fcol + size, frow + size);
		qglVertex2f (x+CHARSIZE_X, y+CHARSIZE_Y);
		qglTexCoord2f (fcol, frow + size);
		qglVertex2f (x, y+CHARSIZE_Y);
		x += CHARSIZE_X;
	}
	qglEnd ();
	GL_TexEnv(GL_REPLACE);
	qglColor3f (1, 1, 1);
}


void R_DrawTexts (void)
{
	textrec_t *rec;

	gl_next_left_y = gl_next_right_y = TOP_TEXT_POS;
	if (!gl_textbuf_count) return;

	rec = (textrec_t*)gl_textbuf;
	while (rec)
	{
		DrawColoredText (rec->x, rec->y, rec->text, rec->r, rec->g, rec->b);
		rec = rec->next;
	}

	gl_textbuf_count = 0;
}


void DrawTextPos (int x, int y, char *text, float r, float g, float b)
{
	int size;
	char *text_copy;
	textrec_t *rec;

	if (!text || !*text) return; // empty text
	if (!gl_textbuf_count)
	{	// 1st record - perform initialization
		gl_last_rec = NULL;
		gl_textbuf_pos = 0;
		rec = (textrec_t*) gl_textbuf;
	}
	else
		rec = (textrec_t*) &gl_textbuf[gl_textbuf_pos];

	size = sizeof(textrec_t) + strlen (text) + 1;
	if (size + gl_textbuf_pos > TEXTBUF_SIZE) return; // out of buffer space

	text_copy = (char*)rec + sizeof(textrec_t);
	strcpy (text_copy, text);
	rec->x = x;
	rec->y = y;
	rec->text = text_copy;
	rec->r = r;
	rec->g = g;
	rec->b = b;
	rec->next = NULL;

	if (gl_last_rec) gl_last_rec->next = rec;
	gl_last_rec = rec;
	gl_textbuf_pos += size;
	gl_textbuf_count++;
}


void DrawTextLeft (char *text, float r, float g, float b)
{
	if (gl_next_left_y >= vid.height) return; // out of screen
	DrawTextPos (0, gl_next_left_y, text, r, g, b);
	gl_next_left_y += CHARSIZE_Y;
}


void DrawTextRight (char *text, float r, float g, float b)
{
	if (gl_next_right_y >= vid.height) return; // out of screen
	DrawTextPos (vid.width - strlen(text) * CHARSIZE_X, gl_next_right_y, text, r, g, b);
	gl_next_right_y += CHARSIZE_Y;
}
