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
#include <string.h>
#include <ctype.h>

#include "client.h"
#include "qmenu.h"

static void	 Action_DoEnter (menuAction_t *a);
static void	 Action_Draw (menuAction_t *a);
static void  Menu_DrawStatusBar (const char *string);
static void	 MenuList_Draw (menuList_t *l);
static void	 Separator_Draw (menuSeparator_t *s);
static void	 Slider_DoSlide (menuSlider_t *s, int dir);
static void	 Slider_Draw (menuSlider_t *s);
static void	 SpinControl_Draw (menuList_t *s);
static void	 SpinControl_DoSlide (menuList_t *s, int dir);
static void	 SpinControl2_Draw (menuList2_t *s);
static void	 SpinControl2_DoSlide (menuList2_t *s, int dir);

#define RCOLUMN_OFFSET  16
#define LCOLUMN_OFFSET -16

extern refExport_t re;
extern viddef_t viddef;

#define VID_WIDTH viddef.width
#define VID_HEIGHT viddef.height

#define Draw_Char(x,y,c) re.DrawCharColor(x,y,c,7)
#define Draw_Fill re.DrawFill

void Action_DoEnter (menuAction_t *a)
{
	if (a->generic.callback)
		a->generic.callback( a );
}

void Action_Draw (menuAction_t *a)
{
	if (a->generic.flags & QMF_LEFT_JUSTIFY)
	{
		if (a->generic.flags & QMF_GRAYED)
			Menu_DrawStringDark (a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name);
		else
			Menu_DrawString (a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name);
	}
	else
	{
		if (a->generic.flags & QMF_GRAYED)
			Menu_DrawStringR2LDark (a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name);
		else
			Menu_DrawStringR2L (a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name);
	}
	if (a->generic.ownerdraw)
		a->generic.ownerdraw( a );
}

qboolean Field_DoEnter (menuField_t *f)
{
	if (f->generic.callback)
	{
		f->generic.callback( f );
		return true;
	}
	return false;
}

// Draws uncolorized string
void Menu_DrawStringCommon (int x, int y, const char *string, int shift);

void Field_Draw (menuField_t *f)
{
	int i;
	char tempbuffer[128]="";

	if (f->generic.name)
		Menu_DrawStringR2LDark (f->generic.x + f->generic.parent->x + LCOLUMN_OFFSET, f->generic.y + f->generic.parent->y, f->generic.name);

	strncpy (tempbuffer, f->buffer + f->visible_offset, f->visible_length);

	Draw_Char (f->generic.x + f->generic.parent->x + 16, f->generic.y + f->generic.parent->y - 4, 18);
	Draw_Char (f->generic.x + f->generic.parent->x + 16, f->generic.y + f->generic.parent->y + 4, 24);

	Draw_Char (f->generic.x + f->generic.parent->x + 24 + f->visible_length * 8, f->generic.y + f->generic.parent->y - 4, 20);
	Draw_Char (f->generic.x + f->generic.parent->x + 24 + f->visible_length * 8, f->generic.y + f->generic.parent->y + 4, 26);

	for ( i = 0; i < f->visible_length; i++ )
	{
		Draw_Char (f->generic.x + f->generic.parent->x + 24 + i * 8, f->generic.y + f->generic.parent->y - 4, 19);
		Draw_Char (f->generic.x + f->generic.parent->x + 24 + i * 8, f->generic.y + f->generic.parent->y + 4, 25);
	}

	// perform string drawing with Menu_DrawStringConmmon() to disable text coloring
	Menu_DrawStringCommon (f->generic.x + f->generic.parent->x + 24, f->generic.y + f->generic.parent->y, tempbuffer, 0);

	if ( Menu_ItemAtCursor (f->generic.parent ) == f)
	{
		int offset;

		if (f->visible_offset)
			offset = f->visible_length;
		else
			offset = f->cursor;

		if ( ( ( int ) ( Sys_Milliseconds() / 250 ) ) & 1 )
		{
			Draw_Char (f->generic.x + f->generic.parent->x + ( offset + 2) * 8 + 8,
					   f->generic.y + f->generic.parent->y,
					   11 );
		}
		else
		{
			Draw_Char (f->generic.x + f->generic.parent->x + ( offset + 2) * 8 + 8,
					   f->generic.y + f->generic.parent->y,
					   ' ' );
		}
	}
}

