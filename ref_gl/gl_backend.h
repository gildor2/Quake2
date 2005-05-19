#ifndef __GL_BACKEND_INCLUDED__
#define __GL_BACKEND_INCLUDED__


#include "gl_model.h"

namespace OpenGLDrv {


// per-shader limits
#define MAX_VERTEXES	4096		// 1024 in Q3, 2048 in Q2, but have VALID model with more than 2048 verts
#define MAX_INDEXES		(MAX_VERTEXES*6)


typedef struct
{
	float	xyz[3];
	float	pad;
} bufVertex_t;


typedef struct
{
	float	tex[2];
} bufTexCoord_t;

// some data may be grouped by few vertexes
typedef struct
{
	int		numVerts;
	int		lmWidth;
	vec3_t	normal;
	vec3_t	*axis;
	surfDlight_t *dlight;
} bufExtra_t;


extern int gl_numVerts, gl_numIndexes, gl_numExtra;
extern int			gl_indexesArray[MAX_INDEXES];
extern bufExtra_t	gl_extra[MAX_VERTEXES];


#define MAX_SCENE_SURFACES	(64*1024)


/*-------------------- Functions -------------------------------*/

void	BK_BeginFrame ();
void	BK_EndFrame ();

void	InitBackend (void);
void	ShutdownBackend (void);

void	BK_DrawPic (shader_t *shader, int x, int y, int w, int h,
			float s1 = 0, float t1 = 0, float s2 = 1, float t2 = 1,
			unsigned color = RGB(1,1,1), byte flipMode = 0);
void	BK_DrawText (const char *text, int len, int x, int y, int w, int h, unsigned color);
void	BK_DrawScene ();


/*----------- Surface info bits (for scene sorting) ------------*/

#define ENTITYNUM_BITS		10
#define ENTITYNUM_MASK		((1<<ENTITYNUM_BITS) - 1)

#define SHADERNUM_BITS		10
#define SHADERNUM_MASK		((1<<SHADERNUM_BITS) - 1)

#define DLIGHTNUM_BITS		2
#define DLIGHTNUM_MASK		((1<<DLIGHTNUM_BITS) - 1)


#define MAX_ENTITYNUM		(1<<ENTITYNUM_BITS)
#define ENTITYNUM_WORLD		(MAX_ENTITYNUM-2)
#define ENTITYNUM_NONE		(MAX_ENTITYNUM-1)

#define MAX_GLENTITIES		(MAX_ENTITYNUM-1)	// MAX_ENTITIES defined in ref.h

// Bits layout: shaderNum, entityNum, dlightNum
#define DLIGHTNUM_SHIFT		0
#define ENTITYNUM_SHIFT		(DLIGHTNUM_SHIFT+DLIGHTNUM_BITS)
#define SHADERNUM_SHIFT		(ENTITYNUM_SHIFT+ENTITYNUM_BITS)


} // namespace

#endif
