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
#include "../client/client.h"
#include "../client/qmenu.h"


#define REF_SOFT	0
#define REF_OPENGL	1

#define MIN_GAMMA	5		// 0.5
#define MAX_GAMMA	20		// 2.0


/*------- cvars -------*/

//?? ADD r_ignorehwgamma, gl_ext_compressed_textures and gl_overbrightBits
extern cvar_t *vid_ref;
extern cvar_t *scr_viewsize;
extern cvar_t *r_gamma;
//static cvar_t *gl_driver;
static cvar_t *r_saturation;
static cvar_t *gl_picmip;
static cvar_t *gl_bitdepth;
static cvar_t *gl_textureBits;
static cvar_t *gl_vertexLight;

extern void M_ForceMenuOff (void);


/*----------------- Menu interaction -----------------------*/


#define SOFTWARE_MENU 0
#define OPENGL_MENU   1

static menuFramework_t  s_software_menu;
static menuFramework_t	s_opengl_menu;
static menuFramework_t *s_current_menu;
static int				s_current_menu_index;

/*--- Common menu items ---*/
static menuList_t		s_ref_list[2];				// renderer
static menuList_t		s_mode_list[2];				// video mode
static menuSlider_t		s_screensize_slider[2];		// screen size
static menuSlider_t		s_brightness_slider[2];		// gamma
static menuSlider_t		s_saturation[2];			// saturation
static menuList_t  		s_fs_box[2];				// fullscreen
static menuAction_t		s_cancel_action[2];
static menuAction_t		s_defaults_action[2];

/*--- Software renderer ---*/
static menuList_t  		s_stipple_box;

/*---- OpenGL renderer ----*/
static menuList_t  		s_finish_box;				// gl_finish
static menuList_t		s_colorDepth;				// gl_bitdepth
// quality/speed
static menuList_t		s_lightStyle;				// gl_vertexLight
static menuList_t		s_fastSky;					// gl_fastSky
// textures
static menuSlider_t		s_tq_slider;				// gl_picmip
static menuList_t		s_textureBits;				// gl_textureBits
static menuList_t		s_textureFilter;			// gl_texturemode


float vid_menu_old_gamma;

static void DriverCallback (void *unused)
{
	s_ref_list[!s_current_menu_index].curvalue = s_ref_list[s_current_menu_index].curvalue;

	if (s_ref_list[s_current_menu_index].curvalue == 0)
	{
		s_current_menu = &s_software_menu;
		s_current_menu_index = 0;
	}
	else
	{
		s_current_menu = &s_opengl_menu;
		s_current_menu_index = 1;
	}

}

static void ScreenSizeCallback (void *s)
{
	menuSlider_t *slider = (menuSlider_t *) s;

	Cvar_SetInteger ("viewsize", slider->curvalue * 10);
}

static void BrightnessCallback (void *s)
{
	menuSlider_t *slider = (menuSlider_t *) s;

	if (s_current_menu_index == SOFTWARE_MENU)
		s_brightness_slider[1].curvalue = s_brightness_slider[0].curvalue;
	else
		s_brightness_slider[0].curvalue = s_brightness_slider[1].curvalue;

//	if (stricmp (vid_ref->string, "soft") == 0) -- can work with GL hardware gamma
	{
		float gamma = slider->curvalue / 10.0;

		Cvar_SetValue ("r_gamma", gamma);
	}
}

static void ResetDefaults (void *unused)
{
	Vid_MenuInit ();
}


void M_PopMenu (void);

