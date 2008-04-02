#ifndef __GL_BACKEND_INCLUDED__
#define __GL_BACKEND_INCLUDED__


#include "gl_model.h"

namespace OpenGLDrv {


// per-shader limits
#define MAX_VERTEXES	4096		// 1024 in Q3, 2048 in Q2, but have VALID Q2 model with more than 2048 verts
#define MAX_INDEXES		(MAX_VERTEXES*6)


struct bufVertex_t
{
	CVec3		xyz;
	float		pad;
};


struct bufTexCoordSrc_t
{
	float		tex[2];
	float		lm[2];
};


struct bufTexCoord_t
{
	float		tex[2];
};

// some data may be grouped by few vertexes
struct bufExtra_t
{
	int			numVerts;
	int			lmWidth;
	CVec3		normal;
	CVec3		*axis;				// CVec3[2]
	planeDlight_t   *pDlight;
	trisurfDlight_t *tDlight;
};


struct vertexBuffer_t
{
	bufVertex_t		verts[MAX_VERTEXES];
	color_t			color[MAX_VERTEXES];
	bufTexCoord_t	texCoord[1][MAX_VERTEXES];
	static inline int getSize(int numTmu)
	{
		return sizeof(vertexBuffer_t) + (numTmu-1) * sizeof(bufTexCoord_t) * MAX_VERTEXES;	// 1 texCoord already reserved
	}
};

extern vertexBuffer_t *vb;			// pointers to current buffer


extern int gl_numVerts, gl_numIndexes, gl_numExtra;
extern int			gl_indexesArray[MAX_INDEXES];
extern bufExtra_t	gl_extra[MAX_VERTEXES];

extern color_t		srcVertexColor[MAX_VERTEXES];
extern bufTexCoordSrc_t srcTexCoord[MAX_VERTEXES];


#define MAX_SCENE_SURFACES	(64*1024)


/*-------------------- Functions -------------------------------*/

void BK_Init();
void BK_Shutdown();

void BK_BeginFrame();
void BK_EndFrame();

void BK_DrawPic(shader_t *shader, int x, int y, int w, int h,
				float s1 = 0, float t1 = 0, float s2 = 1, float t2 = 1,
				unsigned color = RGB(1,1,1), byte flipMode = 0);
void BK_DrawText(const char *text, int len, int x, int y, int w, int h, unsigned color);
void BK_DrawScene();
void BK_FlushShader();


/*----------- Surface info bits (for scene sorting) ------------*/

#define ENTITYNUM_BITS		10
#define ENTITYNUM_MASK		((1<<ENTITYNUM_BITS) - 1)

#define SHADERNUM_BITS		12
#define SHADERNUM_MASK		((1<<SHADERNUM_BITS) - 1)

#define DLIGHTNUM_BITS		2
#define DLIGHTNUM_MASK		((1<<DLIGHTNUM_BITS) - 1)

#define FOGNUM_BITS			8
#define FOGNUM_MASK			((1<<FOGNUM_BITS) - 1)

// total: 32 bits

#define MAX_ENTITYNUM		(1<<ENTITYNUM_BITS)
#define ENTITYNUM_WORLD		(MAX_ENTITYNUM-2)
#define ENTITYNUM_NONE		(MAX_ENTITYNUM-1)

#define MAX_GLENTITIES		(MAX_ENTITYNUM-1)	// MAX_ENTITIES defined in ref.h

// Bits layout: fogNum, shaderNum, entityNum, dlightNum
#define DLIGHTNUM_SHIFT		0
#define FOGNUM_SHIFT		(DLIGHTNUM_SHIFT+DLIGHTNUM_BITS)
#define ENTITYNUM_SHIFT		(FOGNUM_SHIFT   +FOGNUM_BITS   )
#define SHADERNUM_SHIFT		(ENTITYNUM_SHIFT+ENTITYNUM_BITS)

#if SHADERNUM_SHIFT + SHADERNUM_BITS > 32
#error Sort key overflow
#endif


} // namespace

#endif
