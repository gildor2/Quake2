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
// r_main.c

#include "r_local.h"

viddef_t	vid;
refImport_t	ri;
static int ref_flags;

unsigned	d_8to24table[256];

entity_t	r_worldentity;

char		skyname[MAX_QPATH];
float		skyrotate;
vec3_t		skyaxis;
image_t		*sky_images[6];

refdef_t	r_newrefdef;
model_t		*currentmodel;

model_t		*r_worldmodel;

byte		r_warpbuffer[WARP_WIDTH * WARP_HEIGHT];

swstate_t sw_state;

void		*colormap;
vec3_t		viewlightvec;
alight_t	r_viewlighting = {128, 192, viewlightvec};
float		r_time1;
int			r_numallocatededges;
float		r_aliasuvscale = 1.0;
int			r_outofsurfaces;
int			r_outofedges;

qboolean	r_dowarp;

mvertex_t	*r_pcurrentvertbase;

qboolean	con_only;	// sw_con_only

int			c_surf;
int			r_maxsurfsseen, r_maxedgesseen, r_cnumsurfs;
qboolean	r_surfsonstack;
int			r_clipflags;

//
// view origin
//
vec3_t	vup, base_vup;
vec3_t	vpn, base_vpn;
vec3_t	vright, base_vright;
vec3_t	r_origin;

//
// screen size info
//
oldrefdef_t	r_refdef;
float		xcenter, ycenter;
float		xscale, yscale;
float		xscaleinv, yscaleinv;
float		xscaleshrink, yscaleshrink;
float		aliasxscale, aliasyscale, aliasxcenter, aliasycenter;

int		r_screenwidth;

float	verticalFieldOfView;
float	xOrigin, yOrigin;

cplane_t	screenedge[4];

//
// refresh flags
//
int		r_framecount = 1;	// so frame counts initialized to 0 don't match
int		r_visframecount;
int		d_spanpixcount;
int		r_polycount;
int		r_drawnpolycount;
int		r_wholepolycount;

int			*pfrustum_indexes[4];
int			r_frustum_indexes[4*6];

mleaf_t		*r_viewleaf;
int			r_viewcluster, r_oldviewcluster;

image_t  	*r_notexture_mip;

float	da_time1, da_time2, dp_time1, dp_time2, db_time1, db_time2, rw_time1, rw_time2;
float	se_time1, se_time2, de_time1, de_time2;

void R_MarkLeaves (void);

cvar_t	*r_lefthand;
cvar_t	*sw_aliasstats;
cvar_t	*sw_allow_modex;
cvar_t	*sw_clearcolor;
cvar_t	*sw_drawflat;
cvar_t	*sw_draworder;
cvar_t	*sw_maxedges;
cvar_t	*sw_maxsurfs;
cvar_t  *sw_mode;
cvar_t	*sw_reportedgeout;
cvar_t	*sw_reportsurfout;
cvar_t  *sw_stipplealpha;
cvar_t	*sw_surfcacheoverride;
cvar_t	*sw_waterwarp;

cvar_t	*r_drawworld;
cvar_t	*r_drawentities;
cvar_t	*r_dspeeds;
cvar_t	*r_fullbright;
cvar_t  *r_lerpmodels;
cvar_t  *r_novis;

cvar_t	*r_speeds;
float	sw_lightlevel;

cvar_t	*r_fullscreen;
cvar_t	*r_gamma, *r_contrast, *r_brightness;

cvar_t	*r_saturation;

//PGM
cvar_t	*sw_lockpvs;
//PGM


#if	!id386

// r_vars.c

// all global and static refresh variables are collected in a contiguous block
// to avoid cache conflicts.

//-------------------------------------------------------
// global refresh variables
//-------------------------------------------------------

// FIXME: make into one big structure, like cl or sv
// FIXME: do separately for refresh engine and driver


// d_vars.c

// all global and static refresh variables are collected in a contiguous block
// to avoid cache conflicts.

//-------------------------------------------------------
// global refresh variables
//-------------------------------------------------------

// FIXME: make into one big structure, like cl or sv
// FIXME: do separately for refresh engine and driver

float	d_sdivzstepu, d_tdivzstepu, d_zistepu;
float	d_sdivzstepv, d_tdivzstepv, d_zistepv;
float	d_sdivzorigin, d_tdivzorigin, d_ziorigin;

fixed16_t	sadjust, tadjust, bbextents, bbextentt;

pixel_t			*cacheblock;
int				cachewidth;
pixel_t			*d_viewbuffer;
short			*d_pzbuffer;
unsigned int	d_zrowbytes;
unsigned int	d_zwidth;


#endif	// !id386

byte	r_notexture_buffer[1024];

