/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// draw.c

#include "r_local.h"


image_t		*draw_chars;				// 8*8 graphic characters

static int colortable[8] = {0x00, 0xF2, 0xD0, 0xDD, 0xF3, 0xB0, 0x6F, 0xD7};

//=============================================================================

/*
================
Draw_FindPic
================
*/
image_t *Draw_FindPic (char *name)
{
	image_t	*image;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf (fullname, sizeof(fullname), "pics/%s.pcx", name);
		image = R_FindImage (fullname, it_pic);
	}
	else
		image = R_FindImage (name+1, it_pic);

	return image;
}



/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal (void)
{
	draw_chars = Draw_FindPic ("conchars");
}



/*
================
Draw_Char

Draws one 8*8 graphics character
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Char (int x, int y, int num)
{
	byte			*dest;
	byte			*source;
	int				drawline;
	int				row, col;

	num &= 255;

	if (num == 32 || num == 32+128)
		return;

	if (y <= -8 || y >= vid.height)
		return;			// totally off screen

//--	if ( ( y + 8 ) >= vid.height )
//	if ( ( y + 8 ) > vid.height )		// PGM - status text was missing in sw...
//		return;

#ifdef PARANOID
	if (y > vid.height - 8 || x < 0 || x > vid.width - 8)
		Com_Error (ERR_FATAL,"Con_DrawCharacter: (%i, %i)", x, y);
	if (num < 0 || num > 255)
		Com_Error (ERR_FATAL,"Con_DrawCharacter: char %i", num);
#endif

	row = num>>4;
	col = num&15;
	source = draw_chars->pixels[0] + (row<<10) + (col<<3);

	if (y < 0)
	{	// clipped
		drawline = 8 + y;
		source -= 128*y;
		y = 0;
	}
	else
		drawline = 8;


	dest = vid.buffer + y*vid.rowbytes + x;

	while (drawline--)
	{
#define _PUT(i)	if (source[i] != TRANSPARENT_COLOR) dest[i] = source[i]
		_PUT(0); _PUT(1); _PUT(2); _PUT(3);
		_PUT(4); _PUT(5); _PUT(6); _PUT(7);
#undef _PUT
		source += 128;
		dest += vid.rowbytes;
	}
}

/*
================
Draw_CharColor
================
*/
#define MARK_COLOR_8BIT 0xEF0000

void Draw_CharColor (int x, int y, int num, int color)
{
	byte			*dest;
	byte			*source;
	int				drawline;
	int				row, col, c;

	num &= 255;

	if (num == 32 || num == 32+128)
		return;

	if (y <= -8 || y >= vid.height)
		return;			// totally off screen

	if ((color & 0xFFFFFF00) == MARK_COLOR_8BIT)
		c = color & 0xFF;
	else
	{
		if (color < 0 || color >= 7)
		{
			Draw_Char (x, y, num);
			return;
		}
		c = colortable[color];
	}

	row = num>>4;
	col = num&15;
	source = draw_chars->pixels[0] + (row<<10) + (col<<3);

	if (y < 0)
	{	// clipped
		drawline = 8 + y;
		source -= 128*y;
		y = 0;
	}
	else
		drawline = 8;

	dest = vid.buffer + y*vid.rowbytes + x;

	while (drawline--)
	{
#define _PUT(i)	if (source[i] != TRANSPARENT_COLOR) dest[i] = c
		_PUT(0); _PUT(1); _PUT(2); _PUT(3);
		_PUT(4); _PUT(5); _PUT(6); _PUT(7);
#undef _PUT
		source += 128;
		dest += vid.rowbytes;
	}
}


#define TEXTBUF_SIZE 65536

#define TOP_TEXT_POS 64

typedef struct textrec_s
{
	// screen position
	int	x;
	int	y;
	int color;
	// text
	char	*text;
	// other
	struct textrec_s *next;
} textrec_t;

char textbuf[TEXTBUF_SIZE];
int textbuf_pos;	// position for next record
int textbuf_count;	// count of records in buffer (0 or greater)
textrec_t *last_rec;

int next_left_y = TOP_TEXT_POS;
int next_right_y = TOP_TEXT_POS;

extern	image_t	*draw_chars;


void DrawText (int x, int y, char *text, int color)
{
	char	c;

	if (!text || !*text) return; // nothing to out
	while (c = *text++)
	{
		Draw_CharColor (x, y, c, color);
		x += 8;
	}
}


void R_DrawTexts (void)
{
	textrec_t *rec;

	next_left_y = next_right_y = TOP_TEXT_POS;
	if (!textbuf_count) return;

	rec = (textrec_t*)textbuf;
	while (rec)
	{
		DrawText (rec->x, rec->y, rec->text, rec->color);
		rec = rec->next;
	}

	textbuf_count = 0;
}

extern unsigned char d_16to8table[65536];

