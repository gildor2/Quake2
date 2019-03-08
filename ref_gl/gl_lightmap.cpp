#include "OpenGLDrv.h"
#include "gl_model.h"
#include "gl_lightmap.h"
#include "gl_image.h"
#include "gl_math.h"

//#define LM_DEBUG	1		// colorize uninitialized/placed lightmaps

namespace OpenGLDrv {


static int lightmapsNum, currentLightmapNum;
static lightmapBlock_t lmBlocks[MAX_LIGHTMAPS];

static int lmAllocSaved[LIGHTMAP_SIZE];

color_t lmMinlight;


void LM_Init()
{
	lightmapsNum = currentLightmapNum = 0;
	lmMinlight.rgba = RGBA(1,1,1,1);
}


void LM_Flush(lightmapBlock_t *lm)
{
	if (!lm->empty && !lm->filled)
	{
		// have valid not stored lightmap -- upload it
		lm->image = CreateImage(va("*lightmap%d", lm->index), lm->pic, LIGHTMAP_SIZE, LIGHTMAP_SIZE,
			IMAGE_CLAMP|IMAGE_LIGHTMAP);
		lm->filled = true;
	}
	if (lm->pic)
	{
		// free memory blocks
		delete lm->pic;
		delete lm->allocated;
		lm->pic = NULL;
	}
}


void LM_Done()
{
	for (int i = 0; i < lightmapsNum; i++)
		LM_Flush(&lmBlocks[i]);
}


void LM_Save(lightmapBlock_t *lm)
{
	memcpy(lmAllocSaved, lm->allocated, sizeof(lmAllocSaved));
}


void LM_Restore(lightmapBlock_t *lm)
{
	memcpy(lm->allocated, lmAllocSaved, sizeof(lmAllocSaved));
}


lightmapBlock_t *LM_NewBlock()
{
	// find free slot
	if (++lightmapsNum == MAX_LIGHTMAPS)
		appError("LM_NewBlock: MAX_LIGHTMAPS hit");
	lightmapBlock_t &lm = lmBlocks[lightmapsNum - 1];
	// init fields
	lm.index     = lightmapsNum;
	lm.image     = NULL;
	lm.empty     = true;
	lm.filled    = false;
	// alloc data blocks
	lm.pic       = new byte [LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4];
	lm.allocated = new int [LIGHTMAP_SIZE];

#if LM_DEBUG
	byte *p = lm.pic;
	for (int i = 0; i < LIGHTMAP_SIZE*LIGHTMAP_SIZE; i++)
	{
		*p++=128; *p++=16; *p++=16; *p++=255;
	}
#endif

	return &lm;
}


void LM_Rewind()
{
	currentLightmapNum = 0;
}


lightmapBlock_t *LM_NextBlock()
{
	while (currentLightmapNum < lightmapsNum)
	{
		lightmapBlock_t &lm = lmBlocks[currentLightmapNum++];
		if (!lm.filled)
			return &lm;
	}

	return LM_NewBlock();
}


void LM_CheckMinlight(dynamicLightmap_t *dl)
{
	const byte *src = dl->source[0];
	if (!src) return; // seems dl->numStyles == 0 here
	for (int i = 0; i < dl->w * dl->h; i++)
	{
		int r, g, b, m;
		r = *src++; g = *src++; b = *src++;
		m = max(r, g);
		m = max(m, b);
		if (m < 192)	// can get ~(0,0,255) etc. after normalization of (min,min,10000) etc.; 192 is a default maxlight for qrad3
		{
			if (lmMinlight.c[0] > r) lmMinlight.c[0] = r;
			if (lmMinlight.c[1] > g) lmMinlight.c[1] = g;
			if (lmMinlight.c[2] > b) lmMinlight.c[2] = b;
		}
	}
}


bool LM_AllocBlock(lightmapBlock_t *lm, dynamicLightmap_t *dl)
{
	guardSlow(LM_AllocBlock);
	assert(lm);
	assert(dl);

	int w = dl->w2 + 1;					// make a border around lightmap
	int h = dl->h + 1;

	int best = LIGHTMAP_SIZE;

	int i, j;
	for (i = 0; i < LIGHTMAP_SIZE - w; i++)
	{
		int best2 = 0;

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
	unguardSlow;
}


// Helper function for creating lightmaps with r_lightmap=2 (taken from Q3)
// Converts brightness to color
static void CreateSolarColor(float light, float x, float y, float *vec)
{
	float f = light * 5;
	int i = appCeil(f);
	f = f - i;				// frac part

	float s = (1.0f - x) * y;
	float t = (1.0f - f * x) * y;
	float a0 = y * (1.0f - x * (1.0f - f));
	switch (i)
	{
		case 0:
			vec[0] = y;  vec[1] = a0; vec[2] = s;
			break;
		case 1:
			vec[0] = t;  vec[1] = y;  vec[2] = s;
			break;
		case 2:
			vec[0] = s;  vec[1] = y;  vec[2] = a0;
			break;
		case 3:
			vec[0] = s;  vec[1] = t;  vec[2] = y;
			break;
		case 4:
			vec[0] = a0; vec[1] = s;  vec[2] = y;
			break;
		case 5:
			vec[0] = y;  vec[1] = s;  vec[2] = t;
			break;
	}
}


// Copy (and light/color scale) lightmaps from src to dst (samples*3 bytes)
static void CopyLightmap(byte *dst, byte *src, int w, int h, int stride, byte a)
{
	int		x, y;

#if !NO_DEBUG
	if (r_lightmap->integer == 2)
	{
		for (y = 0; y < h; y++)
		{
			for (x = 0; x < w; x++)
			{
				float vec[3], r, g, b, light;

				r = *src++; g = *src++; b = *src++;

				light = r * 0.33f + g * 0.685f + b * 0.063f;
				if (light > 255)
					light = 1;
				else
					light = 1.0f / light;

				CreateSolarColor(light, 1, 0.5, vec);
				*dst++ = appFloor(vec[0] * 255);
				*dst++ = appFloor(vec[1] * 255);
				*dst++ = appFloor(vec[2] * 255);
				*dst++ = a;
			}
			dst += stride;
		}
	}
	else
#endif
	{
		float sat = r_saturation->value;
#if !NO_DEBUG
		if (r_lightmap->integer == 3)
			sat = -sat;
#endif

		for (y = 0; y < h; y++)
		{
			for (x = 0; x < w; x++)
			{
#if !LM_DEBUG
				if (sat != 1.0f)
				{
					float	r, g, b, light;
					// get color
					r = *src++;  g = *src++;  b = *src++;
					// change saturation
					light = (r + g + b) / 3.0f;
					SATURATE(r,light,sat);
					SATURATE(g,light,sat);
					SATURATE(b,light,sat);
					// put color
					*dst++ = appRound(r); *dst++ = appRound(g); *dst++ = appRound(b);
				}
				else
				{
					// copy color
					*dst++ = *src++;
					*dst++ = *src++;
					*dst++ = *src++;
				}
#else
				*dst++=32; *dst++=128; *dst++=128;
#endif
				*dst++ = a;
			}
			dst += stride;
		}
	}
}


// copy non-formatted lightmap data
void LM_Copy(byte *dst, byte *src, int size, int overbright)
{
	CopyLightmap(dst, src, size, 1, 0, overbright);
}


void LM_PutBlock(dynamicLightmap_t *dl)
{
	guardSlow(LM_PutBlock);
	assert(dl);

	if (dl->source[0] == NULL) return; // numStyles may be 0 for HL

	int stride = (LIGHTMAP_SIZE - dl->w) * 4;

	// put main lightmap (style 0, slow)
	byte *dst = dl->block->pic + (dl->t * LIGHTMAP_SIZE + dl->s) * 4;
	CopyLightmap(dst, dl->source[0], dl->w, dl->h, stride, 0);		// alpha (flag: no additional scale)

	// put fast dynamic lightmaps
	int numFast = 0;
	for (int i = 0; i < dl->numStyles; i++)
	{
		if (!IS_FAST_STYLE(dl->style[i])) continue;

		numFast++;
		dst = dl->block->pic + (dl->t * LIGHTMAP_SIZE + dl->s + dl->w * numFast) * 4;
		// alpha (flag: modulate by 2)
		// required, because backend will modulate texture by 0..1, which corresponds lightstyle=0..2
		CopyLightmap(dst, dl->source[i], dl->w, dl->h, stride, 255);
	}

	// mark lightmap block as used
	dl->block->empty = false;
	unguardSlow;
}


// monochrome lightmap
static void CopyLightmap1(byte *dst, byte *src, int w, int h, int stride, byte a)
{
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			// get color (intensity)
			byte b = *src++;
			// put color
#if !LM_DEBUG
			*dst++ = b; *dst++ = b; *dst++ = b;		// put rgb
#else
			*dst++=32; *dst++=32; *dst++=128;
#endif
			*dst++ = a;								// put a
		}
		dst += stride;
	}
}


