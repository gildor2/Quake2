#ifndef __GL_BACKEND_INCLUDED__
#define __GL_BACKEND_INCLUDED__


#define MAX_BACKEND_COMMANDS	(256 * 1024)
extern byte backendCommands[MAX_BACKEND_COMMANDS];
extern int	backendCmdSize;


/*-- Macro for easy generating backend commands --*/

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
		GL_BackEnd ();	\
	varname = (type *) &backendCommands[backendCmdSize];	\
	backendCmdSize += sizeof(type);

/*--------------- Backend commands ---------------*/

typedef enum
{
	BACKEND_STOP,			// placed automatically, no params
	BACKEND_PIC,			// DrawStretchPic()
	BACKEND_SET_COLOR,		// SetColor()
	BACKEND_BEGIN_FRAME,	// no params (??)
	BACKEND_DRAW_FRAME,		// params: portal
	BACKEND_END_FRAME		// no params
} bkCommandType_t;


typedef struct
{
	int		type;	// BACKEND_PIC
	shader_t *shader;
	float	x, y, w, h;
	float	s1, t1, s2, t2;
} bkDrawPic_t;


typedef struct
{
	int		type;	// BACKEND_COLOR
	union {
		byte	c[4];
		int		rgba;
	};
} bkSetColor_t;


typedef struct
{
	int		type;	// BACKEND_DRAW_FRAME
	viewPortal_t portal;
} bkDrawFrame_t;


/*---------- Surface info bits (for scene sorting) -----------*/

#define ENTITYNUM_BITS		10
#define ENTITYNUM_MASK		((1<<ENTITYNUM_BITS) - 1)

#define SHADERNUM_BITS		10
#define SHADERNUM_MASK		((1<<SHADERNUM_BITS) - 1)


#define MAX_ENTITYNUM		(1<<ENTITYNUM_BITS)
#define ENTITYNUM_WORLD		(MAX_ENTITYNUM-2)
#define ENTITYNUM_NONE		(MAX_ENTITYNUM-1)

#define MAX_GLENTITIES		(MAX_ENTITYNUM-1)	// MAX_ENTITIES defined in ref.h

// Bits layout: shaderNum, entityNum
// Give priority to shader sort: shaders with alpha will be rendered after others etc.
#define ENTITYNUM_SHIFT		0
#define SHADERNUM_SHIFT		(ENTITYNUM_SHIFT+ENTITYNUM_BITS)


#endif