/*
==================
R_InitTextures
==================
*/
void	R_InitTextures (void)
{
	int		x,y, m;
	byte	*dest;

// create a simple checkerboard texture for the default
	r_notexture_mip = (image_t *)&r_notexture_buffer;

	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->pixels[0] = &r_notexture_buffer[sizeof(image_t)];
	r_notexture_mip->pixels[1] = r_notexture_mip->pixels[0] + 16*16;
	r_notexture_mip->pixels[2] = r_notexture_mip->pixels[1] + 8*8;
	r_notexture_mip->pixels[3] = r_notexture_mip->pixels[2] + 4*4;

	for (m=0 ; m<4 ; m++)
	{
		dest = r_notexture_mip->pixels[m];
		for (y=0 ; y< (16>>m) ; y++)
			for (x=0 ; x< (16>>m) ; x++)
			{
				if (  (y< (8>>m) ) ^ (x< (8>>m) ) )

					*dest++ = 0;
				else
					*dest++ = 0xff;
			}
	}
}


/*
================
R_InitTurb
================
*/
void R_InitTurb (void)
{
	int		i;

	for (i=0 ; i<1280 ; i++)
	{
		sintable[i] = AMP + sin(i*3.14159*2/CYCLE)*AMP;
		intsintable[i] = AMP2 + sin(i*3.14159*2/CYCLE)*AMP2;	// AMP2, not 20
		blanktable[i] = 0;			//PGM
	}
}

void R_ImageList_f( void );

void R_Register (void)
{
CVAR_BEGIN(vars)
	{&sw_aliasstats, "sw_polymodelstats", "0", 0},
	CVAR_VAR(sw_allow_modex, 1, CVAR_ARCHIVE),
	CVAR_VAR(sw_clearcolor, 2, 0),
	CVAR_VAR(sw_drawflat, 0, 0),
	CVAR_VAR(sw_draworder, 0, CVAR_CHEAT),
	CVAR_VAR(sw_maxsurfs, 0, 0),
	CVAR_VAR(sw_mipcap, 0, 0),
	CVAR_VAR(sw_mipscale, 1, 0),
	CVAR_VAR(sw_reportedgeout, 0, 0),
	CVAR_VAR(sw_reportsurfout, 0, 0),
	CVAR_VAR(sw_stipplealpha, 0, CVAR_ARCHIVE),
	CVAR_VAR(sw_surfcacheoverride, 0, 0),
	CVAR_VAR(sw_waterwarp, 1, 0),
	CVAR_VAR(sw_mode, 0, CVAR_ARCHIVE),

	{&r_lefthand, "hand", "0", CVAR_USERINFO|CVAR_ARCHIVE},
	CVAR_VAR(r_speeds, 0, 0),
	CVAR_VAR(r_fullbright, 0, CVAR_CHEAT),
	CVAR_VAR(r_drawentities, 1, 0),
	CVAR_VAR(r_drawworld, 1, CVAR_CHEAT),
	CVAR_VAR(r_dspeeds, 0, 0),
	CVAR_VAR(r_lerpmodels, 1, 0),
	CVAR_VAR(r_novis, 0, 0),
	CVAR_VAR(sw_lockpvs, 0, 0),

	CVAR_VAR(r_fullscreen, 1, CVAR_ARCHIVE),
	CVAR_VAR(r_gamma, 1, CVAR_ARCHIVE),
	CVAR_VAR(r_contrast, 1, CVAR_ARCHIVE),
	CVAR_VAR(r_brightness, 1, CVAR_ARCHIVE),

	CVAR_VAR(r_saturation, 1, CVAR_ARCHIVE),
CVAR_END

	Cvar_GetVars (ARRAY_ARG(vars));
	sw_maxedges = Cvar_Get ("sw_maxedges", va("%d", NUMSTACKEDGES), 0);	//?? MAXSTACKSURFACES

	Cmd_AddCommand ("modellist", Mod_Modellist_f);
	Cmd_AddCommand ("imagelist", R_ImageList_f);

	sw_mode->modified = true; // force us to do mode specific stuff later
	r_gamma->modified = true; // force us to rebuild the gamma table later
}

void R_UnRegister (void)
{
	Cmd_RemoveCommand ("modellist");
	Cmd_RemoveCommand( "imagelist" );
}

