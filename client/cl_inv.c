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
// cl_inv.c -- client inventory screen

#include "client.h"

/*
================
CL_ParseInventory
================
*/
void CL_ParseInventory (void)
{
	int		i;

	for (i=0 ; i<MAX_ITEMS ; i++)
		cl.inventory[i] = MSG_ReadShort (&net_message);
}


/*
================
Inv_DrawString
================
*/
void Inv_DrawString (int x, int y, char *string, qboolean hgl)
{
	char	c;
	int		color;

	while (c = *string++)
	{
		color = hgl ? 2 : 7;
		re_DrawCharColor (x, y, c, color);
		x += 8;
	}
}

/*
================
CL_DrawInventory
================
*/
#define	DISPLAY_ITEMS	17

void CL_DrawInventory (void)
{
	int		i, num, selected_num, item;
	int		index[MAX_ITEMS];
	char	string[1024];
	int		x, y;
	int		selected, top;

	selected = cl.frame.playerstate.stats[STAT_SELECTED_ITEM];

	num = 0;
	selected_num = 0;
	for (i = 0; i < MAX_ITEMS; i++)
	{
		if (i == selected)
			selected_num = num;
		if (cl.inventory[i])
		{
			index[num] = i;
			num++;
		}
	}

	// determine scroll point
	top = selected_num - DISPLAY_ITEMS/2;
	if (num - top < DISPLAY_ITEMS)
		top = num - DISPLAY_ITEMS;
	if (top < 0)
		top = 0;

	x = (viddef.width-256)/2;
	y = (viddef.height-240)/2;

	re_DrawPic (x, y+8, "inventory");

	y += 24;
	x += 24;
	Inv_DrawString (x, y, "hotkey ### item", true);
	Inv_DrawString (x, y+8, "------ --- ----", true);
	y += 16;
	for (i=top ; i<num && i < top+DISPLAY_ITEMS ; i++)
	{
		int		key;
		char	binding[256];
		const char *keyName;

		item = index[i];
		Com_sprintf (ARRAY_ARG(binding), "use %s", cl.configstrings[CS_ITEMS+item]);
		if (Key_FindBinding (binding, &key, 1))
			keyName = Key_KeynumToString (key);
		else
			keyName = "";

		Com_sprintf (ARRAY_ARG(string), "%6s %3i %s", keyName, cl.inventory[item], cl.configstrings[CS_ITEMS+item]);
		if (item == selected)
			re_DrawChar (x-8, y, 13);	//?? original: 15 (but not displayed) anyway

		Inv_DrawString (x, y, string, item == selected);
		y += 8;
	}
}