static void ApplyChanges (void *unused)
{
	int		quit;

	quit = 0;		// 1 - gl_mode changed, 2 - sw_mode changed

	// make values consistent
	s_fs_box[!s_current_menu_index].curvalue = s_fs_box[s_current_menu_index].curvalue;
	s_brightness_slider[!s_current_menu_index].curvalue = s_brightness_slider[s_current_menu_index].curvalue;
	s_ref_list[!s_current_menu_index].curvalue = s_ref_list[s_current_menu_index].curvalue;

	// common
	Cvar_SetInteger ("r_fullscreen", s_fs_box[s_current_menu_index].curvalue);
	Cvar_SetValue ("r_saturation", s_saturation[s_current_menu_index].curvalue / 10.0);
	// software
	Cvar_SetInteger ("sw_stipplealpha", s_stipple_box.curvalue);
	if (Cvar_SetInteger ("sw_mode", s_mode_list[SOFTWARE_MENU].curvalue)->modified) quit |= 2;
	// OpenGL
	Cvar_SetInteger ("gl_picmip", 3 - s_tq_slider.curvalue);
	Cvar_SetInteger ("gl_finish", s_finish_box.curvalue);
	if (Cvar_SetInteger ("gl_mode", s_mode_list[OPENGL_MENU].curvalue)->modified) quit |= 1;
	Cvar_SetInteger ("gl_fastsky", s_fastSky.curvalue);
	Cvar_SetInteger ("gl_bitdepth", s_colorDepth.curvalue * 16);
	Cvar_SetInteger ("gl_textureBits", s_textureBits.curvalue * 16);
	Cvar_SetInteger ("gl_vertexLight", s_lightStyle.curvalue);
	Cvar_Set ("gl_texturemode", s_textureFilter.curvalue ? "GL_LINEAR_MIPMAP_LINEAR" : "GL_LINEAR_MIPMAP_NEAREST");

	switch (s_ref_list[s_current_menu_index].curvalue)
	{
	case REF_SOFT:
		Cvar_Set ("vid_ref", "soft");
		quit &= 2;		// reset "gl_mode changed"
		break;
	case REF_OPENGL:
		Cvar_Set ("vid_ref", "gl");
//		Cvar_Set ("gl_driver", "opengl32");
		quit &= 1;		// reset "sw_mode changed"
		break;
	}

#define CHECK_UPDATE(var)	\
	if (var->modified)	\
	{	\
		var->modified = false;		\
		vid_ref->modified = true;	\
		Com_DPrintf ("%s modified -- updating\n", var->name);	\
	}

	CHECK_UPDATE(r_saturation);
	CHECK_UPDATE(r_gamma);

	if (!stricmp (vid_ref->string, "gl"))
	{
		CHECK_UPDATE(gl_picmip);
		CHECK_UPDATE(gl_vertexLight);
		CHECK_UPDATE(gl_bitdepth);
		CHECK_UPDATE(gl_textureBits);
	}

	if (vid_ref->modified || quit)
		M_ForceMenuOff ();
	else
		M_PopMenu ();
}

static void CancelChanges (void *unused)
{
	Cvar_SetValue ("r_gamma", vid_menu_old_gamma);
	M_PopMenu ();
}

/*
 * Vid_MenuInit
 */
