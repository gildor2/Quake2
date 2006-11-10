#ifndef __GL_WIN_H__
#define __GL_WIN_H__

namespace OpenGLDrv {

extern HDC gl_hDC;		// handle to device context
// ... here for wglGetExtensionsStringARB(gl_hDC) and wglSwapBuffers(gl_hDC) only!

} // namespace

#endif