/*
===============
R_Init
===============
*/
qboolean R_Init (void)
{
	R_InitImages ();
	Mod_Init ();
	Draw_InitLocal ();
	R_InitTextures ();

	R_InitTurb ();
	R_InitAdivtab ();

	view_clipplanes[0].leftedge = true;
	view_clipplanes[1].rightedge = true;
	view_clipplanes[1].leftedge = view_clipplanes[2].leftedge =
			view_clipplanes[3].leftedge = false;
	view_clipplanes[0].rightedge = view_clipplanes[2].rightedge =
			view_clipplanes[3].rightedge = false;

	r_refdef.xOrigin = XCENTERING;
	r_refdef.yOrigin = YCENTERING;

// TODO: collect 386-specific code in one place
#if	id386
	Sys_MakeCodeWriteable ((long)R_EdgeCodeStart,
					     (long)R_EdgeCodeEnd - (long)R_EdgeCodeStart);
	Sys_SetFPCW ();		// get bit masks for FPCW	(FIXME: is this id386?)
#endif	// id386

	r_aliasuvscale = 1.0;

	R_Register ();
	Draw_GetPalette ();
	SWimp_Init ();

	// create the window
	R_BeginFrame( 0 );

	Com_Printf ("ref_soft version: "REF_VERSION"\n");

	return true;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown (void)
{
	// free z buffer
	if (d_pzbuffer)
	{
		Z_Free (d_pzbuffer);
		d_pzbuffer = NULL;
	}
	// free surface cache
	if (sc_base)
	{
		D_FlushCaches ();
		Z_Free (sc_base);
		sc_base = NULL;
	}

	// free colormap
	if (vid.colormap)
	{
		Z_Free (vid.colormap);
		vid.colormap = NULL;
	}
	R_UnRegister ();
	Mod_FreeAll ();
	R_ShutdownImages ();

	SWimp_Shutdown();
}

/*
===============
R_NewMap
===============
*/
void R_NewMap (void)
{
	r_viewcluster = -1;

	r_cnumsurfs = sw_maxsurfs->integer;

	if (r_cnumsurfs <= MINSURFACES)
		r_cnumsurfs = MINSURFACES;

	if (r_cnumsurfs > NUMSTACKSURFACES)
	{
		surfaces = Z_Malloc (r_cnumsurfs * sizeof(surf_t));
		surface_p = surfaces;
		surf_max = &surfaces[r_cnumsurfs];
		r_surfsonstack = false;
	// surface 0 doesn't really exist; it's just a dummy because index 0
	// is used to indicate no edge attached to surface
		surfaces--;
		R_SurfacePatch ();
	}
	else
	{
		r_surfsonstack = true;
	}

	r_maxedgesseen = 0;
	r_maxsurfsseen = 0;

	r_numallocatededges = sw_maxedges->integer;

	if (r_numallocatededges < MINEDGES)
		r_numallocatededges = MINEDGES;

	if (r_numallocatededges <= NUMSTACKEDGES)
	{
		auxedges = NULL;
	}
	else
	{
		auxedges = Z_Malloc (r_numallocatededges * sizeof(edge_t));
	}
}


/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
void R_MarkLeaves (void)
{
	byte	*vis;
	mnode_t	*node;
	int		i;
	mleaf_t	*leaf;
	int		cluster;

	if (r_oldviewcluster == r_viewcluster && !r_novis->integer && r_viewcluster != -1)
		return;

	// development aid to let you run around and see exactly where
	// the pvs ends
	if (sw_lockpvs->integer)
		return;

	r_visframecount++;
	r_oldviewcluster = r_viewcluster;

	if (r_novis->integer || r_viewcluster == -1 || !r_worldmodel->vis)
	{
		// mark everything
		for (i=0 ; i<r_worldmodel->numleafs ; i++)
			r_worldmodel->leafs[i].visframe = r_visframecount;
		for (i=0 ; i<r_worldmodel->numnodes ; i++)
			r_worldmodel->nodes[i].visframe = r_visframecount;
		return;
	}

	vis = Mod_ClusterPVS (r_viewcluster, r_worldmodel);

	for (i=0,leaf=r_worldmodel->leafs ; i<r_worldmodel->numleafs ; i++, leaf++)
	{
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;
		if (vis[cluster>>3] & (1<<(cluster&7)))
		{
			node = (mnode_t *)leaf;
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}

#if 0
	for (i=0 ; i<r_worldmodel->vis->numclusters ; i++)
	{
		if (vis[i>>3] & (1<<(i&7)))
		{
			node = (mnode_t *)&r_worldmodel->leafs[i];	// FIXME: cluster
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
#endif
}

/*
** R_DrawNullModel
**
** IMPLEMENT THIS!
*/
void R_DrawNullModel( void )
{
}

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int			i;
	qboolean	translucent_entities = false;
	beam_t	*b;

	if (!r_drawentities->integer)
		return;

	// all bmodels have already been drawn by the edge list
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];

		if ( currententity->flags & RF_TRANSLUCENT )
		{
			translucent_entities = true;
			continue;
		}

		currentmodel = currententity->model;
		if (!currentmodel)
		{
			R_DrawNullModel();
			continue;
		}
		VectorCopy (currententity->origin, r_entorigin);
		VectorSubtract (r_origin, r_entorigin, modelorg);

		switch (currentmodel->type)
		{
		case mod_sprite:
			R_DrawSprite ();
			break;

		case mod_alias:
			R_AliasDrawModel ();
			break;

		default:
			break;
		}
	}

	for (b = r_newrefdef.beams; b; b = b->next)
	{
		modelorg[0] = -r_origin[0];
		modelorg[1] = -r_origin[1];
		modelorg[2] = -r_origin[2];
		VectorCopy( vec3_origin, r_entorigin );
		R_DrawBeam (b);
	}

	if ( !translucent_entities )
		return;

	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];

		if ( !( currententity->flags & RF_TRANSLUCENT ) )
			continue;

		currentmodel = currententity->model;
		if (!currentmodel)
		{
			R_DrawNullModel();
			continue;
		}
		VectorCopy (currententity->origin, r_entorigin);
		VectorSubtract (r_origin, r_entorigin, modelorg);

		switch (currentmodel->type)
		{
		case mod_sprite:
			R_DrawSprite ();
			break;

		case mod_alias:
			R_AliasDrawModel ();
			break;

		default:
			break;
		}
	}
}