void Vid_MenuInit( void )
{
	int i, y;

	static const char *resolutions[] = {
		"[320  240 ]",	"[400  300 ]",	"[512  384 ]",	"[640  480 ]",
		"[800  600 ]",	"[960  720 ]",	"[1024 768 ]",	"[1152 864 ]",
		"[1280 960 ]",	"[1600 1200]",	"[2048 1536]",	NULL
	};
	static const char *refNames[] = {
		"[software]",	"[OpenGL  ]",	NULL
	};
	static const char *yesnoNames[] = {
		"no",			"yes",			NULL
	};
	static const char *bits[] = {
		"default",		"16 bit",		"32 bit",		NULL
	};
	static const char *lighting[] = {
		"lightmap",		"vertex",		NULL
	};
	static const char *filters[] = {
		"bilinear",		"trilinear",	NULL
	};

	// init cvars
CVAR_BEGIN(vars)
//	CVAR_GET(gl_driver, opengl32, CVAR_ARCHIVE),
	{&scr_viewsize, "viewsize", "100", CVAR_ARCHIVE},
	CVAR_VAR(r_saturation, 1, CVAR_ARCHIVE),
	CVAR_VAR(gl_picmip, 0, CVAR_ARCHIVE),
	CVAR_VAR(gl_bitdepth, 0, CVAR_ARCHIVE),
	CVAR_VAR(gl_textureBits, 0, CVAR_ARCHIVE),
	CVAR_VAR(gl_vertexLight, 0, CVAR_ARCHIVE)
CVAR_END

	CVAR_GET_VARS(vars);

	s_mode_list[SOFTWARE_MENU].curvalue = Cvar_Get ("sw_mode", "0", CVAR_ARCHIVE)->integer;
	s_mode_list[OPENGL_MENU].curvalue = Cvar_Get ("gl_mode", "3", CVAR_ARCHIVE)->integer;

	if (!strcmp (vid_ref->string, "soft"))
	{
		s_current_menu_index = SOFTWARE_MENU;
		s_ref_list[0].curvalue = s_ref_list[1].curvalue = REF_SOFT;
	}
	else // if (!strcmp (vid_ref->string, "gl"))
	{
		s_current_menu_index = OPENGL_MENU;
		s_ref_list[s_current_menu_index].curvalue = REF_OPENGL;
	}

	s_software_menu.x = viddef.width / 2;
	s_software_menu.y = viddef.height / 2 - 58;
	s_software_menu.nitems = 0;
	s_opengl_menu.x = viddef.width / 2;
	s_opengl_menu.y = viddef.height / 2 - 58;
	s_opengl_menu.nitems = 0;

	// initialize common menu items
	for (i = 0; i < 2; i++)
	{
		y = 0;
		MENU_SPIN(s_ref_list[i], y+=10, "driver", DriverCallback, refNames);
		MENU_SPIN(s_mode_list[i], y+=10, "video mode", NULL, resolutions);
		MENU_SLIDER(s_screensize_slider[i], y+=10, "screen size", ScreenSizeCallback, 4, 10);
		s_screensize_slider[i].curvalue = scr_viewsize->value / 10;
		MENU_SLIDER(s_brightness_slider[i], y+=10, "brightness", BrightnessCallback, MIN_GAMMA, MAX_GAMMA);
		Cvar_Clamp (r_gamma, 0.5, 3);
		s_brightness_slider[i].curvalue = r_gamma->value * 10;
		MENU_SLIDER(s_saturation[i], y+=10, "saturation", NULL, 0, 20);
		Cvar_Clamp (r_saturation, 0, 2);
		s_saturation[i].curvalue = r_saturation->value * 10;
		MENU_SPIN(s_fs_box[i], y+=10, "fullscreen", NULL, yesnoNames);
		s_fs_box[i].curvalue = Cvar_Get ("r_fullscreen", "1", CVAR_ARCHIVE)->integer;

		y = 160;
		MENU_ACTION(s_defaults_action[i], y+=10, "reset to defaults", ResetDefaults);
		MENU_ACTION(s_cancel_action[i], y+=10, "cancel", CancelChanges);
	}
	// save current gamma (for cancel action)
	vid_menu_old_gamma = r_gamma->value;

	// software renderer menu
	y = 70;
	MENU_SPIN(s_stipple_box, y+=10, "stipple alpha", NULL, yesnoNames);
	s_stipple_box.curvalue = Cvar_Get ("sw_stipplealpha", "0", CVAR_ARCHIVE)->integer;

	// OpenGL renderer menu
	y = 70;
	MENU_SPIN(s_colorDepth, y+=10, "color depth", NULL, bits);
	s_colorDepth.curvalue = gl_bitdepth->integer >> 4;
	MENU_SPIN(s_lightStyle, y+=10, "lighting", NULL, lighting);
	s_lightStyle.curvalue = gl_vertexLight->integer;
	MENU_SPIN(s_fastSky, y+=10, "fast sky", NULL, yesnoNames);
	s_fastSky.curvalue = Cvar_Get ("gl_fastsky", "0", CVAR_ARCHIVE)->integer;
	MENU_SLIDER(s_tq_slider, y+=10, "texture detail", NULL, 0, 3);
	s_tq_slider.curvalue = 3 - gl_picmip->integer;
	MENU_SPIN(s_textureBits, y+=10, "texture quality", NULL, bits);
	s_textureBits.curvalue = gl_textureBits->integer >> 4;
	MENU_SPIN(s_textureFilter, y+=10, "texture filter", NULL, filters);
	s_textureFilter.curvalue = stricmp (Cvar_Get ("gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE)->string,
		"GL_LINEAR_MIPMAP_LINEAR") ? 0 : 1;
	MENU_SPIN(s_finish_box, y+=10, "sync every frame", NULL, yesnoNames);
	s_finish_box.curvalue = Cvar_Get ("gl_finish", "0", CVAR_ARCHIVE)->integer;

	Menu_AddItem (&s_software_menu, (void *) &s_ref_list[SOFTWARE_MENU]);
	Menu_AddItem (&s_software_menu, (void *) &s_mode_list[SOFTWARE_MENU]);
	Menu_AddItem (&s_software_menu, (void *) &s_screensize_slider[SOFTWARE_MENU]);
	Menu_AddItem (&s_software_menu, (void *) &s_brightness_slider[SOFTWARE_MENU]);
	Menu_AddItem (&s_software_menu, (void *) &s_saturation[SOFTWARE_MENU]);
	Menu_AddItem (&s_software_menu, (void *) &s_fs_box[SOFTWARE_MENU]);
	Menu_AddItem (&s_software_menu, (void *) &s_stipple_box);

	Menu_AddItem (&s_opengl_menu, (void *) &s_ref_list[OPENGL_MENU]);
	Menu_AddItem (&s_opengl_menu, (void *) &s_mode_list[OPENGL_MENU]);
	Menu_AddItem (&s_opengl_menu, (void *) &s_screensize_slider[OPENGL_MENU]);
	Menu_AddItem (&s_opengl_menu, (void *) &s_brightness_slider[OPENGL_MENU]);
	Menu_AddItem (&s_opengl_menu, (void *) &s_saturation[OPENGL_MENU]);
	Menu_AddItem (&s_opengl_menu, (void *) &s_fs_box[OPENGL_MENU]);
	Menu_AddItem (&s_opengl_menu, (void *) &s_colorDepth);
	Menu_AddItem (&s_opengl_menu, (void *) &s_lightStyle);
	Menu_AddItem (&s_opengl_menu, (void *) &s_fastSky);
	Menu_AddItem (&s_opengl_menu, (void *) &s_tq_slider);
	Menu_AddItem (&s_opengl_menu, (void *) &s_textureBits);
	Menu_AddItem (&s_opengl_menu, (void *) &s_textureFilter);
	Menu_AddItem (&s_opengl_menu, (void *) &s_finish_box);

	Menu_AddItem (&s_software_menu, (void *) &s_defaults_action[SOFTWARE_MENU]);
	Menu_AddItem (&s_software_menu, (void *) &s_cancel_action[SOFTWARE_MENU]);
	Menu_AddItem (&s_opengl_menu, (void *) &s_defaults_action[OPENGL_MENU]);
	Menu_AddItem (&s_opengl_menu, (void *) &s_cancel_action[OPENGL_MENU]);
}

/*
================
Vid_MenuDraw
================
*/
void Vid_MenuDraw (void)
{
	int w, h;

	if (s_current_menu_index == 0)
		s_current_menu = &s_software_menu;
	else
		s_current_menu = &s_opengl_menu;

	// draw the banner
	re.DrawGetPicSize (&w, &h, "m_banner_video");
	re_DrawPic (viddef.width / 2 - w / 2, viddef.height /2 - 110, "m_banner_video");

	// move cursor to a reasonable starting position
	Menu_AdjustCursor (s_current_menu, 1);

	// draw the menu
	Menu_Draw (s_current_menu);
}

/*
================
Vid_MenuKey
================
*/
const char *Vid_MenuKey (int key)
{
	menuFramework_t *m = s_current_menu;
	static const char *sound = "misc/menu1.wav";

	switch (key)
	{
	case K_ESCAPE:
	case K_MOUSE2:
		ApplyChanges (0);
		return NULL;
	case K_KP_UPARROW:
	case K_UPARROW:
	case K_MWHEELUP:
		m->cursor--;
		Menu_AdjustCursor (m, -1);
		break;
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
	case K_MWHEELDOWN:
		m->cursor++;
		Menu_AdjustCursor (m, 1);
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		Menu_SlideItem (m, -1);
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		Menu_SlideItem (m, 1);
		break;
	case K_MOUSE1:
		Menu_SelectItem (m);
		break;
	case K_KP_ENTER:
	case K_ENTER:
		if (!Menu_SelectItem (m))
			ApplyChanges (NULL);
		break;
	case K_HOME:
		m->cursor = 0;
		Menu_AdjustCursor (m, 1);
		break;
	case K_END:
		m->cursor = m->nitems - 1;
		Menu_AdjustCursor (m, -1);
		break;
	}
	return sound;
}