// similar to LM_PutBlock(), but uses CopyLightmap1() instead of CopyLightmap()
// (monochrome lightmaps, Quake1 map format)
void LM_PutBlock1(dynamicLightmap_t *dl)
{
	guardSlow(LM_PutBlock1);
	assert(dl);
	int stride = (LIGHTMAP_SIZE - dl->w) * 4;

	// put main lightmap (style 0, slow)
	byte *dst = dl->block->pic + (dl->t * LIGHTMAP_SIZE + dl->s) * 4;
	CopyLightmap1(dst, dl->source[0], dl->w, dl->h, stride, 0);		// alpha (flag: no additional scale)

	// put fast dynamic lightmaps
	int numFast = 0;
	for (int i = 0; i < dl->numStyles; i++)
	{
		if (!IS_FAST_STYLE(dl->style[i])) continue;

		numFast++;
		dst = dl->block->pic + (dl->t * LIGHTMAP_SIZE + dl->s + dl->w * numFast) * 4;
		// alpha (flag: modulate by 2)
		// required, because backend will modulate texture by 0..1, which corresponds lightstyle=0..2
		CopyLightmap1(dst, dl->source[i], dl->w, dl->h, stride, 255);
	}

	// mark lightmap block as used
	dl->block->empty = false;
	unguardSlow;
}