/*
=============
R_BmodelCheckBBox
=============
*/
int R_BmodelCheckBBox (float *minmaxs)
{
	int			i, *pindex, clipflags;
	vec3_t		acceptpt, rejectpt;
	float		d;

	clipflags = 0;

	for (i=0 ; i<4 ; i++)
	{
	// generate accept and reject points
	// FIXME: do with fast look-ups or integer tests based on the sign bit
	// of the floating point values

		pindex = pfrustum_indexes[i];

		rejectpt[0] = minmaxs[pindex[0]];
		rejectpt[1] = minmaxs[pindex[1]];
		rejectpt[2] = minmaxs[pindex[2]];

		d = DotProduct (rejectpt, view_clipplanes[i].normal);
		d -= view_clipplanes[i].dist;

		if (d <= 0)
			return BMODEL_FULLY_CLIPPED;

		acceptpt[0] = minmaxs[pindex[3+0]];
		acceptpt[1] = minmaxs[pindex[3+1]];
		acceptpt[2] = minmaxs[pindex[3+2]];

		d = DotProduct (acceptpt, view_clipplanes[i].normal);
		d -= view_clipplanes[i].dist;

		if (d <= 0)
			clipflags |= (1<<i);
	}

	return clipflags;
}


/*
===================
R_FindTopnode

Find the first node that splits the given box
===================
*/
mnode_t *R_FindTopnode (vec3_t mins, vec3_t maxs)
{
	cplane_t	*splitplane;
	int			sides;
	mnode_t *node;

	node = r_worldmodel->nodes;

	while (1)
	{
		if (node->visframe != r_visframecount)
			return NULL;		// not visible at all

		if (node->contents != CONTENTS_NODE)
		{
			if (node->contents != CONTENTS_SOLID)
				return	node; // we've reached a non-solid leaf, so it's
							//  visible and not BSP clipped
			return NULL;	// in solid, so not visible
		}

		splitplane = node->plane;
		sides = BOX_ON_PLANE_SIDE(mins, maxs, (cplane_t *)splitplane);

		if (sides == 3)
			return node;	// this is the splitter

	// not split yet; recurse down the contacted side
		if (sides & 1)
			node = node->children[0];
		else
			node = node->children[1];
	}
}


/*
=============
RotatedBBox

Returns an axially aligned box that contains the input box at the given rotation
=============
*/
void RotatedBBox (vec3_t mins, vec3_t maxs, vec3_t angles, vec3_t tmins, vec3_t tmaxs)
{
	vec3_t	tmp, v;
	int		i, j;
	vec3_t	forward, right, up;

	if (!angles[0] && !angles[1] && !angles[2])
	{
		VectorCopy (mins, tmins);
		VectorCopy (maxs, tmaxs);
		return;
	}

	for (i=0 ; i<3 ; i++)
	{
		tmins[i] = BIG_NUMBER;
		tmaxs[i] = -BIG_NUMBER;
	}

	AngleVectors (angles, forward, right, up);

	for ( i = 0; i < 8; i++ )
	{
		if ( i & 1 )
			tmp[0] = mins[0];
		else
			tmp[0] = maxs[0];

		if ( i & 2 )
			tmp[1] = mins[1];
		else
			tmp[1] = maxs[1];

		if ( i & 4 )
			tmp[2] = mins[2];
		else
			tmp[2] = maxs[2];


		VectorScale (forward, tmp[0], v);
		VectorMA (v, -tmp[1], right, v);
		VectorMA (v, tmp[2], up, v);

		for (j=0 ; j<3 ; j++)
		{
			if (v[j] < tmins[j])
				tmins[j] = v[j];
			if (v[j] > tmaxs[j])
				tmaxs[j] = v[j];
		}
	}
}

