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
#ifndef __QMENU_H__
#define __QMENU_H__

// do not display menus when in console-only mode
#define MENU_CHECK	if (*re.flags & REF_CONSOLE_ONLY) return;


#define MENU_SCROLL_BORDER	32
#define MENU_SCROLL_AHEAD	3

typedef enum {
	MTYPE_SLIDER,
	MTYPE_LIST,
	MTYPE_ACTION,
	MTYPE_SPINCONTROL,		// uses array of *char
	MTYPE_SPINCONTROL2,		// uses *CStringItem list
	MTYPE_SEPARATOR,
	MTYPE_FIELD
} menuItemType_t;

#define QMF_LEFT_JUSTIFY	0x00000001
#define QMF_GRAYED			0x00000002
#define QMF_NUMBERSONLY		0x00000004
#define QMF_CENTER			0x00000008

struct menuCommon_s;

typedef struct menuFramework_s
{
	int		x, y;
	int		cursor;

	int		nitems;
	struct menuCommon_s *itemList;

	const char *statusbar;

	void (*cursordraw) (struct menuFramework_s *m);
} menuFramework_t;

typedef struct menuCommon_s
{
	menuItemType_t type;
	struct menuCommon_s *next;
	char	*name;
	int		x, y;
	menuFramework_t *parent;
	int		cursor_offset;
	int		localdata[4];
	int		flags;

	char	*statusbar;

	void (*callback) (void *self);
	void (*ownerdraw) (void *self);
	void (*cursordraw) (void *self);
} menuCommon_t;

typedef struct
{
	menuCommon_t generic;

	char	buffer[80];
	int		cursor;
	int		length;
	int		visible_length;
	int		visible_offset;
} menuField_t;

typedef struct
{
	menuCommon_t generic;

	float	minvalue;
	float	maxvalue;
	float	curvalue;
	float	range;
} menuSlider_t;

typedef struct
{
	menuCommon_t generic;

	int		curvalue;
	const char	**itemnames;
} menuList_t;

typedef struct
{
	menuCommon_t generic;

	int		curvalue;
	CStringItem *itemnames;
} menuList2_t;

typedef struct
{
	menuCommon_t generic;
} menuAction_t;

typedef struct
{
	menuCommon_t generic;
} menuSeparator_t;


bool	Field_Key (menuField_t *field, int key);

void	Menu_AddItem (menuFramework_t *menu, void *item);
void	Menu_AdjustCursor (menuFramework_t *menu, int dir);
void	Menu_Center (menuFramework_t *menu);
void	Menu_Draw (menuFramework_t *menu);
menuCommon_t *Menu_ItemAtCursor (menuFramework_t *m);
bool	Menu_SelectItem (menuFramework_t *s);
void	Menu_SetStatusBar (menuFramework_t *s, const char *string);
void	Menu_SlideItem (menuFramework_t *s, int dir);
int		Menu_TallySlots (menuFramework_t *menu);

#define Menu_DrawStringR2L(x,y,s)			DrawString(x-(appCStrlen(s)-1)*CHAR_WIDTH,y,s)
#define Menu_DrawStringCenter(x,y,s)		DrawString(x-(appCStrlen(s)-1)*CHAR_WIDTH/2,y,s)


#define MENU_ACTION(var,ypos,text,call)	\
	{									\
		var.generic.type = MTYPE_ACTION;\
		var.generic.x = 0;				\
		var.generic.y = ypos;			\
		var.generic.name = text;		\
		var.generic.callback = (void(*)(void*))call;\
	}

#define MENU_SLIDER(var,ypos,text,call,min,max)	\
	{									\
		var.generic.type = MTYPE_SLIDER;\
		var.generic.x = 0;				\
		var.generic.y = ypos;			\
		var.generic.name = text;		\
		var.generic.callback = call;	\
		var.minvalue = min;				\
		var.maxvalue = max;				\
	}

#define MENU_SPIN(var,ypos,text,call,items)	\
	{										\
		var.generic.type = MTYPE_SPINCONTROL;\
		var.generic.x = 0;				\
		var.generic.y = ypos;			\
		var.generic.name = text;		\
		var.generic.callback = call;	\
		var.itemnames = items;			\
	}

#endif
