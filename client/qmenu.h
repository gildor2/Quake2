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


#define MENU_SCROLL_BORDER	32
#define MENU_SCROLL_AHEAD	3

enum menuItemType_t
{
	MTYPE_SLIDER,
	MTYPE_LIST,
	MTYPE_ACTION,
	MTYPE_SPINCONTROL,		// uses array of *char
	MTYPE_SPINCONTROL2,		// uses *CStringItem list
	MTYPE_SEPARATOR,
	MTYPE_FIELD
};

#define QMF_LEFT_JUSTIFY	0x00000001
#define QMF_GRAYED			0x00000002
#define QMF_NUMBERSONLY		0x00000004
#define QMF_CENTER			0x00000008

// forward
struct menuCommon_t;

struct menuFramework_t
{
	short	x, y;
	short	cursor;

	short	nitems;
	menuCommon_t *itemList;

	const char *statusbar;
	const char *banner;

	void (*cursordraw) (menuFramework_t *m);

	void	AddItem (menuCommon_t *item);
	void	AdjustCursor (int dir);
	void	Center ();
	menuCommon_t *ItemAtCursor ();
	bool	SelectItem ();
	void	SetStatusBar (const char *string);
	void	SlideItem (int dir);
	int		TallySlots ();
	// menu opening/closing
	void	Push ();
	void	Pop ();
	// virtual functions
	virtual bool Init ();
	virtual void Draw ();
	virtual const char * KeyDown (int key);
};

struct menuCommon_t
{
	menuItemType_t type;
	menuCommon_t *next;
	char	*name;
	short	x, y;
	menuFramework_t *parent;
	short	cursor_offset;
	int		localData;
	byte	flags;

	char	*statusbar;

	void (*callback) (void *self);
	void (*ownerdraw) (void *self);
	void (*cursordraw) (void *self);
};

struct menuField_t : menuCommon_t
{
	char	buffer[80];
	short	cursor;
	short	length;
	short	visible_length;
	short	visible_offset;
};

struct menuSlider_t : menuCommon_t
{
	float	minvalue;
	float	maxvalue;
	float	curvalue;
	float	range;
};

struct menuList_t : menuCommon_t
{
	short	curvalue;
	const char	**itemnames;
};

struct menuList2_t : menuCommon_t
{
	short	curvalue;
	CStringItem *itemnames;
};

struct menuAction_t : menuCommon_t
{
	char	*command;
};

struct menuSeparator_t : menuCommon_t
{
};


extern const char *menu_in_sound;
extern const char *menu_move_sound;
extern const char *menu_out_sound;


#define Menu_DrawStringR2L(x,y,s)			DrawString(x-(appCStrlen(s)-1)*CHAR_WIDTH,y,s)
#define Menu_DrawStringCenter(x,y,s)		DrawString(x-(appCStrlen(s)-1)*CHAR_WIDTH/2,y,s)


#define MENU_ACTION(var,ypos,text,call,cmd)	\
	{							\
		var.type = MTYPE_ACTION;\
		var.x = 0;				\
		var.y = ypos;			\
		var.name = text;		\
		var.callback = (void(*)(void*))call;\
		var.command = cmd;		\
	}

#define MENU_SLIDER(var,ypos,text,call,min,max)	\
	{							\
		var.type = MTYPE_SLIDER;\
		var.x = 0;				\
		var.y = ypos;			\
		var.name = text;		\
		var.callback = call;	\
		var.minvalue = min;		\
		var.maxvalue = max;		\
	}

#define MENU_SPIN(var,ypos,text,call,items)	\
	{							\
		var.type = MTYPE_SPINCONTROL;\
		var.x = 0;				\
		var.y = ypos;			\
		var.name = text;		\
		var.callback = call;	\
		var.itemnames = items;	\
	}

#endif