/*
=============
R_DrawBEntitiesOnList
=============
*/
void R_DrawBEntitiesOnList (void)
{
	int			i, clipflags;
	vec3_t		oldorigin;
	vec3_t		mins, maxs;
	float		minmaxs[6];
	mnode_t		*topnode;

	if (!r_drawentities->integer)
		return;

	VectorCopy (modelorg, oldorigin);
	insubmodel = true;
	r_dlightframecount = r_framecount;

	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];
		currentmodel = currententity->model;
		if (!currentmodel)
			continue;
		if (currentmodel->nummodelsurfaces == 0)
			continue;	// clip brush only
		if ( currententity->flags & RF_BEAM )
			continue;
		if (currentmodel->type != mod_brush)
			continue;
	// see if the bounding box lets us trivially reject, also sets
	// trivial accept status
		RotatedBBox (currentmodel->mins, currentmodel->maxs,
			currententity->angles, mins, maxs);
		VectorAdd (mins, currententity->origin, minmaxs);
		VectorAdd (maxs, currententity->origin, (minmaxs+3));

		clipflags = R_BmodelCheckBBox (minmaxs);
		if (clipflags == BMODEL_FULLY_CLIPPED)
			continue;	// off the edge of the screen

		topnode = R_FindTopnode (minmaxs, minmaxs+3);
		if (!topnode)
			continue;	// no part in a visible leaf

		VectorCopy (currententity->origin, r_entorigin);
		VectorSubtract (r_origin, r_entorigin, modelorg);

		r_pcurrentvertbase = currentmodel->vertexes;

	// FIXME: stop transforming twice
		R_RotateBmodel ();

	// calculate dynamic lighting for bmodel
		R_PushDlights (currentmodel);

		if (topnode->contents == CONTENTS_NODE)
		{
		// not a leaf; has to be clipped to the world BSP
			r_clipflags = clipflags;
			R_DrawSolidClippedSubmodelPolygons (currentmodel, topnode);
		}
		else
		{
		// falls entirely in one leaf, so we just put all the
		// edges in the edge list and let 1/z sorting handle
		// drawing order
			R_DrawSubmodelPolygons (currentmodel, clipflags, topnode);
		}

	// put back world rotation and frustum clipping
	// FIXME: R_RotateBmodel should just work off base_vxx
		VectorCopy (base_vpn, vpn);
		VectorCopy (base_vup, vup);
		VectorCopy (base_vright, vright);
		VectorCopy (oldorigin, modelorg);
		R_TransformFrustum ();
	}

	insubmodel = false;
}


