#include "gl_local.h"
#include "gl_lightmap.h"
#include "gl_model.h"


static int lightmapsNum, currentLightmapNum;
static lightmapBlock_t lmBlocks[MAX_LIGHTMAPS];

static int lmAllocSaved[LIGHTMAP_SIZE];

color_t lmMinlight;


void LM_Init (void)
{
	lightmapsNum = currentLightmapNum = 0;
	lmMinlight.rgba = 0xFFFFFFFF;
}


void LM_Flush (lightmapBlock_t *lm)
{
	if (!lm->empty && !lm->filled)
	{
		// have valid not stored lightmap -- upload it
		lm->image = GL_CreateImage (va("*lightmap%d", lm->index), lm->pic, LIGHTMAP_SIZE, LIGHTMAP_SIZE,
			IMAGE_CLAMP|IMAGE_LIGHTMAP);
		lm->filled = true;
	}
	if (lm->pic)
	{
		// free memory blocks
		Z_Free (lm->pic);
		Z_Free (lm->allocated);
		lm->pic = NULL;
	}
}


void LM_Done (void)
{
	int		i;

	for (i = 0; i < lightmapsNum; i++)
		LM_Flush (&lmBlocks[i]);
}


void LM_Save (lightmapBlock_t *lm)
{
	memcpy (lmAllocSaved, lm->allocated, sizeof(lmAllocSaved));
}


void LM_Restore (lightmapBlock_t *lm)
{
	memcpy (lm->allocated, lmAllocSaved, sizeof(lmAllocSaved));
}


lightmapBlock_t *LM_NewBlock (void)
{
	lightmapBlock_t *lm;

	// find free slot
	if (++lightmapsNum == MAX_LIGHTMAPS)
		Com_Error (ERR_FATAL, "LM_NewBlock: MAX_LIGHTMAPS hit");
	lm = &lmBlocks[lightmapsNum - 1];
	// init fields
	lm->index = lightmapsNum;
	lm->image = NULL;
	lm->empty = true;
	lm->filled = false;
	// alloc data blocks
	lm->pic = Z_Malloc (LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4);
	lm->allocated = Z_Malloc (LIGHTMAP_SIZE * sizeof(lm->allocated[0]));
	// clear data blocks
	memset (&lm->allocated[0], 0, sizeof(lm->allocated));
	memset (lm->pic, 0, LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4);

	return lm;
}


void LM_Rewind (void)
{
	currentLightmapNum = 0;
}


lightmapBlock_t *LM_NextBlock (void)
{
	lightmapBlock_t *lm;

	while (currentLightmapNum < lightmapsNum)
	{
		lm = &lmBlocks[currentLightmapNum++];
		if (!lm->filled)
			return lm;
	}

	return LM_NewBlock ();
}


qboolean LM_AllocBlock (lightmapBlock_t *lm, dynamicLightmap_t *dl)
{
	int		i, j, w, h;
	int		best, best2;

	w = dl->w2 + 1;					// make a border around lightmap
	h = dl->h + 1;

	best = LIGHTMAP_SIZE;

	for (i = 0; i < LIGHTMAP_SIZE - w; i++)
	{
		best2 = 0;

		for (j = 0; j < w; j++)
		{
			if (lm->allocated[i+j] >= best)
				break;
			if (lm->allocated[i+j] > best2)
				best2 = lm->allocated[i+j];
		}
		if (j == w)
		{	// this is a valid spot
			dl->s = i;
			dl->t = best = best2;
		}
	}

	if (best + h > LIGHTMAP_SIZE)
		return false;

	j = best + h;
	for (i = 0; i < w; i++)
		lm->allocated[dl->s + i] = j;
	dl->block = lm;

	return true;
}


