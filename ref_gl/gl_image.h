#ifndef __GL_IMAGE_INCLUDED__
#define __GL_IMAGE_INCLUDED__


namespace OpenGLDrv {


// consts for image_t.flags
#define IMAGE_CLAMP			1			// 0 -- repeat
#define	IMAGE_MIPMAP		2			// generate and upload mipmap levels
#define IMAGE_PICMIP		4			// allow texture downsampling for improving FPS
#define IMAGE_NOALPHA		8			// original image have alpha-channel, but it was removed
#define IMAGE_MAKEALPHA		0x10		// generate alpha-channel from color
// image type
#define IMAGE_SYSTEM		0x20		// auto-generated image (default, fog, dlight etc.)
#define IMAGE_WORLD			0x40		// used for mip bias only
#define IMAGE_SKIN			0x80		// ...
//?? #define IMAGE_GUI			0x100
// hints for image creation
#define IMAGE_TRUECOLOR		0x80000000	// hint for GL_CreateImage: force to upload image in 32 bit
#define IMAGE_LIGHTMAP		0x40000000
#define IMAGE_NOCOMPRESS	0x20000000
#define IMAGE_RELOAD		0x10000000	// load image from disk even when it was already loaded
#define IMAGE_CHECKLOADED	0x08000000	// if image loaded, return it, else - NULL
#define IMAGE_ANYFLAGS		0x04000000	// ignore wrap/clamp/mipmap etc flags; used with CHECKLOADED
// mask of flags, stored in image (exclude hints)
#define IMAGE_FLAGMASK		0x0000FFFF

class image_t //?? : public CBasicImage
{
public:
	TString<64> Name;					// lowercased game-relative image name (system images started with "*")
	int		width, height;
	byte	alphaType;					// 0 - none, 1 - 1-bit alpha, 2 - 8-bit alpha
	GLenum	target;						// GL_TEXTURE_2D, GL_TEXTURE_RECTANGLE_NV
	int		internalWidth, internalHeight;	// statistics
	GLenum	internalFormat;				// statistics (for imagelist)
	unsigned texnum;					// gl texture binding
	unsigned flags;
	byte	*pic;						// when non NULL, image was not uploaded
	color_t	color;
	image_t *hashNext;
};


extern	image_t		*gl_defaultImage;
//??extern	image_t		*gl_whiteImage;
extern	image_t		*gl_identityLightImage;
extern	image_t		*gl_dlightImage;
extern	image_t		*gl_particleImage;
extern	image_t		*gl_fogImage;
extern	image_t		*gl_reflImage;		// NULL if not found
extern	image_t		*gl_reflImage2;		// NULL if not found

extern int 	screenshotFlags;
extern char	*screenshotName;

void	GL_TextureMode(const char *name);

void	PerformScreenshot();

// setup
void	SetupGamma();
void	InitImages();
void	ShutdownImages();
void	LoadDelayedImages();

// creating images
image_t *CreateImage(const char *name, void *pic, int width, int height, unsigned flags);
image_t *CreateImage8(const char *name, void *pic, int width, int height, unsigned flags, unsigned *palette);
image_t *FindImage(const char *name, unsigned flags);
bool     GetImageColor(const char *name, color_t *color);

// video support
//void	DrawStretchRaw8(int x, int y, int w, int h, int width, int height, byte *pic, unsigned *palette); -- interface

void	ShowImages();


} // namespace


#endif
