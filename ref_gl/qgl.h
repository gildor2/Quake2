#ifndef __QGL_H__
#define __QGL_H__

#ifdef _WIN32
// need this include, because have wgl and GDI functions here
#  include <windows.h>
#endif

#include "gl.h"

#ifdef __linux__
//#include <GL/fxmesa.h>
#include <GL/glx.h>
#endif

qboolean QGL_Init (const char *dllname);
void	QGL_InitExtensions (void);
void	QGL_Shutdown (void);
void	GLimp_EnableLogging (qboolean enable);

#ifndef APIENTRY
#  define APIENTRY
#endif


#include "qgl_decl.h"


#ifdef __linux__

// local function in dll
extern void *qwglGetProcAddress(char *symbol);

extern void (*qgl3DfxSetPaletteEXT)(GLuint *);

/*
//FX Mesa Functions
extern fxMesaContext (*qfxMesaCreateContext)(GLuint win, GrScreenResolution_t, GrScreenRefresh_t, const GLint attribList[]);
extern fxMesaContext (*qfxMesaCreateBestContext)(GLuint win, GLint width, GLint height, const GLint attribList[]);
extern void (*qfxMesaDestroyContext)(fxMesaContext ctx);
extern void (*qfxMesaMakeCurrent)(fxMesaContext ctx);
extern fxMesaContext (*qfxMesaGetCurrentContext)(void);
extern void (*qfxMesaSwapBuffers)(void);
*/

//GLX Functions
extern XVisualInfo * (*qglXChooseVisual) (Display *dpy, int screen, int *attribList);
extern GLXContext (*qglXCreateContext) (Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct);
extern void (*qglXDestroyContext) (Display *dpy, GLXContext ctx);
extern Bool (*qglXMakeCurrent) (Display *dpy, GLXDrawable drawable, GLXContext ctx);
extern void (*qglXCopyContext) (Display *dpy, GLXContext src, GLXContext dst, GLuint mask);
extern void (*qglXSwapBuffers) (Display *dpy, GLXDrawable drawable);

// 3dfxSetPaletteEXT shunt
void Fake_glColorTableEXT (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);

#endif // linux

#endif
