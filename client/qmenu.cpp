#include "client.h"
#include "qmenu.h"

#define CHAR_WIDTH	8
#define CHAR_HEIGHT	8

static void	 Action_Draw(menuAction_t *a);
static void	 Action_DoEnter(menuAction_t *a);
static void	 MenuList_Draw(menuList_t *l);
static void	 Separator_Draw(menuSeparator_t *s);
static void	 Slider_Draw(menuSlider_t *s);
static void	 Slider_DoSlide(menuSlider_t *s, int dir);
static void	 SpinControl_Draw(menuList_t *s);
static void	 SpinControl_DoSlide(menuList_t *s, int dir);
static void	 SpinControl2_Draw(menuList2_t *s);
static void	 SpinControl2_DoSlide(menuList2_t *s, int dir);

#define RCOLUMN_OFFSET  16
#define LCOLUMN_OFFSET -16


#define VID_WIDTH		viddef.width
#define VID_HEIGHT		viddef.height


static menuCommon_t *GetItem(menuFramework_t *menu, int index)
{
	int		i;
	menuCommon_t *item;

	for (i = 0, item = menu->itemList; i < index && item; i++, item = item->next) ;

	if (!item)
		appError("GetMenuItem: item index %d is out of bounds %d\n", index, menu->nitems);

	return item;
}

menuCommon_t * menuFramework_t::ItemAtCursor()
{
	if (cursor < 0 || cursor >= nitems)
		return NULL;

	return GetItem(this, cursor);
}

static void Action_DoEnter(menuAction_t *a)
{
	if (a->command) Cbuf_AddText(a->command);
	if (a->callback) a->callback(a);
}

static void Action_Draw(menuAction_t *a)
{
	if (a->flags & QMF_LEFT_JUSTIFY)
		DrawString(a->x + a->parent->x + LCOLUMN_OFFSET, a->y + a->parent->y,
			va("%s%s", a->flags & QMF_GRAYED ? S_GREEN : "", a->name));
	else
		Menu_DrawStringR2L(a->x + a->parent->x + LCOLUMN_OFFSET, a->y + a->parent->y,
			va("%s%s", a->flags & QMF_GRAYED ? S_GREEN : "", a->name));
	if (a->ownerdraw)
		a->ownerdraw(a);
}

static bool Field_DoEnter(menuField_t *f)
{
	if (f->callback)
	{
		f->callback( f );
		return true;
	}
	return false;
}

static void Field_Draw(menuField_t *f)
{
	int		i;
	char	tempbuffer[128];

	// draw caption
	if (f->name)
		Menu_DrawStringR2L(f->x + f->parent->x + LCOLUMN_OFFSET, f->y + f->parent->y,
			va(S_GREEN"%s", f->name));

	int x = f->x + f->parent->x;
	int y = f->y + f->parent->y;

	// draw border
#if 1
	RE_DrawChar(x + CHAR_WIDTH * 2, y - CHAR_HEIGHT/2, 18);				// left
	RE_DrawChar(x + CHAR_WIDTH * 2, y + CHAR_HEIGHT/2, 24);
	RE_DrawChar(x + (f->visible_length + 3) * CHAR_WIDTH, y - CHAR_HEIGHT/2, 20);	// right
	RE_DrawChar(x + (f->visible_length + 3) * CHAR_WIDTH, y + CHAR_HEIGHT/2, 26);
	for (i = 0; i < f->visible_length; i++)
	{
		RE_DrawChar(x + (i + 3) * CHAR_WIDTH, y - CHAR_HEIGHT/2, 19);	// top
		RE_DrawChar(x + (i + 3) * CHAR_WIDTH, y + CHAR_HEIGHT/2, 25);	// bottom
	}
#else
	RE_Fill(x + CHAR_WIDTH * 2, y - CHAR_HEIGHT / 2, (f->visible_length + 2) * CHAR_WIDTH, CHAR_HEIGHT * 2, RGBA(1, 0, 0, 0.2));
#endif

	// perform string drawing without text coloring
	appStrncpyz(tempbuffer, f->buffer + f->visible_offset, max(f->visible_length, sizeof(tempbuffer)));
	const char *s = tempbuffer;
	int x1 = x + 3 * CHAR_WIDTH;
	while (char c = *s++)
	{
		RE_DrawChar(x1, y, c);
		x1 += CHAR_WIDTH;
	}

	if (f->parent->ItemAtCursor() == f)
	{
		int offset = (f->visible_offset) ? f->visible_length : f->cursor;
		// show cursor
		RE_DrawChar(x + (offset + 3) * CHAR_WIDTH, y, ((cls.realtime >> 8) & 1) ? 11 : ' ');
	}
}