qboolean Field_Key (menuField_t *f, int key)
{
	extern int keydown[];

	switch (key)
	{
	case K_KP_SLASH:
		key = '/';
		break;
	case K_KP_MINUS:
		key = '-';
		break;
	case K_KP_PLUS:
		key = '+';
		break;
	case K_KP_HOME:
		key = '7';
		break;
	case K_KP_UPARROW:
		key = '8';
		break;
	case K_KP_PGUP:
		key = '9';
		break;
	case K_KP_LEFTARROW:
		key = '4';
		break;
	case K_KP_5:
		key = '5';
		break;
	case K_KP_RIGHTARROW:
		key = '6';
		break;
	case K_KP_END:
		key = '1';
		break;
	case K_KP_DOWNARROW:
		key = '2';
		break;
	case K_KP_PGDN:
		key = '3';
		break;
	case K_KP_INS:
		key = '0';
		break;
	case K_KP_DEL:
		key = '.';
		break;
	}

	if (key > 127)
	{
		switch (key)
		{
		case K_DEL:
		default:
			return false;
		}
	}

	/*-------- support pasting from the clipboard ------------*/
	if ( ( toupper (key) == 'V' && keydown[K_CTRL] ) ||
		 ( ( ( key == K_INS ) || ( key == K_KP_INS ) ) && keydown[K_SHIFT] ) )
	{
		char *cbd;

		if ( ( cbd = Sys_GetClipboardData() ) != 0 )
		{
			strtok( cbd, "\n\r\b" );

			strncpy (f->buffer, cbd, f->length - 1);
			f->cursor = strlen (f->buffer);
			f->visible_offset = f->cursor - f->visible_length;
			if (f->visible_offset < 0)
				f->visible_offset = 0;

			free (cbd);
		}
		return true;
	}

	switch (key)
	{
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
	case K_BACKSPACE:
		if (f->cursor > 0)
		{
			memmove (&f->buffer[f->cursor-1], &f->buffer[f->cursor], strlen (&f->buffer[f->cursor]) + 1);
			f->cursor--;

			if (f->visible_offset)
			{
				f->visible_offset--;
			}
		}
		break;

	case K_KP_DEL:
	case K_DEL:
		memmove (&f->buffer[f->cursor], &f->buffer[f->cursor+1], strlen (&f->buffer[f->cursor+1]) + 1);
		break;

	case K_KP_ENTER:
	case K_ENTER:
	case K_ESCAPE:
	case K_TAB:
		return false;

	case K_SPACE:
	default:
		if (!isdigit (key) && (f->generic.flags & QMF_NUMBERSONLY))
			return false;

		if (f->cursor < f->length)
		{
			f->buffer[f->cursor++] = key;
			f->buffer[f->cursor] = 0;

			if (f->cursor > f->visible_length)
			{
				f->visible_offset++;
			}
		}
	}

	return true;
}

void Menu_AddItem (menuFramework_t *menu, void *item)
{
	if (menu->nitems == 0)
		menu->nslots = 0;

	if (menu->nitems < MAXMENUITEMS)
	{
		menu->items[menu->nitems] = item;
		( ( menuCommon_t * ) menu->items[menu->nitems] )->parent = menu;
		menu->nitems++;
	}

	menu->nslots = Menu_TallySlots (menu);
}


static qboolean Menu_CompletelyVisible (menuFramework_t *menu)
{
	int		y0, y1;

	y0 = ((menuCommon_t*)menu->items[0])->y + menu->y;
	y1 = ((menuCommon_t*)menu->items[menu->nitems - 1])->y + menu->y;
	if (y0 >= 0 && y1 < VID_HEIGHT - 10) return true;
	return false;
}


static qboolean Menu_ItemVisible (menuFramework_t *menu, int i)
{
	int		y;

	if (Menu_CompletelyVisible (menu))
		return true;

	if (menu->y > MENU_SCROLL_BORDER)
		return true;

	if (i < 0 || i >= menu->nitems)
		return false;
	y = ((menuCommon_t*)menu->items[i])->y + menu->y;
	return (y >= MENU_SCROLL_BORDER && y <= VID_HEIGHT - MENU_SCROLL_BORDER);
}