void DrawText_Pos (int x, int y, char *text, unsigned rgba)
{
	int size;
	char *text_copy;
	textrec_t *rec;
	int ri, gi, bi, color;

	ri = (rgba >> 3) & 31;			// 5 bits
	gi = (rgba >> (8+2)) & 63;		// 6 bits
	bi = (rgba >> (16+3)) & 31;		// 5 bits
	color = d_16to8table[ri | (gi << 5) | (bi << 11)] | MARK_COLOR_8BIT;

	if (!text || !*text) return; // empty text
	if (!textbuf_count)
	{	// 1st record - perform initialization
		last_rec = NULL;
		textbuf_pos = 0;
		rec = (textrec_t*) textbuf;
	}
	else
		rec = (textrec_t*) &textbuf[textbuf_pos];

	size = sizeof(textrec_t) + strlen (text) + 1;
	if (size + textbuf_pos > TEXTBUF_SIZE) return; // out of buffer space

	text_copy = (char*)rec + sizeof(textrec_t);
	strcpy (text_copy, text);
	rec->x = x;
	rec->y = y;
	rec->color = color;
	rec->text = text_copy;
	rec->next = NULL;

	if (last_rec) last_rec->next = rec;
	last_rec = rec;
	textbuf_pos += size;
	textbuf_count++;
}

void DrawText_Left (char *text, unsigned rgba)
{
	if (next_left_y >= vid.height) return; // out of screen
	DrawText_Pos (0, next_left_y, text, rgba);
	next_left_y += 8;
}

void DrawText_Right (char *text, unsigned rgba)
{
	if (next_right_y >= vid.height) return; // out of screen
	DrawText_Pos (vid.width - strlen(text) * 8, next_right_y, text, rgba);
	next_right_y += 8;
}

/*
=============
Draw_GetPicSize
=============
*/
void Draw_GetPicSize (int *w, int *h, char *pic)
{
	image_t *gl;

	gl = Draw_FindPic (pic);
	if (!gl)
	{
		if (w) *w = 0;
		if (h) *h = 0;
	}
	else
	{
		if (w) *w = gl->width;
		if (h) *h = gl->height;
	}
}

/*
=============
Draw_StretchPicImplementation
=============
*/
void Draw_StretchPicImplementation (int x, int y, int w, int h, image_t	*pic)
{
	byte			*dest, *source;
	int				v, u, sv;
	int				height;
	int				f, fstep;
	int				skip;

	if ((x < 0) ||
		(x + w > vid.width) ||
		(y + h > vid.height))
	{
		Com_Error (ERR_FATAL,"Draw_Pic: bad coordinates");
	}

	height = h;
	if (y < 0)
	{
		skip = -y;
		height += y;
		y = 0;
	}
	else
		skip = 0;

	dest = vid.buffer + y * vid.rowbytes + x;

	for (v=0 ; v<height ; v++, dest += vid.rowbytes)
	{
		sv = (skip + v)*pic->height/h;
		source = pic->pixels[0] + sv*pic->width;
		if (w == pic->width)
			memcpy (dest, source, w);
		else
		{
			f = 0;
			fstep = pic->width*0x10000/w;
			for (u=0 ; u<w ; u+=4)
			{
				dest[u] = source[f>>16];
				f += fstep;
				dest[u+1] = source[f>>16];
				f += fstep;
				dest[u+2] = source[f>>16];
				f += fstep;
				dest[u+3] = source[f>>16];
				f += fstep;
			}
		}
	}
}

/*
=============
Draw_StretchPic
=============
*/
void Draw_StretchPic (int x, int y, int w, int h, char *name)
{
	image_t	*pic;

	pic = Draw_FindPic (name);
	if (!pic)
	{
		Com_Printf ("Can't find pic: %s\n", name);
		return;
	}
	Draw_StretchPicImplementation (x, y, w, h, pic);
}

/*
=============
Draw_StretchRaw
=============
*/
void Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data)
{
	image_t	pic;

	pic.pixels[0] = data;
	pic.width = cols;
	pic.height = rows;
	Draw_StretchPicImplementation (x, y, w, h, &pic);
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, char *name)
{
	image_t			*pic;
	byte			*dest, *source;
	int				v, u;
	int				tbyte;
	int				height;

	pic = Draw_FindPic (name);
	if (!pic)
	{
		Com_Printf ("Can't find pic: %s\n", name);
		return;
	}

	if ((x < 0) ||
		(x + pic->width > vid.width) ||
		(y + pic->height > vid.height))
		return;	//	Com_Error (ERR_FATAL,"Draw_Pic: bad coordinates");

	height = pic->height;
	source = pic->pixels[0];
	if (y < 0)
	{
		height += y;
		source += pic->width*-y;
		y = 0;
	}

	dest = vid.buffer + y * vid.rowbytes + x;

	if (!pic->transparent)
	{
		for (v=0 ; v<height ; v++)
		{
			memcpy (dest, source, pic->width);
			dest += vid.rowbytes;
			source += pic->width;
		}
	}
	else
	{
		if (pic->width & 7)
		{	// general
			for (v=0 ; v<height ; v++)
			{
#define _PUT(i) if ((tbyte=source[u+i]) != TRANSPARENT_COLOR) dest[u+i] = tbyte
				for (u=0 ; u<pic->width ; u++)
					_PUT(0);

				dest += vid.rowbytes;
				source += pic->width;
			}
		}
		else
		{	// unwound
			for (v=0 ; v<height ; v++)
			{
				for (u=0 ; u<pic->width ; u+=8)
				{
					_PUT(0); _PUT(1); _PUT(2); _PUT(3);
					_PUT(4); _PUT(5); _PUT(6); _PUT(7);
#undef _PUT
				}
				dest += vid.rowbytes;
				source += pic->width;
			}
		}
	}
}