void LM_PutBlock (dynamicLightmap_t *dl)
{
	byte	*dst, *src;
	int		x, y, stride, i, numFast;

	stride = (LIGHTMAP_SIZE - dl->w) * 4;

	/*------------- put main lightmap -------------*/

	dst = dl->block->pic + (dl->t * LIGHTMAP_SIZE + dl->s) * 4;
	src = dl->source[0];

	for (y = 0; y < dl->h; y++)
	{
		for (x = 0; x < dl->w; x++)
		{
			int		r, g, b, m;

			// copy lightmap
			r = *src++;
			*dst++ = r;
			g = *src++;
			*dst++ = g;
			b = *src++;
			*dst++ = b;
			// check minlight
			m = max(r, g);
			m = max(m, b);
			if (m < 192)	// can get ~(0,0,255) etc. after normalization of (min,min,10000) etc.; 192 is a default maxlight for qrad3
			{
				if (lmMinlight.c[0] > r) lmMinlight.c[0] = r;
				if (lmMinlight.c[1] > g) lmMinlight.c[1] = g;
				if (lmMinlight.c[2] > b) lmMinlight.c[2] = b;
			}
			// alpha (flag: no additional scale)
			*dst++ = 0;
		}
		dst += stride;
	}

	/*----------- put dynamic lightmaps -----------*/

	numFast = 0;
	for (i = 0; i < dl->numStyles; i++)
	{
		if (!IS_FAST_STYLE(dl->style[i])) continue;

		numFast++;
		dst = dl->block->pic + (dl->t * LIGHTMAP_SIZE + dl->s + dl->w * numFast) * 4;
		src = dl->source[i];

		for (y = 0; y < dl->h; y++)
		{
			for (x = 0; x < dl->w; x++)
			{
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = 255;		// alpha (flag: modulate by 2)
			}
			dst += stride;
		}
	}

	// mark lightmap block as used
	dl->block->empty = false;
}


// perform selection sort on lightstyles (mostly not needed, just in case)
void LM_SortLightStyles (dynamicLightmap_t *dl)
{
	int		i, j;

	if (dl->numStyles < 2) return;	// nothing to sort

	for (i = 0; i < dl->numStyles - 1; i++)	// at iteration "i == dl->numStyles-1" all already will be sorted
	{
		int		min;
		byte	*source, style;

		min = i;
		for (j = i + 1; j < dl->numStyles; j++)
			if (dl->style[j] < dl->style[i]) min = j;
		if (min == i) continue;		// in place
		// exchange styles [i] and [j]
		source = dl->source[i];
		dl->source[i] = dl->source[j];
		dl->source[j] = source;
		style = dl->style[i];
		dl->style[i] = dl->style[j];
		dl->style[j] = style;
	}
}