static void Menu_MakeVisible (menuFramework_t *menu, int i)
{
	int		y, y0, y1;

	y0 = ((menuCommon_t*)menu->items[0])->y;
	y1 = ((menuCommon_t*)menu->items[menu->nitems - 1])->y;
#if 1
	if (Menu_CompletelyVisible (menu)) return;
#else
	int		h;

	h = y1 - y0;
	if (h < VID_HEIGHT - 2 * MENU_SCROLL_BORDER)
		return;		// menu must be totally visible
#endif

	if (y0 + menu->y > MENU_SCROLL_BORDER)
	{
		menu->y = MENU_SCROLL_BORDER - y0;
		return;
	}
	if (y1 + menu->y < VID_HEIGHT - MENU_SCROLL_BORDER)
	{
		menu->y = VID_HEIGHT - MENU_SCROLL_BORDER - y1;
		return;
	}

	if (i < 0)
		i = 0;
	else if (i >= menu->nitems)
		i = menu->nitems - 1;

	y = ((menuCommon_t*)menu->items[i])->y + menu->y;

	if (y < MENU_SCROLL_BORDER)
		menu->y += MENU_SCROLL_BORDER - y;
	else if (y > VID_HEIGHT - MENU_SCROLL_BORDER)
		menu->y += VID_HEIGHT - MENU_SCROLL_BORDER - y;
}


/*
 * Menu_AdjustCursor
 *
 * This function takes the given menu, the direction, and attempts
 * to adjust the menu's cursor so that it's at the next available
 * slot.
 */
void Menu_AdjustCursor (menuFramework_t *m, int dir)
{
	menuCommon_t *citem;

	/* it's not in a valid spot, so crawl in the direction indicated until we
	 * find a valid spot
	 */
	if (dir == 1)
	{
		while (1)
		{
			citem = Menu_ItemAtCursor (m);
			if (citem)
				if (citem->type != MTYPE_SEPARATOR)
					break;
			m->cursor += dir;
			if (m->cursor >= m->nitems)
				// m->cursor = 0; -- will wrap cursor
				dir = -1;
		}
	}
	else
	{
		while (1)
		{
			citem = Menu_ItemAtCursor (m);
			if (citem)
				if (citem->type != MTYPE_SEPARATOR)
					break;
			m->cursor += dir;
			if (m->cursor < 0)
				// m->cursor = m->nitems - 1; -- will wrap cursor
				dir = 1;
		}
	}

	Menu_MakeVisible (m, m->cursor - MENU_SCROLL_AHEAD);
	Menu_MakeVisible (m, m->cursor + MENU_SCROLL_AHEAD);
}


void Menu_Center (menuFramework_t *menu)
{
	int		height;

	height = ((menuCommon_t*) menu->items[menu->nitems-1])->y;
	height += 10;

	menu->y = (VID_HEIGHT - height) / 2;
	if (menu->y < MENU_SCROLL_BORDER && !Menu_CompletelyVisible (menu))
		menu->y = MENU_SCROLL_BORDER;
}


void Menu_DrawDotsItem (menuCommon_t *item)
{
	int		x, y, center;

	center = VID_WIDTH / 2;
	y = item->y + item->parent->y;
	for (x = center - 128; x < center + 128; x += 8)
		re.DrawCharColor (x, y, 0, 1);
}


static int strlen_color (const char *s)
{
	int		i, c;

	i = 0;
	while (c = *s++)
	{
		if (c == '^')
		{
			if (*s >= '0' && *s <= '7')
			{
				s++;
				continue;
			}
		}
		i++;
	}

	return i;
}


void Menu_Draw (menuFramework_t *menu)
{
	int		i, vis;
	menuCommon_t *item;

	/*------- draw contents -------*/
	vis = -1;
	for (i = 0; i < menu->nitems; i++)
	{
		if (Menu_ItemVisible (menu, i))
		{
			if (vis == -1)	// previous item vas invisible
			{
				vis = 0;
				if (i > 0)	// it is not a first item
				{
					Menu_DrawDotsItem ((menuCommon_t*)menu->items[i]);
					continue;
				}
			}
			else if (i < menu->nitems - 1 && !Menu_ItemVisible (menu, i + 1))
			{
				Menu_DrawDotsItem ((menuCommon_t*)menu->items[i]);
				break;	// no more visible items
			}
		}
		else
			continue;

		switch (((menuCommon_t*)menu->items[i])->type)
		{
		case MTYPE_FIELD:
			Field_Draw ((menuField_t*)menu->items[i]);
			break;
		case MTYPE_SLIDER:
			Slider_Draw ((menuSlider_t*) menu->items[i]);
			break;
		case MTYPE_LIST:
			MenuList_Draw ((menuList_t*) menu->items[i]);
			break;
		case MTYPE_SPINCONTROL:
			SpinControl_Draw ((menuList_t*) menu->items[i]);
			break;
		case MTYPE_SPINCONTROL2:
			SpinControl2_Draw ((menuList2_t*) menu->items[i]);
			break;
		case MTYPE_ACTION:
			Action_Draw ((menuAction_t*)menu->items[i]);
			break;
		case MTYPE_SEPARATOR:
			Separator_Draw ((menuSeparator_t*)menu->items[i]);
			break;
		}
	}

	item = Menu_ItemAtCursor (menu);

	if (item && item->cursordraw)
	{
		item->cursordraw (item);
	}
	else if (menu->cursordraw)
	{
		menu->cursordraw (menu);
	}
	else if (item && item->type != MTYPE_FIELD)
	{
		if (item->flags & QMF_LEFT_JUSTIFY)
		{
			Draw_Char (menu->x + item->x - 24 + item->cursor_offset, menu->y + item->y, 12 + ((int)(Sys_Milliseconds()/250) & 1));
		}
		else
		{
			Draw_Char (menu->x + item->cursor_offset, menu->y + item->y, 12 + ((int)(Sys_Milliseconds()/250) & 1));
		}
	}

	if (item)
	{
		if (item->statusbarfunc)
			item->statusbarfunc((void*)item);
		else if (item->statusbar)
			Menu_DrawStatusBar (item->statusbar);
		else
			Menu_DrawStatusBar (menu->statusbar);

	}
	else
		Menu_DrawStatusBar (menu->statusbar);
}

