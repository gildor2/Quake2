// gl_textout.c

#include "gl_local.h"

#define TEXTBUF_SIZE 65536

#define TOP_TEXT_POS 64
#define CHARSIZE_X 6
#define CHARSIZE_Y 8

typedef struct textrec_s
{
	// screen position
	int		x;
	int		y;
	// color
	unsigned rgba;
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


void DrawColoredText (int x, int y, char *text, unsigned rgba)
{
	char	c;
	float	frow, fcol, size;

	if (!text || !*text) return;	// nothing to out
	GL_Bind (draw_chars->texnum);
	glColor4ubv ((byte*)&rgba);	// make colored chars
	GL_TexEnv(GL_MODULATE);
	glBegin (GL_QUADS);

	size = 0.0625;

	while (c = *text++)
	{
		frow = (c >> 4) * 0.0625;
		fcol = (c & 15) * 0.0625;

		glTexCoord2f (fcol, frow);
		glVertex2f (x, y);
		glTexCoord2f (fcol + size, frow);
		glVertex2f (x+CHARSIZE_X, y);
		glTexCoord2f (fcol + size, frow + size);
		glVertex2f (x+CHARSIZE_X, y+CHARSIZE_Y);
		glTexCoord2f (fcol, frow + size);
		glVertex2f (x, y+CHARSIZE_Y);
		x += CHARSIZE_X;
	}
	glEnd ();
	GL_TexEnv(GL_REPLACE);
	glColor3f (1, 1, 1);
}


void R_DrawTexts (void)
{
	textrec_t *rec;

	gl_next_left_y = gl_next_right_y = TOP_TEXT_POS;
	if (!gl_textbuf_count) return;

	rec = (textrec_t*)gl_textbuf;
	while (rec)
	{
		DrawColoredText (rec->x, rec->y, rec->text, rec->rgba);
		rec = rec->next;
	}

	gl_textbuf_count = 0;
}


void DrawTextPos (int x, int y, char *text, unsigned rgba)
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
	rec->rgba= rgba;
	rec->next = NULL;

	if (gl_last_rec) gl_last_rec->next = rec;
	gl_last_rec = rec;
	gl_textbuf_pos += size;
	gl_textbuf_count++;
}


void DrawTextLeft (char *text, unsigned rgba)
{
	if (gl_next_left_y >= vid.height) return; // out of screen
	DrawTextPos (0, gl_next_left_y, text, rgba);
	gl_next_left_y += CHARSIZE_Y;
}


void DrawTextRight (char *text, unsigned rgba)
{
	if (gl_next_right_y >= vid.height) return; // out of screen
	DrawTextPos (vid.width - strlen(text) * CHARSIZE_X, gl_next_right_y, text, rgba);
	gl_next_right_y += CHARSIZE_Y;
}
