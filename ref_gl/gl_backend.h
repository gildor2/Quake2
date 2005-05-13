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


#define MAX_BACKEND_COMMANDS	(64 * 1024)		// test results: 33K usage for 1600x1200 with a full console
extern byte backendCommands[MAX_BACKEND_COMMANDS];
extern int	backendCmdSize;
extern byte *lastBackendCommand;

#define MAX_SCENE_SURFACES	(64*1024)


/*-------------------- Functions -------------------------------*/

void	BackEnd (void);

void	InitBackend (void);
void	ShutdownBackend (void);

void	DrawStretchPic (shader_t *shader, int x, int y, int w, int h,
			float s1 = 0, float t1 = 0, float s2 = 1, float t2 = 1,
			unsigned color = RGB(1,1,1), byte flipMode = 0);


/*-------- Macro for easy generating backend commands ----------*/

/* should be placed in {}
 * Usage:
 *		{
 *			PUT_BACKEND_COMMAND(bkSomeType_t, bkVar);
 *			bkVar->type = BACKEND_SOME_TYPE;
 *			bkVar->field = data;
 *			...
 *		}
 * Note:
 *   used "+sizeof(type)+4" -- 4 is sizeof(BACKEND_STOP) -- reserve
 *   place for "stop" command.
 */

#define PUT_BACKEND_COMMAND(type,varname) \
	type *varname;	\
	if (backendCmdSize+sizeof(type)+4 > MAX_BACKEND_COMMANDS)	\
		BackEnd ();	\
	varname = (type *) &backendCommands[backendCmdSize];	\
	lastBackendCommand = (byte*) varname;	\
	backendCmdSize += sizeof(type);

// same as PUT_BACKEND_COMMAND, but with additional reserving of "size" bytes
#define PUT_BACKEND_COMMAND2(type,varname,size) \
	type *varname;	\
	if (backendCmdSize+sizeof(type)+size+4 > MAX_BACKEND_COMMANDS)	\
		BackEnd ();	\
	varname = (type *) &backendCommands[backendCmdSize];	\
	lastBackendCommand = (byte*) varname;	\
	backendCmdSize += sizeof(type) + size;

inline bool GET_BACKEND_SPACE (int size)
{
	if (backendCmdSize+size+4 > MAX_BACKEND_COMMANDS)
	{
		BackEnd ();
		return false;
	}
	backendCmdSize += size;
	return true;
}



/*-------------------- Backend commands ------------------------*/

typedef enum
{
	BACKEND_STOP,			// placed automatically, no params
	BACKEND_PIC,			// DrawStretchPic()
	BACKEND_TEXT,			// draw colored text; any output size
	BACKEND_BEGIN_FRAME,	// no params
	BACKEND_DRAW_FRAME,		// params: portal
	BACKEND_END_FRAME		// no params
} bkCommandType_t;


typedef struct
{
	int		type;			// BACKEND_PIC
	shader_t *shader;
	short	x, y, w, h;
	float	s1, t1, s2, t2;
	byte	flipMode;		// 1 = swap s1-s2; 2 = swap t1-t2; 4 = swap s-t
	color_t	c;
} bkDrawPic_t;


typedef struct
{
	int		type;			// BACKEND_TEXT
	byte	len;
	short	x, y;			// position on screen
	short	w, h;			// character output size
	color_t	c;
	char	text[1];		// [len]
} bkDrawText_t;


typedef struct
{
	int		type;			// BACKEND_DRAW_FRAME
	viewPortal_t portal;
} bkDrawFrame_t;


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