static bool Field_Key(menuField_t *f, int key)
{
	// numpad keys -> ASCII
	static const char *kpString = KEYPAD_STRING;
	if (key >= KEYPAD_FIRST && key <= KEYPAD_LAST)
		key = kpString[key - KEYPAD_FIRST];

	switch (key)
	{
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
	case K_BACKSPACE:
		if (f->cursor > 0)
		{
			memcpy(&f->buffer[f->cursor-1], &f->buffer[f->cursor], strlen(&f->buffer[f->cursor]) + 1);
			f->cursor--;

			if (f->visible_offset)
				f->visible_offset--;
		}
		break;

	case K_KP_DEL:
	case K_DEL:
		memcpy(&f->buffer[f->cursor], &f->buffer[f->cursor+1], strlen(&f->buffer[f->cursor+1]) + 1);
		break;

	case K_KP_ENTER:
	case K_ENTER:
	case K_ESCAPE:
	case K_TAB:
		return false;

	default:
		if (key < 32 || key >= 128) return false;
		if ((f->flags & QMF_NUMBERSONLY) && (key < '0' || key > '9'))
			return false;

		if (f->cursor < f->length)
		{
			f->buffer[f->cursor++] = key;
			f->buffer[f->cursor] = 0;

			if (f->cursor > f->visible_length)
				f->visible_offset++;
		}
	}

	return true;
}

void menuFramework_t::AddItem(menuCommon_t *item)
{
	menuCommon_t *last, *p;
	int		i;

	guard(Menu_AddItem);
	// NOTE: menu without items will have nitems==0, but itemList will not be NULL !
	// find last item
	last = NULL;
	for (i = 0, p = itemList; i < nitems; i++, p = p->next)
	{
		if (p == item)				// item already present in list -- fatal (circular linked list)
			appError("double item \"%s\" in menu, index=%d, count=%d", item->name, i, nitems);
		last = p;
	}
	if (last && last->next)			// last item (with index == menu->nitem) have next != NULL
		appError("invalid item list");
	// add to list
	if (i > 0)	last->next = item;	// append to list tail
	else		itemList = item;	// first in list
	nitems++;
	// setup new item
	item->parent = this;
	item->next   = NULL;
	unguard;
}


static bool Menu_CompletelyVisible(menuFramework_t *menu)
{
	int y0 = menu->itemList->y + menu->y;					// 1st item
	int y1 = GetItem(menu, menu->nitems - 1)->y + menu->y;	// last item
	if (y0 >= 0 && y1 < VID_HEIGHT - 10) return true;
	return false;
}


static bool Menu_ItemVisible(menuFramework_t *menu, int i)
{
	if (Menu_CompletelyVisible(menu))
		return true;

	if (menu->y > MENU_SCROLL_BORDER)
		return true;

	if (i < 0 || i >= menu->nitems)
		return false;
	int y = GetItem(menu, i)->y + menu->y;
	return (y >= MENU_SCROLL_BORDER && y <= VID_HEIGHT - MENU_SCROLL_BORDER);
}


