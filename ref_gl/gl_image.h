#ifndef __GL_IMAGE_INCLUDED__
#define __GL_IMAGE_INCLUDED__


// consts for image_t.flags
#define IMAGE_CLAMP			1			// 0 -- repeat
#define	IMAGE_MIPMAP		2
#define IMAGE_PICMIP		4
#define IMAGE_NOALPHA		8			// original image have alpha-channel, but it was removed
// image type
#define IMAGE_SYSTEM		0x10		// auto-generated image (default, fog, dlight etc.)
#define IMAGE_WORLD			0x20
#define IMAGE_SKIN			0x40
//?? #define IMAGE_GUI			0x80
// hints for image creating
#define IMAGE_TRUECOLOR		0x80000000	// hint for GL_CreateImage: force to upload image in 32 bit
#define IMAGE_LIGHTMAP		0x40000000
#define IMAGE_NOCOMPRESS	0x20000000
#define IMAGE_RELOAD		0x10000000	// load image from disk even when it was already loaded
// mask of flags, stored in image (exclude hints)
#define IMAGE_FLAGMASK		0x0000FFFF

typedef struct image_s
{
	char	name[MAX_QPATH];			// lowercased game-relative image name (system images started with "*")
	int		width, height;
	byte	alphaType;					// 0 - none, 1 - 1-bit alpha, 2 - 8-bit alpha
	GLenum	target;						// GL_TEXTURE_2D, GL_TEXTURE_RECTANGLE_NV
	int		internalWidth, internalHeight;	// statistics
	int		internalFormat;				// statistics (for imagelist)
	unsigned texnum;					// gl texture binding
	int		flags;
	byte	*pic;						// when non NULL, image was not uploaded
	color_t	color;
	struct image_s *hashNext;
} image_t;


extern	image_t		*gl_defaultImage;
//??extern	image_t		*gl_whiteImage;
extern	image_t		*gl_identityLightImage;
extern	image_t		*gl_dlightImage;
extern	image_t		*gl_particleImage;
//extern	image_t		*gl_fogImage;
extern	image_t		*gl_reflImage;		// NULL if not found
extern	image_t		*gl_reflImage2;		// NULL if not found

extern int 	gl_screenshotFlags;
extern char	*gl_screenshotName;

void	GL_TextureMode (char *name);

void	GL_PerformScreenshot (void);

// setup
void	GL_SetupGamma (void);
void	GL_InitImages (void);
void	GL_ShutdownImages (void);
void	GL_LoadDelayedImages (void);

// creating images
image_t *GL_CreateImage (const char *name, void *pic, int width, int height, int flags);
image_t *GL_FindImage (const char *name, int flags);

// video support
void	GL_SetRawPalette (const byte *palette);
void	GL_DrawStretchRaw8 (int x, int y, int w, int h, int width, int height, byte *pic);

void GL_ShowImages (void);


#endif
