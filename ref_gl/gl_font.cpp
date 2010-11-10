#include "OpenGLDrv.h"
#include "gl_shader.h"
#include "gl_font.h"


namespace OpenGLDrv {


#define MAX_FONTS		16
#define FONT_SPACING	1

static CFont fontArray[MAX_FONTS];
int fontCount;


CFont *gl_concharsFont;
CFont *gl_debugFont;
CFont *gl_consoleFont;


CFont* FindFont(const char* name)
{
	// check for loaded font
	for (int i = 0; i < fontCount; i++)
		if (!stricmp(fontArray[i].Name, name))
			return &fontArray[i];
	// load a new font
	assert(fontCount < MAX_FONTS);
	CFont *font = &fontArray[fontCount];
	if (font->Load(name))
	{
		fontCount++;
		return font;
	}

	return gl_consoleFont;		// fallback
}


static void Fontlist_f(bool usage, int argc, char **argv)
{
	appPrintf("--w---h---name---------\n");
	for (int i = 0; i < fontCount; i++)
	{
		const CFont &font = fontArray[i];
		appPrintf("%3d %3d   %s\n", font.charWidth, font.charHeight, *font.Name);
	}
}


void InitFonts()
{
	RegisterCommand("fontlist", Fontlist_f);
	ResetFonts();
}


void ShutdownFonts()
{
	UnregisterCommand("fontlist");
}


void ResetFonts()
{
	fontCount = 0;

	CFont *f;
	// hardcoded font fot "conchars" - uses different shader flags, plus different texture location
	f = gl_concharsFont = &fontArray[fontCount++];
	f->Name         = "conchars";
	f->shader       = FindShader("pics/conchars", SHADER_ALPHA);
	f->firstChar    = 0;
	f->charWidth    = 8;
	f->charHeight   = 8;
	f->outWidth     = 8;
	f->outHeight    = 8;
	f->spacing      = 0;
	// load other fonts
	gl_debugFont   = FindFont("debug");
	gl_consoleFont = FindFont("console");
}


static CFont *loadingFont;

static void cBitmap(int argc, char **argv)
{
	loadingFont->shader = FindShader(va("fonts/%s", argv[1]), SHADER_MAKEALPHA);
}

static void cFirstChar(int argc, char **argv)
{
	loadingFont->firstChar = atoi(argv[1]);
}

static void cCharSize(int argc, char **argv)
{
	loadingFont->charWidth  = loadingFont->outWidth  = atoi(argv[1]) - FONT_SPACING;
	loadingFont->charHeight = loadingFont->outHeight = atoi(argv[2]) - FONT_SPACING;
}

static const CSimpleCommand fontCommands[] = {
	{"bitmap",    cBitmap    },
	{"firstChar", cFirstChar },
	{"charSize",  cCharSize  }
};


bool CFont::Load(const char *name)
{
	TString<64> Filename;
	Filename.sprintf("fonts/%s.font", name);
	char* buf = (char*)GFileSystem->LoadFile(Filename);
	if (!buf)
	{
		appWPrintf("File %s does not exist\n", *Filename);
		return false;
	}
	CSimpleParser text;
	text.InitFromBuf(buf);
	loadingFont = this;
	while (const char *line = text.GetLine())
	{
		if (!ExecuteCommand(line, ARRAY_ARG(fontCommands)))
			appWPrintf("%s: invalid line [%s]\n", name, line);
	}
	// setup remaining fields
	Name    = name;
	spacing = FONT_SPACING;

	delete buf;

	return true;
}


void CFont::GetTextExtents(const char *s, int &width, int &height)
{
	int x = 0, w = 0;
	int h = outHeight;
	while (char c = *s++)
	{
		if (c == '\n')
		{
			if (x > w) w = x;
			x = 0;
			h += outHeight;
			continue;
		}
		x += outWidth;
	}
	width  = max(x, w);
	height = h;
}


} // namespace