/*
=============
Draw_PicColor
=============
*/

void Draw_PicColor (int x, int y, char *name, int color)
{
	image_t			*pic;
	byte			*dest, *source;
	int				v, u;
	int				c;
	int				height;

	if (color < 0 || color >= 7)
	{
		Draw_Pic (x, y, name);
		return;
	}

	c = colortable[color];

	pic = Draw_FindPic (name);
	if (!pic)
	{
		Com_Printf ("Can't find pic: %s\n", name);
		return;
	}

	if ((x < 0) ||
		(x + pic->width > vid.width) ||
		(y + pic->height > vid.height))
		return;	//	Com_Error (ERR_FATAL,"Draw_Pic: bad coordinates");

	height = pic->height;
	source = pic->pixels[0];
	if (y < 0)
	{
		height += y;
		source += pic->width*-y;
		y = 0;
	}

	dest = vid.buffer + y * vid.rowbytes + x;

	if (!pic->transparent)
	{
		for (v=0 ; v<height ; v++)
		{
			memcpy (dest, source, pic->width);
			dest += vid.rowbytes;
			source += pic->width;
		}
	}
	else
	{
		if (pic->width & 7)
		{	// general
			for (v=0 ; v<height ; v++)
			{
#define _PUT(i) if (source[u+i] != TRANSPARENT_COLOR) dest[u+i] = c
				for (u=0 ; u<pic->width ; u++)
					_PUT(0);

				dest += vid.rowbytes;
				source += pic->width;
			}
		}
		else
		{	// unwound
			for (v=0 ; v<height ; v++)
			{
				for (u=0 ; u<pic->width ; u+=8)
				{
					_PUT(0); _PUT(1); _PUT(2); _PUT(3);
					_PUT(4); _PUT(5); _PUT(6); _PUT(7);
#undef _PUT
				}
				dest += vid.rowbytes;
				source += pic->width;
			}
		}
	}
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h, char *name)
{
	int			i, j;
	byte		*psrc;
	byte		*pdest;
	image_t		*pic;
	int			x2;

	if (x < 0)
	{
		w += x;
		x = 0;
	}
	if (y < 0)
	{
		h += y;
		y = 0;
	}
	if (x + w > vid.width)
		w = vid.width - x;
	if (y + h > vid.height)
		h = vid.height - y;
	if (w <= 0 || h <= 0)
		return;

	pic = Draw_FindPic (name);
	if (!pic)
	{
		Com_Printf ("Can't find pic: %s\n", name);
		return;
	}
	x2 = x + w;
	pdest = vid.buffer + y*vid.rowbytes;
	for (i=0 ; i<h ; i++, pdest += vid.rowbytes)
	{
		psrc = pic->pixels[0] + pic->width * ((i+y)&63);
		for (j=x ; j<x2 ; j++)
			pdest[j] = psrc[j&63];
	}
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	byte			*dest;
	int				u, v;

	if (x+w > vid.width)
		w = vid.width - x;
	if (y+h > vid.height)
		h = vid.height - y;
	if (x < 0)
	{
		w += x;
		x = 0;
	}
	if (y < 0)
	{
		h += y;
		y = 0;
	}
	if (w < 0 || h < 0)
		return;
	dest = vid.buffer + y*vid.rowbytes + x;
	for (v=0 ; v<h ; v++, dest += vid.rowbytes)
		for (u=0 ; u<w ; u++)
			dest[u] = c;
}
//=============================================================================

/*
================
Draw_Fade

================
*/
void Draw_Fade (int x, int y, int w, int h, int c)
{
	int		x1, x2, y1, y2;
	byte	*pbuf;
	int		t;

	x2 = x + w;
	y2 = y + h;
	for (y1 = y; y1 < y2; y1++)
	{
		pbuf = (byte *)(vid.buffer + vid.rowbytes * y1);
		t = (y1 & 1) << 1;

		for (x1 = x; x1 < x2; x1++)
		{
			if ((x1 & 3) != t)
				pbuf[x1] = c;
		}
	}
}