// perform selection sort on lightstyles (mostly not needed, just in case)
void LM_SortLightStyles(dynamicLightmap_t *dl)
{
	if (dl->numStyles < 2) return;	// nothing to sort

	for (int i = 0; i < dl->numStyles - 1; i++)	// at iteration "i == dl->numStyles-1" all already will be sorted
	{
		int min = i;
		for (int j = i + 1; j < dl->numStyles; j++)
			if (dl->style[j] < dl->style[i]) min = j;
		if (min == i) continue;		// in place
		// exchange styles [i] and [j]
		Exchange(dl->source[i], dl->source[min]);
		Exchange(dl->style[i],  dl->style[min]);
	}
}


void UpdateDynamicLightmap(surfacePlanar_t *surf, bool vertexOnly, unsigned dlightMask)
{
	guardSlow(UpdateDynamicLightmap);
	assert(surf);
	assert(surf->lightmap);

	byte	pic[LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4];
	int		i, x, z;
	unsigned r, g, b;

	const dynamicLightmap_t *dl = surf->lightmap;

	if (dl->block && !vertexOnly)
	{
		STAT(clock(gl_stats.dynLightmap));
		/*------------- update regular lightmap ---------------*/
		memset(pic, 0, dl->w * dl->h * 4);	// set initial state to zero (add up to 4 lightmaps)
		for (z = 0; z < dl->numStyles; z++)
		{
			if (IS_FAST_STYLE(dl->style[z])) continue;
			const byte *src = dl->source[z];
			byte *dst = pic;
			int scale = dl->modulate[z] >> gl_config.overbright;
			if (!gl_config.doubleModulateLM) scale <<= 1;
			// apply scaled lightmap
			// test results: when using table for "scale*src >> 7" -- computations will be ~1/7 faster
			int count = dl->w * dl->h;
			if (!map.monoLightmap)
				for (x = 0; x < count; x++)
				{
					// modulate lightmap: scale==0 -> by 0, scale==128 - by 1; scale==255 - by 2 (to be exact, 2.0-1/256)
					r = dst[0] + (scale * *src++ >> 7);
					g = dst[1] + (scale * *src++ >> 7);
					b = dst[2] + (scale * *src++ >> 7);
					NORMALIZE_COLOR255(r, g, b);
					*dst++ = r; *dst++ = g; *dst++ = b;
					dst++;
				}
			else
				for (x = 0; x < count; x++)
				{
					// modulate lightmap: scale==0 -> by 0, scale==128 - by 1; scale==255 - by 2 (to be exact, 2.0-1/256)
					r = dst[0] + (scale * *src++ >> 7);
					if (r > 255) r = 255;		// almost 'NORMALIZE' ...
					*dst++ = r; *dst++ = r; *dst++ = r;
					dst++;
				}
		}
		STAT(unclock(gl_stats.dynLightmap));
		//!! WARNING: lightmap is not saturated here
		GL_Bind(dl->block->image);
		glTexSubImage2D(GL_TEXTURE_2D, 0, dl->s, dl->t, dl->w, dl->h, GL_RGBA, GL_UNSIGNED_BYTE, pic);
		STAT(gl_stats.numUploads++);
	}
//??	else
	{
		/*-------------- update vertex colors --------------*/
		vertex_t *v;
		for (i = 0, v = surf->verts; i < surf->numVerts; i++, v++)
		{
			r = g = b = 0;				// 0 -- 0.0f, 256*16384*128 -- 256 (rang=21+8)
			for (z = 0; z < dl->numStyles; z++)
			{
				unsigned scale;			// 0 -- 0.0f, 128 -- 1.0f, 255 ~ 2.0 (rang=7)
				unsigned frac_s, frac_t;// 0 -- 0.0f, 128 -- 1.0f (rang=7)
				unsigned frac;			// point frac: 0 -- 0.0f, 16384*128 -- 1.0f (rang=14+7=21)

				// calculate vertex color as weighted average of 4 points
				scale = dl->modulate[z] * 2 >> gl_config.overbright;
				byte *point = (!map.monoLightmap)
					? dl->source[z] + (appFloor(v->lm2[1]) * dl->w + appFloor(v->lm2[0])) * 3
					: dl->source[z] + (appFloor(v->lm2[1]) * dl->w + appFloor(v->lm2[0]));
				// calculate s/t weights
				frac_s = appRound(v->lm2[0] * 128) & 127;
				frac_t = appRound(v->lm2[1] * 128) & 127;
#define STEP(n)	\
		r += point[n] * frac;	\
		g += point[n+1] * frac;	\
		b += point[n+2] * frac;
				frac = (128 - frac_s) * (128 - frac_t) * scale;
				STEP(0);
				frac = frac_s * (128 - frac_t) * scale;
				STEP(3);
				point += dl->w * 3;		// next line
				frac = (128 - frac_s) * frac_t * scale;
				STEP(0);
				frac = frac_s * frac_t * scale;
				STEP(3);
#undef STEP
			}
			// "fixed" -> int
			r >>= 21;
			g >>= 21;
			b >>= 21;

			if (!map.monoLightmap)
			{
				NORMALIZE_COLOR255(r, g, b);
				v->c.c[0] = r;
				v->c.c[1] = g;
				v->c.c[2] = b;
				SaturateColor4b(&v->c);		//?? better - saturate "dl->source[]"
			}
			else
			{
				// monochrome lightmap: use 1st (single) component only
				// do not worry about using point[1],point[2] in STEP() macro above:
				// it will not point outside of loaded bsp file, and their values are
				// ignored anyway ...
				v->c.c[0] = v->c.c[1] = v->c.c[2] = r;
			}
		}
		// apply vertex dlights
		if (dlightMask)
		{
			const planeDlight_t *sdl = surf->dlights;
			for (i = 0; dlightMask; i++, dlightMask >>= 1)
			{
				if (!(dlightMask & 1)) continue;

				refDlight_t *dlight = vp.dlights + i;
				float intens2 = sdl->radius * sdl->radius;

				for (z = 0, v = surf->verts; z < surf->numVerts; z++, v++)
				{
					float f1 = sdl->pos[0] - v->pos[0];
					float f2 = sdl->pos[1] - v->pos[1];
					float dist2 = f1 * f1 + f2 * f2;
					if (dist2 >= intens2) continue;			// vertex is too far from dlight

					int intens = appRound((1 - dist2 / intens2) * 256);
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

	unguardfSlow(("%s", *surf->shader->Name));
}


// check for single-color lightmap block
bool LM_IsMonotone(dynamicLightmap_t *lm, color_t *avg)
{
	if (lm->numStyles != 1) return false;

#define MAX_DEV		4		// maximal deviation of texture color (4 looks bad with 1-texel lm, but good with vertex lighting)
	//?? MAX_DEV should depend on value ( fabs(v1-v2)/(v1+v2) < MAX_DEV , MAX_DEV--float )

	const byte *p = lm->source[0];
	byte min[3], max[3];
	min[0] = max[0] = *p++;
	min[1] = max[1] = *p++;
	min[2] = max[2] = *p++;

	for (int i = 1; i < lm->w * lm->h; i++, p += 3)
	{
		byte	c;
		bool m = false;
#define STEP(n)	\
		c = p[n];		\
		if (c < min[n])	\
		{				\
			min[n] = c;	\
			m = true;	\
		}				\
		if (c > max[n])	\
		{				\
			max[n] = c;	\
			m = true;	\
		}

		STEP(0); STEP(1); STEP(2);
#undef STEP
		if (m && ((max[0] - min[0] > MAX_DEV) || (max[1] - min[1] > MAX_DEV) || (max[2] - min[2] > MAX_DEV)))
			return false;
	}

	if (avg)
	{
		avg->c[0] = (min[0] + max[0]) / 2;
		avg->c[1] = (min[1] + max[1]) / 2;
		avg->c[2] = (min[2] + max[2]) / 2;
	}

	return true;
}


} // namespace