void Menu_DrawStatusBar (const char *string)
{
	if (string)
	{
		int		l, maxrow, maxcol, col;

		l = strlen_color (string);
		maxrow = VID_HEIGHT / 8;
		maxcol = VID_WIDTH / 8;
		col = maxcol / 2 - l / 2;

		Draw_Fill (0, VID_HEIGHT-8, VID_WIDTH, 8, 4);
		Menu_DrawString (col*8, VID_HEIGHT - 8, string);
	}
	else
		Draw_Fill (0, VID_HEIGHT-8, VID_WIDTH, 8, 0);
}

void Menu_DrawStringCommon (int x, int y, const char *string, int shift)
{
	char	c;
	const char *s;

	s = string;
	while (c = *s++)
	{
		Draw_Char (x, y, c + shift);
		x += 8;
	}
}

void *Menu_ItemAtCursor (menuFramework_t *m)
{
	if (m->cursor < 0 || m->cursor >= m->nitems)
		return 0;

	return m->items[m->cursor];
}

qboolean Menu_SelectItem (menuFramework_t *s)
{
	menuCommon_t *item = ( menuCommon_t * ) Menu_ItemAtCursor( s );

	if (item)
	{
		switch (item->type)
		{
		case MTYPE_FIELD:
			return Field_DoEnter( ( menuField_t * ) item ) ;
		case MTYPE_ACTION:
			Action_DoEnter( ( menuAction_t * ) item );
			return true;
//		case MTYPE_LIST:
//		case MTYPE_SPINCONTROL:
//		case MTYPE_SPINCONTROL2:
//			return false;
		}
	}
	return false;
}

void Menu_SetStatusBar (menuFramework_t *m, const char *string)
{
	m->statusbar = string;
}

void Menu_SlideItem (menuFramework_t *s, int dir)
{
	menuCommon_t *item = (menuCommon_t *) Menu_ItemAtCursor (s);

	if (item)
	{
		switch (item->type)
		{
		case MTYPE_SLIDER:
			Slider_DoSlide ((menuSlider_t *) item, dir);
			break;
		case MTYPE_SPINCONTROL:
			SpinControl_DoSlide ((menuList_t *) item, dir);
			break;
		case MTYPE_SPINCONTROL2:
			SpinControl2_DoSlide ((menuList2_t *) item, dir);
			break;
		}
	}
}

int Menu_TallySlots (menuFramework_t *menu)
{
	int i;
	int total = 0;

	for ( i = 0; i < menu->nitems; i++ )
	{
		if ( ( ( menuCommon_t * ) menu->items[i] )->type == MTYPE_LIST )
		{
			int nitems = 0;
			const char **n = ( ( menuList_t * ) menu->items[i] )->itemnames;

			while (*n)
				nitems++, n++;

			total += nitems;
		}
		else
		{
			total++;
		}
	}

	return total;
}


static void DrawCaption (menuCommon_t *m)
{
	if (m->name)
		Menu_DrawStringR2LDark (m->x + m->parent->x + LCOLUMN_OFFSET, m->y + m->parent->y, m->name);
}

void MenuList_Draw (menuList_t *l)
{
	const char **n;
	int y = 0;

	DrawCaption (&l->generic);

	n = l->itemnames;

  	Draw_Fill (l->generic.x - 112 + l->generic.parent->x, l->generic.parent->y + l->generic.y + l->curvalue*10 + 10, 128, 10, 16);
	while ( *n )
	{
		Menu_DrawStringR2LDark (l->generic.x + l->generic.parent->x + LCOLUMN_OFFSET, l->generic.y + l->generic.parent->y + y + 10, *n);

		n++;
		y += 10;
	}
}