void GL_UpdateDynamicLightmap (shader_t *shader, surfacePlanar_t *surf, qboolean vertexOnly, unsigned dlightMask)
{
	byte	pic[LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4];
	int		x, z;
	dynamicLightmap_t *dl;

	dl = surf->lightmap;

	if (dl->block && !vertexOnly)
	{
		/*------------- update regular lightmap ---------------*/
		memset (pic, 0, dl->w * dl->h * 4);	// set initial state to zero (add up to 4 lightmaps)
		for (z = 0; z < dl->numStyles; z++)
		{
			int		scale, count;
			byte	*src, *dst;

			if (IS_FAST_STYLE(dl->style[z])) continue;
			src = dl->source[z];
			dst = pic;
			scale = dl->modulate[z] >> gl_config.overbrightBits;
			if (gl_config.lightmapOverbright) scale <<= 1;

			count = dl->w * dl->h;
			for (x = 0; x < count; x++)
			{
				// modulate lightmap: scale==0 -> by 0, scale==128 - by 1; scale==255 - by 2 (or, to exact, 2.0-1/256)
				int		r, g, b;

				r = dst[0] + (scale * *src++ >> 7);
				g = dst[1] + (scale * *src++ >> 7);
				b = dst[2] + (scale * *src++ >> 7);
				NORMALIZE_COLOR255(r, g, b);
				*dst++ = r; *dst++ = g; *dst++ = b;
				dst++;
			}
		}
		GL_Bind (dl->block->image);
		qglTexSubImage2D (GL_TEXTURE_2D, 0, dl->s, dl->t, dl->w, dl->h, GL_RGBA, GL_UNSIGNED_BYTE, pic);
		gl_speeds.numUploads++;
	}
//??	else
	{
		/*-------------- update vertex colors --------------*/
		vertex_t *v;
		int		i;

		for (i = 0, v = surf->verts; i < surf->numVerts; i++, v++)
		{
			unsigned r, g, b;			// 0 -- 0.0f, 256*16384*128 -- 256 (rang=21+8)

			r = g = b = 0;
			for (z = 0; z < dl->numStyles; z++)
			{
				byte	*point;
				unsigned scale;			// 0 -- 0.0f, 128 -- 1.0f, 255 ~ 2.0 (rang=7)
				unsigned frac_s, frac_t;// 0 -- 0.0f, 128 -- 1.0f (rang=7)
				unsigned frac;			// point frac: 0 -- 0.0f, 16384*128 -- 1.0f (rang=14+7=21)

				// calculate vertex color as weighted average of 4 points
				scale = dl->modulate[z] * 2 >> gl_config.overbrightBits;
//				point = dl->source[z] + ((int)v->lm2[1] * dl->w + (int)v->lm2[0]) * 3;
				point = dl->source[z] + (Q_ftol (v->lm2[1]) * dl->w + Q_ftol (v->lm2[0])) * 3;
				// calculate s/t weights
				frac_s = Q_ftol (v->lm2[0] * 128) & 127;
				frac_t = Q_ftol (v->lm2[1] * 128) & 127;
#define STEP(c,n)	c += point[n] * frac;
				frac = (128 - frac_s) * (128 - frac_t) * scale;
				STEP(r, 0); STEP(g, 1); STEP(b, 2);
				frac = frac_s * (128 - frac_t) * scale;
				STEP(r, 3); STEP(g, 4); STEP(b, 5);
				point += dl->w * 3;		// next line
				frac = (128 - frac_s) * frac_t * scale;
				STEP(r, 0); STEP(g, 1); STEP(b, 2);
				frac = frac_s * frac_t * scale;
				STEP(r, 3); STEP(g, 4); STEP(b, 5);
#undef STEP
			}
			// "fixed" -> int
			r >>= 21;
			g >>= 21;
			b >>= 21;

			NORMALIZE_COLOR255(r, g, b);
			v->c.c[0] = r;
			v->c.c[1] = g;
			v->c.c[2] = b;
		}
		// apply vertex dlights
		if (dlightMask)
		{
			surfDlight_t *sdl;
			refDlight_t *dlight;

			sdl = surf->dlights;
			for ( ; dlightMask; dlightMask >>= 1)
			{
				float	intens2;

				if (!(dlightMask & 1)) continue;
				dlight = sdl->dlight;
				intens2 = sdl->radius * sdl->radius;

				for (i = 0, v = surf->verts; i < surf->numVerts; i++, v++)
				{
					float	f1, f2, dist2;
					int		intens, r, g, b;

					f1 = sdl->pos[0] - v->pos[0];
					f2 = sdl->pos[1] - v->pos[1];
					dist2 = f1 * f1 + f2 * f2;
					if (dist2 >= intens2) continue;			// vertex is too far from dlight

					intens = Q_ftol ((1 - dist2 / intens2) * 256);
					r = v->c.c[0] + (dlight->c.c[0] * intens >> 8);
					g = v->c.c[1] + (dlight->c.c[1] * intens >> 8);
					b = v->c.c[2] + (dlight->c.c[2] * intens >> 8);
					NORMALIZE_COLOR255(r, g, b);
					v->c.c[0] = r;
					v->c.c[1] = g;
					v->c.c[2] = b;
				}
				sdl++;
			}
		}
	}
}
