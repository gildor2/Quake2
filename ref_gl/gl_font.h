#ifndef __GL_FONT_INCLUDED__
#define __GL_FONT_INCLUDED__


namespace OpenGLDrv {


class CFont : public CBasicFont
{
public:
	shader_t	*shader;
	int			firstChar;
	int			outWidth;
	int			outHeight;
	int			spacing;		// internal spacing between chars in font texture

	bool Load(const char *name);
	void GetTextExtents(const char *s, int &width, int &height);
};



extern CFont *gl_concharsFont;
extern CFont *gl_debugFont;
extern CFont *gl_consoleFont;


void InitFonts();
void ShutdownFonts();
void ResetFonts();

// will return "conchars" if will not find a font
CFont* FindFont(const char* name);


} // namespace


#endif // __GL_FONT_INCLUDED__
