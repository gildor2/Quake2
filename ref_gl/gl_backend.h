#ifndef __GL_BACKEND_INCLUDED__
#define __GL_BACKEND_INCLUDED__


#define MAX_BACKEND_COMMANDS	(64 * 1024)		// test results: 33K usage for 1600x1200 with a full console
extern byte backendCommands[MAX_BACKEND_COMMANDS];
extern int	backendCmdSize;
extern byte *lastBackendCommand;

#define MAX_SCENE_SURFACES	(64*1024)

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
		GL_BackEnd ();	\
	varname = (type *) &backendCommands[backendCmdSize];	\
	lastBackendCommand = (byte*) varname;	\
	backendCmdSize += sizeof(type);

// same as PUT_BACKEND_COMMAND, but with additional reserving of "size" bytes
#define PUT_BACKEND_COMMAND2(type,varname,size) \
	type *varname;	\
	if (backendCmdSize+sizeof(type)+size+4 > MAX_BACKEND_COMMANDS)	\
		GL_BackEnd ();	\
	varname = (type *) &backendCommands[backendCmdSize];	\
	lastBackendCommand = (byte*) varname;	\
	backendCmdSize += sizeof(type) + size;

__inline qboolean GET_BACKEND_SPACE (int size)
{
	if (backendCmdSize+size+4 > MAX_BACKEND_COMMANDS)
	{
		GL_BackEnd ();
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
	float	x, y, w, h;
	float	s1, t1, s2, t2;
	color_t	c;
} bkDrawPic_t;


typedef struct
{
	int		type;			// BACKEND_TEXT
	float	x, y;			// position on screen
	float	w, h;			// character output size
	color_t	c;
	byte	len;
	char	text[0];		// [len]
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


#endif
