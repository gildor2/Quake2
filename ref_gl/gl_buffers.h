#ifndef __GL_BUFFERS_INCLUDED__
#define __GL_BUFFERS_INCLUDED__


#include "gl_frontend.h"


void	*GL_AllocDynamicMemory (int size);
void	GL_ResizeDynamicMemory (void *ptr, int newSize);
void	GL_ClearBuffers (void);
void	GL_ClearPortal (void);
void	GL_AddSurfaceToPortal (surfaceCommon_t *surf, shader_t *shader, int entityNum, int numDlights);
surfaceCommon_t *GL_AddDynamicSurface (shader_t *shader, int entityNum);
void	GL_InsertShaderIndex (int index);
void	GL_FinishPortal (void);
void	GL_SortSurfaces (viewPortal_t *port, surfaceInfo_t **destination);


void	GL_CreateBuffers (void);
void	GL_FreeBuffers (void);

#endif