void Separator_Draw (menuSeparator_t *s)
{
	int x, y;
	const char *name;

	x = s->generic.x + s->generic.parent->x;
	y = s->generic.y + s->generic.parent->y;
	name = s->generic.name;
	if ( !name ) return;
	if (s->generic.flags & QMF_CENTER)
		Menu_DrawStringCenterDark (x, y, name);
	else
		Menu_DrawStringR2LDark (x, y, name);
}

void Slider_DoSlide (menuSlider_t *s, int dir)
{
	s->curvalue += dir;

	if (s->curvalue > s->maxvalue)
		s->curvalue = s->maxvalue;
	else if (s->curvalue < s->minvalue)
		s->curvalue = s->minvalue;

	if (s->generic.callback)
		s->generic.callback( s );
}

#define SLIDER_RANGE 10

void Slider_Draw (menuSlider_t *s)
{
	int	i;

	DrawCaption (&s->generic);

	s->range = (s->curvalue - s->minvalue) / (float)(s->maxvalue - s->minvalue);

	if (s->range < 0)
		s->range = 0;
	if (s->range > 1)
		s->range = 1;
	Draw_Char (s->generic.x + s->generic.parent->x + RCOLUMN_OFFSET, s->generic.y + s->generic.parent->y, 128);
	for (i = 0; i < SLIDER_RANGE; i++)
		Draw_Char (RCOLUMN_OFFSET + s->generic.x + i*8 + s->generic.parent->x + 8, s->generic.y + s->generic.parent->y, 129);
	Draw_Char (RCOLUMN_OFFSET + s->generic.x + i*8 + s->generic.parent->x + 8, s->generic.y + s->generic.parent->y, 130);
	Draw_Char ((int)(8 + RCOLUMN_OFFSET + s->generic.parent->x + s->generic.x + (SLIDER_RANGE-1)*8 * s->range), s->generic.y + s->generic.parent->y, 131);
}


void SpinControl_DoSlide (menuList_t *s, int dir)
{
	s->curvalue += dir;

	if (s->curvalue < 0)
		s->curvalue = 0;
	else if (s->itemnames[s->curvalue] == 0)
		s->curvalue--;

	if (s->generic.callback)
		s->generic.callback (s);
}

void SpinControl_Draw (menuList_t *s)
{
	int		maxIndex;
	const char	*text;
	char	*newline;

	DrawCaption (&s->generic);

	// check for valid index
	maxIndex = 0;
	while (s->itemnames[maxIndex]) maxIndex++;
	maxIndex--;
	if (s->curvalue > maxIndex)
		s->curvalue = maxIndex;
	// draw value
	text = s->itemnames[s->curvalue];
	if (!(newline = strchr (text, '\n')))
		Menu_DrawString (RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y, text);
	else
	{
		char	buffer[256];

		Q_strncpyz (buffer, text, newline - text + 1);	// copy text till '\n' and put zero to its place
		Menu_DrawString (RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y, buffer);
		strcpy (buffer, newline + 1);
		Menu_DrawString (RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y + 10, buffer);
	}
}


void SpinControl2_DoSlide (menuList2_t *s, int dir)
{
	s->curvalue += dir;

	if (s->curvalue < 0)
		s->curvalue = 0;
	else
	{
		basenamed_t *item;
		int		i;

		for (i = 0, item = s->itemnames; item && i < s->curvalue; item = item->next, i++) ;
		if (!item) i--;
		s->curvalue = i;
	}

	if (s->generic.callback)
		s->generic.callback (s);
}

void SpinControl2_Draw (menuList2_t *s)
{
	int		i;
	basenamed_t *item, *last;
	const char	*text;
	char	*newline;

	DrawCaption (&s->generic);

	// check for valid index
	last = NULL;
	for (i = 0, item = s->itemnames; item && i < s->curvalue; item = item->next, i++) last = item;
	if (!item)	// out of list
	{
		if (!last) return;		// empty list?
		s->curvalue = i - 1;
		item = last;
	}
	// draw value
	text = item->name;
	if (!(newline = strchr (text, '\n')))
		Menu_DrawString (RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y, text);
	else
	{
		char	buffer[256];

		Q_strncpyz (buffer, text, newline - text + 1);	// copy text till '\n' and put zero to its place
		Menu_DrawString (RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y, buffer);
		strcpy (buffer, newline + 1);
		Menu_DrawString (RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y + 10, buffer);
	}
}