static void Menu_MakeVisible(menuFramework_t *menu, int i)
{
	if (Menu_CompletelyVisible(menu)) return;

	int y0 = menu->itemList->y;						// 1st item
	int y1 = GetItem(menu, menu->nitems - 1)->y;	// last item
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

	i = bound(i, 0, menu->nitems-1);
	int y = GetItem(menu, i)->y + menu->y;

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
void menuFramework_t::AdjustCursor(int dir)
{
	menuCommon_t *citem;

	if (!nitems) return;

	/* it's not in a valid spot, so crawl in the direction indicated until we
	 * find a valid spot
	 */
	if (dir == 1)
	{
		while (true)
		{
			citem = ItemAtCursor();
			if (citem && citem->type != MTYPE_SEPARATOR)
				break;
			cursor += dir;		// may be changed
			if (cursor >= nitems)
				// cursor = 0; -- will wrap cursor
				dir = -1;
		}
	}
	else
	{
		while (true)
		{
			citem = ItemAtCursor();
			if (citem && citem->type != MTYPE_SEPARATOR)
				break;
			cursor += dir;
			if (cursor < 0)
				// cursor = nitems - 1; -- will wrap cursor
				dir = 1;
		}
	}

	Menu_MakeVisible(this, cursor - MENU_SCROLL_AHEAD);
	Menu_MakeVisible(this, cursor + MENU_SCROLL_AHEAD);
}


void menuFramework_t::Center()
{
	int height = GetItem(this, nitems-1)->y + 10;	// last item
	y = (VID_HEIGHT - height) / 2;
	if (y < MENU_SCROLL_BORDER && !Menu_CompletelyVisible(this))
		y = MENU_SCROLL_BORDER;
}


static void Menu_DrawDotsItem(menuCommon_t *item)
{
	int center = VID_WIDTH / 2;
	int y = item->y + item->parent->y;
	for (int x = center - 128; x < center + 128; x += CHAR_WIDTH)
		RE_DrawChar(x, y, 0, C_RED);
}


void menuFramework_t::Draw()
{
	int		i;
	menuCommon_t *item;

	/*------- draw banner ---------*/
	if (banner)
		RE_DrawPic(viddef.width / 2, viddef.height / 2 - 110, va("pics/m_banner_%s", banner), ANCHOR_TOP);

	/*------- draw contents -------*/
	int vis = -1;
	for (i = 0, item = itemList; i < nitems; i++, item = item->next)
	{
		if (Menu_ItemVisible(this, i))
		{
			if (vis == -1)	// previous item vas invisible
			{
				vis = 0;
				if (i > 0)	// it is not a first item
				{
					Menu_DrawDotsItem(item);
					continue;
				}
			}
			else if (i < nitems - 1 && !Menu_ItemVisible(this, i + 1))
			{
				Menu_DrawDotsItem(item);
				break;	// no more visible items
			}
		}
		else
			continue;

		switch (item->type)
		{
		case MTYPE_FIELD:
			Field_Draw((menuField_t*)item);
			break;
		case MTYPE_SLIDER:
			Slider_Draw((menuSlider_t*)item);
			break;
		case MTYPE_LIST:
			MenuList_Draw((menuList_t*)item);
			break;
		case MTYPE_SPINCONTROL:
			SpinControl_Draw((menuList_t*)item);
			break;
		case MTYPE_SPINCONTROL2:
			SpinControl2_Draw((menuList2_t*)item);
			break;
		case MTYPE_ACTION:
			Action_Draw((menuAction_t*)item);
			break;
		case MTYPE_SEPARATOR:
			Separator_Draw((menuSeparator_t*)item);
			break;
		}
	}

	item = ItemAtCursor();

	if (item && item->cursordraw)
		item->cursordraw(item);
	else if (cursordraw)
		cursordraw(this);
	else if (item && item->type != MTYPE_FIELD)
	{
		if (item->flags & QMF_LEFT_JUSTIFY)
			RE_DrawChar(x + item->x - 3*CHAR_WIDTH + item->cursor_offset,
				y + item->y, 12 + ((cls.realtime >> 8) & 1));
		else
			RE_DrawChar(x + item->cursor_offset,
				y + item->y, 12 + ((cls.realtime >> 8) & 1));
	}

	// draw statusbar
	const char *string = (item && item->statusbar) ? item->statusbar : statusbar;
	if (string)
	{
		RE_Fill8(0, VID_HEIGHT - CHAR_HEIGHT, VID_WIDTH, CHAR_HEIGHT, 4);
		DrawString((VID_WIDTH - appCStrlen(string) * CHAR_WIDTH) / 2, VID_HEIGHT - CHAR_WIDTH, string);
	}
}


bool menuFramework_t::SelectItem()
{
	menuCommon_t *item = ItemAtCursor();
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

void menuFramework_t::SetStatusBar(const char *string)
{
	statusbar = string;
}

void menuFramework_t::SlideItem(int dir)
{
	menuCommon_t *item = ItemAtCursor();
	if (item)
	{
		switch (item->type)
		{
		case MTYPE_SLIDER:
			Slider_DoSlide((menuSlider_t *) item, dir);
			break;
		case MTYPE_SPINCONTROL:
			SpinControl_DoSlide((menuList_t *) item, dir);
			break;
		case MTYPE_SPINCONTROL2:
			SpinControl2_DoSlide((menuList2_t *) item, dir);
			break;
		}
	}
}


static void DrawCaption(menuCommon_t *m)
{
	if (m->name)
		Menu_DrawStringR2L(m->x + m->parent->x + LCOLUMN_OFFSET, m->y + m->parent->y, va(S_GREEN"%s", m->name));
}

static void MenuList_Draw(menuList_t *l)
{
	DrawCaption(l);
  	RE_Fill8(l->x - 112 + l->parent->x, l->parent->y + l->y + l->curvalue*10 + 10, 128, CHAR_HEIGHT+2, 16);

	int y = 0;
	for (const char **n = l->itemnames; *n; n++)
	{
		Menu_DrawStringR2L(l->x + l->parent->x + LCOLUMN_OFFSET, l->y + l->parent->y + y + CHAR_HEIGHT+2, va(S_GREEN"%s", *n));
		y += CHAR_HEIGHT + 2;
	}
}

static void Separator_Draw(menuSeparator_t *s)
{
	const char *name = s->name;
	if (!name) return;

	int x = s->x + s->parent->x;
	int y = s->y + s->parent->y;

	if (s->flags & QMF_CENTER)
		Menu_DrawStringCenter(x, y, va(S_GREEN"%s", name));
	else
		Menu_DrawStringR2L(x, y, va(S_GREEN"%s", name));
}

static void Slider_DoSlide(menuSlider_t *s, int dir)
{
	s->curvalue += dir;
	s->curvalue = bound(s->curvalue, s->minvalue, s->maxvalue);
	if (s->callback)
		s->callback( s );
}

#define SLIDER_RANGE 10

static void Slider_Draw(menuSlider_t *s)
{
	DrawCaption(s);

	s->range = (s->curvalue - s->minvalue) / (float)(s->maxvalue - s->minvalue);
	s->range = bound(s->range, 0, 1);

	int x = s->x + s->parent->x;
	int y = s->y + s->parent->y;

	RE_DrawChar(x + RCOLUMN_OFFSET, y, 128);									// left
	for (int i = 0; i < SLIDER_RANGE; i++)
		RE_DrawChar(x + RCOLUMN_OFFSET + (i + 1) * CHAR_WIDTH, y, 129);			// middle
	RE_DrawChar(x + RCOLUMN_OFFSET + (SLIDER_RANGE + 1) * CHAR_WIDTH, y, 130);	// right
	RE_DrawChar(appRound(x + CHAR_WIDTH + RCOLUMN_OFFSET + (SLIDER_RANGE - 1) * CHAR_WIDTH * s->range), y, 131);
}


static void SpinControl_DoSlide(menuList_t *s, int dir)
{
	s->curvalue += dir;

	if (s->curvalue < 0)
		s->curvalue = 0;
	else if (s->itemnames[s->curvalue] == 0)
		s->curvalue--;

	if (s->callback)
		s->callback(s);
}

static void SpinControl_Draw(menuList_t *s)
{
	DrawCaption(s);

	// check for valid index
	int maxIndex = 0;
	while (s->itemnames[maxIndex]) maxIndex++;
	maxIndex--;
	if (s->curvalue > maxIndex)
		s->curvalue = maxIndex;
	// draw value
	const char *text = s->itemnames[s->curvalue];
	const char *newline;

	int x = s->x + s->parent->x;
	int y = s->y + s->parent->y;

	if (!(newline = strchr(text, '\n')))
		DrawString(RCOLUMN_OFFSET + x, y, text);
	else
	{
		char buffer[256];
		appStrncpyz(buffer, text, newline - text + 1);	// copy text till '\n' and put zero to its place
		DrawString(RCOLUMN_OFFSET + x, y, buffer);
		DrawString(RCOLUMN_OFFSET + x, y + 10, newline+1);
	}
}


static void SpinControl2_DoSlide(menuList2_t *s, int dir)
{
	s->curvalue += dir;

	if (s->curvalue < 0)
		s->curvalue = 0;
	else
	{
		CStringItem *item;
		int		i;

		for (i = 0, item = s->itemnames; item && i < s->curvalue; item = item->next, i++) ;
		if (!item) i--;
		s->curvalue = i;
	}

	if (s->callback)
		s->callback(s);
}

static void SpinControl2_Draw(menuList2_t *s)
{
	int		i;
	CStringItem *item, *last;

	DrawCaption(s);

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
	const char *text = item->name;
	const char *newline;

	int x = s->x + s->parent->x;
	int y = s->y + s->parent->y;

	if (!(newline = strchr(text, '\n')))
		DrawString(RCOLUMN_OFFSET + x, y, text);
	else
	{
		char	buffer[256];

		appStrncpyz(buffer, text, newline - text + 1);	// copy text till '\n' and put zero to its place
		DrawString(RCOLUMN_OFFSET + x, y, buffer);
		DrawString(RCOLUMN_OFFSET + x, y + 10, newline+1);
	}
}


//-----------------------------------------------------------------------------

#define	MAX_MENU_DEPTH	16
static menuFramework_t *m_layers[MAX_MENU_DEPTH];

menuFramework_t *m_current;
static int		m_menudepth;

void menuFramework_t::Push()
{
	// if this menu is already present, drop back to that level
	// to avoid stacking menus by hotkeys
//	if (m_menudepth == 0) CL_Pause(true);
	int i;
	for (i = 0; i < m_menudepth; i++)
		if (m_layers[i] == this)
		{
			m_menudepth = ++i;	//!! if we use dynamic menus - need to free i+1..m_menudepth (check all m_menudepth changes !!)
			//?? add SetMenuDepth() function?
			break;
		}

	if (i == m_menudepth)
	{	// menu was not pushed
		if (m_menudepth >= MAX_MENU_DEPTH)
			appError("M_PushMenu: MAX_MENU_DEPTH");
		if (!Init()) return;
		m_layers[m_menudepth++] = this;
	}
	if (m_menudepth == 1) CL_Pause(true);

	m_current = this;
	AdjustCursor(1);
	cls.key_dest = key_menu;
}

void M_ForceMenuOff()
{
	//?? should call for each opened menu "virtual OnClose()" for freeing menu data
	//?? now: will leak resources, when open some resource-allocating menu and typing
	//??      "disconnect" from console
	m_current   = NULL;
	m_menudepth = 0;
	cls.key_dest = key_game;
	Key_ClearStates();
	CL_Pause(false);
	SCR_ShowConsole(false, true);	// immediately hide console
}

void menuFramework_t::Pop()
{
	S_StartLocalSound(menu_out_sound);
	if (m_menudepth < 1)
		appError("M_PopMenu: depth < 1");

	m_menudepth--;
	if (m_menudepth)
		m_current = m_layers[m_menudepth-1];
	else
		M_ForceMenuOff();
}


const char *menuFramework_t::KeyDown(int key)
{
	const char *sound = NULL;

	menuCommon_t *item;
	if ((item = ItemAtCursor()) && item->type == MTYPE_FIELD && Field_Key((menuField_t*) item, key))
		return NULL;

	switch (key)
	{
	case K_ESCAPE:
	case K_MOUSE2:
		Pop();
		return menu_out_sound;
	case K_KP_UPARROW:
	case K_UPARROW:
	case K_MWHEELUP:
		cursor--;
		AdjustCursor(-1);
		sound = menu_move_sound;
		break;
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
	case K_MWHEELDOWN:
		cursor++;
		AdjustCursor(1);
		sound = menu_move_sound;
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		SlideItem(-1);
		sound = menu_move_sound;
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		SlideItem(1);
		sound = menu_move_sound;
		break;
	case K_HOME:
		cursor = 0;
		AdjustCursor(1);
		sound = menu_move_sound;
		break;
	case K_END:
		cursor = nitems - 1;
		AdjustCursor(-1);
		sound = menu_move_sound;
		break;

	case K_MOUSE1:
	case K_KP_ENTER:
	case K_ENTER:
		SelectItem();
		sound = menu_move_sound;
		break;
	}

	return sound;
}


bool menuFramework_t::Init()
{
	return true;		// enable menu by default
}


void M_Draw()
{
	if (!m_current) return;

	if (!cls.keep_console)	// do not blend whole screen when small menu painted
		RE_Fill(0, 0, viddef.width, viddef.height, RGBA(0,0,0,0.5));

	m_current->Draw();
}


void M_Keydown(int key)
{
	if (!m_current) return;

	const char *s = m_current->KeyDown(key);
	if (s) S_StartLocalSound(s);
}