/*
================
R_EdgeDrawing
================
*/
void R_EdgeDrawing (void)
{
	edge_t	ledges[NUMSTACKEDGES +
				((CACHE_SIZE - 1) / sizeof(edge_t)) + 1];
	surf_t	lsurfs[NUMSTACKSURFACES +
				((CACHE_SIZE - 1) / sizeof(surf_t)) + 1];

	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
		return;

	if (auxedges)
	{
		r_edges = auxedges;
	}
	else
	{
		r_edges =  (edge_t *)
				(((long)&ledges[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));
	}

	if (r_surfsonstack)
	{
		surfaces =  (surf_t *)
				(((long)&lsurfs[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));
		surf_max = &surfaces[r_cnumsurfs];
	// surface 0 doesn't really exist; it's just a dummy because index 0
	// is used to indicate no edge attached to surface
		surfaces--;
		R_SurfacePatch ();
	}

	R_BeginEdgeFrame ();

	if (r_dspeeds->integer)
	{
		rw_time1 = Sys_Milliseconds ();
	}

	R_RenderWorld ();

	if (r_dspeeds->integer)
	{
		rw_time2 = Sys_Milliseconds ();
		db_time1 = rw_time2;
	}

	R_DrawBEntitiesOnList ();

	if (r_dspeeds->integer)
	{
		db_time2 = Sys_Milliseconds ();
		se_time1 = db_time2;
	}

	R_ScanEdges ();
}

//=======================================================================


/*
=============
R_CalcPalette

=============
*/
void R_CalcPalette (void)
{
	static qboolean modified;
	byte	palette[256][4], *in, *out;
	int		i, j;
	float	alpha, one_minus_alpha;
	vec3_t	premult;
	int		v;

	alpha = r_newrefdef.blend[3];
	if (alpha <= 0)
	{
		if (modified)
		{	// set back to default
			modified = false;
			R_GammaCorrectAndSetPalette( ( const unsigned char * ) d_8to24table );
			return;
		}
		return;
	}

	modified = true;
	if (alpha > 1)
		alpha = 1;

	premult[0] = r_newrefdef.blend[0]*alpha*255;
	premult[1] = r_newrefdef.blend[1]*alpha*255;
	premult[2] = r_newrefdef.blend[2]*alpha*255;

	one_minus_alpha = (1.0 - alpha);

	in = (byte *)d_8to24table;
	out = palette[0];
	for (i=0 ; i<256 ; i++, in+=4, out+=4)
	{
		for (j=0 ; j<3 ; j++)
		{
			v = premult[j] + one_minus_alpha * in[j];
			if (v > 255)
				v = 255;
			out[j] = v;
		}
		out[3] = 255;
	}

	R_GammaCorrectAndSetPalette( ( const unsigned char * ) palette[0] );
//	SWimp_SetPalette( palette[0] );
}

//=======================================================================

void R_SetLightLevel (void)
{
	vec3_t		light;

	if ((r_newrefdef.rdflags & RDF_NOWORLDMODEL) || (!r_drawentities->integer) || (!currententity))
	{
		sw_lightlevel = 150.0;
		return;
	}

	// save off light value for server to look at (BIG HACK!)
	R_LightPoint (r_newrefdef.vieworg, light);
	sw_lightlevel = 150.0 * light[0];
}


/*
@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@
*/
void R_RenderFrame (refdef_t *fd)
{
	r_newrefdef = *fd;

	if (!r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		Com_FatalError ("R_RenderView: NULL worldmodel");

	VectorCopy (fd->vieworg, r_refdef.vieworg);
	VectorCopy (fd->viewangles, r_refdef.viewangles);

	if (r_speeds->integer || r_dspeeds->integer)
		r_time1 = Sys_Milliseconds ();

	R_SetupFrame ();

	R_MarkLeaves ();	// done here so we know if we're in water

	R_PushDlights (r_worldmodel);

	R_EdgeDrawing ();

	if (r_dspeeds->integer)
	{
		se_time2 = Sys_Milliseconds ();
		de_time1 = se_time2;
	}

	R_DrawEntitiesOnList ();

	if (r_dspeeds->integer)
	{
		de_time2 = Sys_Milliseconds ();
		dp_time1 = Sys_Milliseconds ();
	}

	R_DrawParticles ();

	if (r_dspeeds->integer)
		dp_time2 = Sys_Milliseconds ();

	R_DrawAlphaSurfaces();

	R_SetLightLevel ();

	if (r_dowarp)
		D_WarpScreen ();

	if (r_dspeeds->integer)
		da_time1 = Sys_Milliseconds ();

	if (r_dspeeds->integer)
		da_time2 = Sys_Milliseconds ();

	R_CalcPalette ();

	if (r_screenshotName && r_screenshotFlags & SHOT_NO_2D)
		R_ScreenShot ();

	R_DrawTexts ();

	if (sw_aliasstats->integer)
		R_PrintAliasStats ();

	if (r_speeds->integer)
		R_PrintTimes ();

	if (r_dspeeds->integer)
		R_PrintDSpeeds ();

	if (sw_reportsurfout->integer && r_outofsurfaces)
		Com_Printf ("Short %d surfaces\n", r_outofsurfaces);

	if (sw_reportedgeout->integer && r_outofedges)
		Com_Printf ("Short roughly %d edges\n", r_outofedges * 2 / 3);

}

/*
** R_InitGraphics
*/
void R_InitGraphics( int width, int height )
{
	vid.width  = width;
	vid.height = height;

	// free z buffer
	if ( d_pzbuffer )
	{
		Z_Free( d_pzbuffer );
		d_pzbuffer = NULL;
	}

	// free surface cache
	if ( sc_base )
	{
		D_FlushCaches ();
		Z_Free( sc_base );
		sc_base = NULL;
	}

	d_pzbuffer = Z_Malloc(vid.width*vid.height*2);

	R_InitCaches ();

	R_GammaCorrectAndSetPalette( ( const unsigned char *) d_8to24table );
}

/*
** R_BeginFrame
*/
void R_BeginFrame( float camera_separation )
{
	extern void Draw_BuildGammaTable( void );

	/*
	** rebuild the gamma correction palette if necessary
	*/
	if (r_gamma->modified || r_contrast->modified || r_brightness->modified)
	{
		Draw_BuildGammaTable();
		R_GammaCorrectAndSetPalette( ( const unsigned char * ) d_8to24table );

		r_gamma->modified = r_contrast->modified = r_brightness->modified = false;
	}

	while ( sw_mode->modified || r_fullscreen->modified )
	{
		rserr_t err;

		/*
		** if this returns rserr_invalid_fullscreen then it set the mode but not as a
		** fullscreen mode, e.g. 320x200 on a system that doesn't support that res
		*/
		if ( ( err = SWimp_SetMode( &vid.width, &vid.height, sw_mode->integer, r_fullscreen->integer ) ) == rserr_ok )
		{
			R_InitGraphics( vid.width, vid.height );

			sw_state.prev_mode = sw_mode->integer;
			r_fullscreen->modified = false;
			sw_mode->modified = false;
		}
		else
		{
			if ( err == rserr_invalid_mode )
			{
				Cvar_SetInteger ("sw_mode", sw_state.prev_mode);
				Com_Printf ("ref_soft::R_BeginFrame() - could not set mode\n");
			}
			else if ( err == rserr_invalid_fullscreen )
			{
				R_InitGraphics( vid.width, vid.height );

				Cvar_SetInteger ("r_fullscreen", 0);
				Com_Printf ("ref_soft::R_BeginFrame() - fullscreen unavailable in this mode\n");
				sw_state.prev_mode = sw_mode->integer;
//				r_fullscreen->modified = false;
//				sw_mode->modified = false;
			}
			else
			{
				Com_FatalError ("ref_soft::R_BeginFrame() - catastrophic mode change failure\n" );
			}
		}
	}

	if (con_only)
		Draw_Fill (0, 0, vid.width, vid.height, 0);
}

/*
** R_GammaCorrectAndSetPalette
*/
void R_GammaCorrectAndSetPalette( const unsigned char *palette )
{
	int i;

	for ( i = 0; i < 256; i++ )
	{
		sw_state.currentpalette[i*4+0] = sw_state.gammatable[palette[i*4+0]];
		sw_state.currentpalette[i*4+1] = sw_state.gammatable[palette[i*4+1]];
		sw_state.currentpalette[i*4+2] = sw_state.gammatable[palette[i*4+2]];
	}

	SWimp_SetPalette( sw_state.currentpalette );
}

/*
** R_CinematicSetPalette
*/
void R_CinematicSetPalette( const unsigned char *palette )
{
	byte palette32[1024];
	int		i, j, w;
	int		*d;

	// clear screen to black to avoid any palette flash
	w = abs(vid.rowbytes)>>2;	// stupid negative pitch win32 stuff...
	for (i=0 ; i<vid.height ; i++, d+=w)
	{
		d = (int *)(vid.buffer + i*vid.rowbytes);
		for (j=0 ; j<w ; j++)
			d[j] = 0;
	}
	// flush it to the screen
	SWimp_EndFrame ();

	if ( palette )
	{
		for ( i = 0; i < 256; i++ )
		{
			palette32[i*4+0] = palette[i*3+0];
			palette32[i*4+1] = palette[i*3+1];
			palette32[i*4+2] = palette[i*3+2];
			palette32[i*4+3] = 0xFF;
		}

		R_GammaCorrectAndSetPalette( palette32 );
	}
	else
	{
		R_GammaCorrectAndSetPalette( ( const unsigned char * ) d_8to24table );
	}
}

/*
================
Draw_BuildGammaTable
================
*/
void Draw_BuildGammaTable (void)
{
	int		i, v;
	float	gamma, invGamma, contr, bright;

	gamma = r_gamma->value;
	if (gamma < 0.1) gamma = 0.1;
	contr = bound(r_contrast->value, 0.5, 2);
	bright = bound(r_brightness->value, 0.5, 2);

	invGamma = 1.0f / gamma;
	for (i = 0; i < 256; i++)
	{
		float	tmp;

		tmp = pow (i / 255.0f, invGamma) * contr * 255;
		tmp = tmp + (bright - 1) * 128 - (contr - 0.5) * 128 + 64;
		v = Q_round (tmp);
		v = bound(v, 0, 255);
		sw_state.gammatable[i] = v;
	}
}

/*
** R_DrawBeam
*/
void R_DrawBeam( beam_t *e )
{
#define NUM_BEAM_SEGS 6

	int	i;

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	oldorigin[0] = e->start[0];
	oldorigin[1] = e->start[1];
	oldorigin[2] = e->start[2];

	origin[0] = e->end[0];
	origin[1] = e->end[1];
	origin[2] = e->end[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;

	PerpendicularVector( perpvec, normalized_direction );
	VectorScale( perpvec, e->radius, perpvec );

	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		RotatePointAroundVector( start_points[i], normalized_direction, perpvec, (360.0/NUM_BEAM_SEGS)*i );
		VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		R_IMFlatShadedQuad( start_points[i],
		                    end_points[i],
							end_points[(i+1)%NUM_BEAM_SEGS],
							start_points[(i+1)%NUM_BEAM_SEGS],
							e->color.c[0],
							e->alpha );
	}
}


//===================================================================

/*
============
R_SetSky
============
*/
// 3dstudio environment map names
char	*suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
int	r_skysideimage[6] = {5, 2, 4, 1, 0, 3};
extern	mtexinfo_t		r_skytexinfo[6];
void R_SetSky (char *name, float rotate, vec3_t axis)
{
	int		i;
	char	pathname[MAX_QPATH];

	strncpy (skyname, name, sizeof(skyname)-1);
	skyrotate = rotate;
	VectorCopy (axis, skyaxis);

	for (i=0 ; i<6 ; i++)
	{
		Com_sprintf (pathname, sizeof(pathname), "env/%s%s.pcx", skyname, suf[r_skysideimage[i]]);
		r_skytexinfo[i].image = R_FindImage (pathname, it_sky);
	}
}


/*
===============
Draw_GetPalette
===============
*/
void Draw_GetPalette (void)
{
	byte	*pal, *out;
	int		i;
	float	sat;

	sat = r_saturation->value;

	// get the palette and colormap
	LoadPCX ("pics/colormap.pcx", &vid.colormap, &pal, NULL, NULL);
	if (!vid.colormap)
		Com_FatalError ("Couldn't load pics/colormap.pcx");
	vid.alphamap = vid.colormap + 64*256;

	out = (byte *)d_8to24table;
	for (i=0 ; i<256 ; i++, out+=4)
	{
		int r, g, b;

		r = pal[i*3+0];
		g = pal[i*3+1];
		b = pal[i*3+2];

//		Com_Printf ("#%2X -> (%2X / %2X / %2X\n)",i,r,g,b);

		if (sat != 1.0)
		{
			float light;
#define SATURATE(c,l,v) c = (l+0.5+(c-l)*v); if (c < 0) c = 0; else if (c > 255) c = 255;
			// change saturation
			light = (r + g + b) / 3.0;
			SATURATE(r,light,sat);
			SATURATE(g,light,sat);
			SATURATE(b,light,sat);
		}

		out[0] = r;
		out[1] = g;
		out[2] = b;
	}

	Z_Free (pal);
}

struct image_s *R_RegisterSkin (char *name);


void	Draw_ConCharColor (int x, int y, int c, int color)
{
	Draw_CharColor (x * 8, y * 8, c, color);
}

static void Screenshot (int flags, char *name)
{
	static char shotName[MAX_QPATH];

	strcpy (shotName, name);
	r_screenshotFlags = flags;
	r_screenshotName = shotName;
}

static float GetClientLight (void)
{
	return sw_lightlevel;
}

static void R_EndFrame (void)
{
	if (r_screenshotName)
		R_ScreenShot ();
	SWimp_EndFrame ();
}

extern unsigned char d_16to8table[65536];

static void Draw_Fill2 (int x, int y, int w, int h, float r, float g, float b, float a)
{
	int ri, gi, bi, color;

	ri = Q_floor(r * 31.9);
	gi = Q_floor(g * 63.9);
	bi = Q_floor(b * 31.9);
	color = d_16to8table[ri | (gi << 5) | (bi << 11)];

	if (a == 1)
	{
		// convert to DrawFill()
		Draw_Fill (x, y, w, h, color);
	}
	else
		Draw_Fade (x, y, w, h, color);
}

static void ReloadImage (char *name)
{
	// do nothing here ??
}


/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refExport_t GetRefAPI (refImport_t rimp)
{
	refExport_t	re;

	ri = rimp;

#ifndef REF_HARD_LINKED
	if (ri.struc_size != sizeof(refImport_t) || ri.api_version != API_VERSION)
	{
		re.struc_size = 0;
		re.api_version = 0;
		return re;
	}
#endif

	con_only = Cvar_Get ("sw_console_only", "0", 0)->integer;

	re.struc_size = sizeof(re);
	re.api_version = API_VERSION;
	re.flags = &ref_flags;
	ref_flags = REF_USE_PALETTE;
	if (con_only)
		ref_flags |= REF_CONSOLE_ONLY;

	re.Init = R_Init;
	re.Shutdown = R_Shutdown;
	re.BeginFrame = R_BeginFrame;
	re.EndFrame = R_EndFrame;
	re.AppActivate = SWimp_AppActivate;
	re.DrawConCharColor = Draw_ConCharColor;
	re.Screenshot =		Screenshot;

	re.RenderFrame = R_RenderFrame;
	re.BeginRegistration = R_BeginRegistration;
	re.RegisterModel = R_RegisterModel;
	re.RegisterSkin = R_RegisterSkin;
	re.RegisterPic = Draw_FindPic;
	re.SetSky = R_SetSky;
	re.EndRegistration = R_EndRegistration;

	re.ReloadImage = ReloadImage;
	re.DrawGetPicSize = Draw_GetPicSize;
	re.DrawPicColor = Draw_PicColor;
	re.DrawStretchPic = Draw_StretchPic;
	re.DrawCharColor = Draw_CharColor;
	re.DrawTileClear = Draw_TileClear;
	re.DrawFill = Draw_Fill;
	re.DrawFill2 = Draw_Fill2;

	re.DrawStretchRaw8 = Draw_StretchRaw8;
	re.SetRawPalette = R_CinematicSetPalette;

	re.DrawTextPos = DrawText_Pos;
	re.DrawTextLeft = DrawText_Left;
	re.DrawTextRight = DrawText_Right;

	re.GetClientLight = GetClientLight;

	Swap_Init ();

	return re;
}
