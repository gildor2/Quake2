#ifndef __GL_LIGHTMAP_INCLUDED__
#define __GL_LIGHTMAP_INCLUDED__


#define MAX_LIGHTMAPS	64		// max lightmap textures
#define LIGHTMAP_SIZE	256		// width and height of the lightmap texture (square)

typedef struct
{
	int		index;				// "*lightmap%d"
	image_t	*image;				// required for uploading (updating) lightmap
	int		*allocated;			// [LIGHTMAP_SIZE]
	byte	*pic;				// [LIGHTMAP_SIZE*LIGHTMAP_SIZE*4]
	qboolean empty;
	qboolean filled;
} lightmapBlock_t;


#define IS_FAST_STYLE(s)	(s >= 1 && s <= 31 && gl_config.multiPassLM)

typedef struct dynamicLightmap_s
{
	// style info
	int		numStyles;			// 1..4
	int		numFastStyles;		// number of IS_FAST_STYLE styles
	byte	style[4];			// style numbers
	byte	modulate[4];		// current (uploaded) state of styles
	// source
	byte	*source[4];			// points to "byte rgb[w*h][3]"
	int		w, h;				// size of source
	int		w2;					// width of whole lightmap block (including fast dynamic lightmaps)
	// info for uploading lightmap
	int		s, t;				// glTexSubimage to (s,t)-(s+w,t+h)
	lightmapBlock_t *block;
} dynamicLightmap_t;


void LM_Init (void);
void LM_Flush (lightmapBlock_t *lm);
void LM_Done (void);
void LM_Save (lightmapBlock_t *lm);
void LM_Restore (lightmapBlock_t *lm);
lightmapBlock_t *LM_NewBlock (void);
void LM_Rewind (void);
lightmapBlock_t *LM_NextBlock (void);
qboolean LM_AllocBlock (lightmapBlock_t *lm, dynamicLightmap_t *dl);
void LM_PutBlock (dynamicLightmap_t *dl);
void LM_SortLightStyles (dynamicLightmap_t *dl);
void GL_UpdateDynamicLightmap (shader_t *shader, surfacePlanar_t *surf, qboolean vertexOnly, unsigned dlightMask);


extern color_t lmMinlight;


#endif
