/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "r_local.h"


#define	MAX_RIMAGES	1024
image_t		r_images[MAX_RIMAGES];
int			numr_images;
unsigned char d_16to8table[65536];

cvar_t  *sw_prefer8bittex;

/*
===============
R_ImageList_f
===============
*/
void	R_ImageList_f (void)
{
	int		i;
	image_t	*image;
	int		texels, imgs;
	char	*t;

	Com_Printf ("----t---w---h---name------------\n");
	texels = 0;
	imgs = 0;

	for (i = 0, image=r_images; i < numr_images; i++, image++)
	{
		if (image->registration_sequence <= 0)
			continue;
		texels += image->width*image->height;
		imgs++;

		switch (image->type)
		{
		case it_skin:
			t = "skn";//'M';
			break;
		case it_sprite:
			t = "spr";//'S';
			break;
		case it_wall:
			t = "wal";//'W';
			break;
		case it_pic:
			t = "pic";//'P';
			break;
		case it_sky:
			t = "sky";
			break;
		case it_temp:
			t = "tmp";
			break;
		default:
			t = " ? ";//' ';
			break;
		}

		Com_Printf ( "%-3i %s %-3i %-3i %s\n", i,
			t, image->width, image->height, image->name);
	}
	Com_Printf ("Total %i images with texel count %i\n", imgs, texels);
}


//=======================================================

image_t *R_FindFreeImage (void)
{
	image_t		*image;
	int			i;

	// find a free image_t
	for (i=0, image=r_images ; i<numr_images ; i++,image++)
	{
		if (!image->registration_sequence)
			break;
	}
	if (i == numr_images)
	{
		if (numr_images == MAX_RIMAGES)
			Com_DropError ("MAX_RIMAGES");
		numr_images++;
	}
	image = &r_images[i];

	return image;
}

/*
================
R_LoadPic

================
*/
image_t *R_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type)
{
	image_t		*image;
	int			i, c, b;

	image = R_FindFreeImage ();
	if (strlen(name) >= sizeof(image->name))
		Com_DropError ("Draw_LoadPic: \"%s\" is too long", name);
	strcpy (image->name, name);
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;

	c = width*height;
	image->pixels[0] = (byte*)appMalloc (c);
	image->transparent = false;
	for (i=0 ; i<c ; i++)
	{
		b = pic[i];
		if (b == 255)
			image->transparent = true;
		image->pixels[0][i] = b;
	}

	return image;
}

/*
================
R_LoadTex
================
*/
image_t *R_LoadTex (char *name, byte *pic, int width, int height)
{
	image_t		*image;
	int			size;

	image = R_FindFreeImage ();
	strcpy (image->name, name);
	image->width = width;
	image->height = height;
	image->type = it_wall;
	image->registration_sequence = registration_sequence;

	size = width*height * (256+64+16+4)/256;
	image->pixels[0] = (byte*)appMalloc (size);
	image->pixels[1] = image->pixels[0] + width*height;
	image->pixels[2] = image->pixels[1] + width*height/4;
	image->pixels[3] = image->pixels[2] + width*height/16;

	memcpy ( image->pixels[0], pic, size);

	return image;
}


/*
================
R_LoadWal
================
*/
image_t *R_LoadWal (char *name)
{
	miptex_t	*mt;
	int			ofs;
	image_t		*image;
//	int			size;

	if (!(mt = (miptex_t*) FS_LoadFile (name)))
	{
		Com_Printf ("R_LoadWal: can't load %s\n", name);
		return r_notexture_mip;
	}

	ofs = LittleLong (mt->offsets[0]);
	image = R_LoadTex (name, (byte *)mt + ofs, mt->width, mt->height);

	FS_FreeFile (mt);

	return image;
}


/*
================
R_MipMap

Operates in place, quartering the size of the texture
================
*/
void R_MipMap (byte *in, byte *out, int width, int height)
{
	int	i, j, size;

	width <<=2;
	height >>= 1;
	size = 0;
	for (i=0 ; i<height ; i++, in+=width)
	{
		for (j = 0; j < width; j+=8, out+=4, in+=8, size+=4*4)
		{
			out[0] = (in[0] + in[4] + in[width+0] + in[width+4])>>2;
			out[1] = (in[1] + in[5] + in[width+1] + in[width+5])>>2;
			out[2] = (in[2] + in[6] + in[width+2] + in[width+6])>>2;
			out[3] = (in[3] + in[7] + in[width+3] + in[width+7])>>2;
		}
	}
}


/*
=============
R_DownsampleImage()
converts 24(32)-bit image to 8-bit paletted
=============
*/
void R_DownsampleImage (byte **pic, int width, int height, bool mipmap)
{
	byte *buf, *in, *out, *pic2;
	int size, i, r, g, b, a;

	if (!mipmap)
		size = width * height;
	else
		size = width * height * (256+64+16+4)/256;

	buf = (byte*)appMalloc (size * 4); // temporary buffer
	memcpy (buf, *pic, width * height * 4);

	if (mipmap)
	{
		int w,h;
		byte *in2, *out2;

		w = width;
		h = height;
		in2 = buf;
		for (i = 0; i < 3; i++) // generate additional 3 mipmaps
		{
			out2 = in2 + w * h * 4;
			R_MipMap (in2, out2, w, h);
			in2 = out2;
			w >>= 1;
			h >>= 1;
		}
	}

	in = buf;
	pic2 = out = (byte*)appMalloc (size);
	for (i = 0; i < size; i++)
	{
		r = *in++;
		g = *in++;
		b = *in++;
		a = *in++;
		if (a != 0) //?? (a == 255) -- any non-full transparent will be non-transparent
		{
			r = (r >> 3) & 31;
			g = (g << 3) & (63 << 5);
			b = (b << 8) & (31 << 11);
			*out++ = d_16to8table[r | g | b];
		}
		else
		*out++ = 255; // alpha = 0
	}
	appFree (buf);
	appFree (*pic);
	*pic = pic2;
}


/*
===============
R_FindImage

Finds or loads the given image
===============
*/

image_t	*R_FindImage (char *name, imagetype_t type)
{
	image_t	*image;
	int	i, len, check_other;
	byte	*pic, *palette;
	int	width, height, bits, fmt;
	char	c, *s, *d, buf[256];

	if (!name)
		return NULL;	//	Com_DropError ("GL_FindImage: NULL name");

	// make a local copy of image name with a lower case conversion
	s = name;
	d = buf;
	while (c = *s++)
	{
		if (c >= 'A' && c <= 'Z') c += 32;
		*d++ = c;
	}
	*d = 0; // ASCIIZ
	name = buf;

	len = strlen (name);
	if (len < 5)
		return NULL;	//	Com_DropError ("GL_FindImage: bad name: %s", name);

	len -= 4;	// &name[len] -> ".ext"
	s = &name[len];		// points to extension

	// Check if texture already loaded

	// if extension is .??? (originally, .PCX or .WAL), check TGA and JPG first, then PCX/WAL
	check_other = !strcmp (s, ".???") || !strcmp (s, ".pcx") || !strcmp (s, ".wal");
	// look for it
	for (i = 0, image = r_images; i < numr_images; i++,image++)
	{
		char *s1;

		if (memcmp (name, image->name, len)) continue; // different name

		s1 = &image->name[len];
		if (check_other)
		{
			if (!strcmp (s1, ".tga") || !strcmp (s1, ".jpg")
			 || !strcmp (s1, ".wal") || !strcmp (s1, ".pcx"))
			{
				image->registration_sequence = registration_sequence;
				return image;
			}
		}
		else
		{	// full image name specified - check rest of filename
			if (!strcmp (s1, s))
			{
				image->registration_sequence = registration_sequence;
				return image;
			}
		}
	}

	if (type == it_pic && memcmp (name, "pics/", 4))	// mark pics from other directories as temporary
		type = it_temp;

	//
	// load the pic from disk
	//
	pic = NULL;
	palette = NULL;
	image = NULL;
	bits = 0;
	if (check_other)
	{
		*s = 0; // cut extension
		if (sw_prefer8bittex->integer)
		{
			fmt = ImageExists (name, IMAGE_8BIT);
			if (fmt & IMAGE_8BIT) fmt &= IMAGE_8BIT;	// disable loading TGA and JPG when PCX or WAL exists
		}
		else
			fmt = ImageExists (name, IMAGE_32BIT);
		if (fmt & IMAGE_TGA)
			strcpy (s, ".tga");
		else if (fmt & IMAGE_JPG)
			strcpy (s, ".jpg");
		else if (fmt & IMAGE_PCX)
			strcpy (s, ".pcx");
		else if (fmt & IMAGE_WAL)
			strcpy (s, ".wal");
		else
		{
			Com_DPrintf ("Cannot find image %s.*\n", name);
			return NULL;
		}

	}
	else
	{
		if (!strcmp (s, ".tga"))
			fmt = IMAGE_TGA;
		else if (!strcmp (s, ".jpg"))
			fmt = IMAGE_JPG;
		else if (!strcmp (s, ".pcx"))
			fmt = IMAGE_PCX;
		else if (!strcmp (s, ".wal"))
			fmt = IMAGE_WAL;
		else
		{
			Com_WPrintf ("Unrecognized image extension: %s\n", name);
			return NULL;
		}
	}

	if (fmt & IMAGE_TGA)
	{
		LoadTGA (name, &pic, &width, &height);
		bits = 32;
	}
	else if (fmt & IMAGE_JPG)
	{
		LoadJPG (name, &pic, &width, &height);
		bits = 32;
	}
	else if (fmt & IMAGE_PCX)
	{
		LoadPCX (name, &pic, &palette, &width, &height);
		bits = 8;
	}
	else if (fmt & IMAGE_WAL)
		image = R_LoadWal (name);

	if (pic)
	{
		bool iswall;

		iswall = (type == it_wall);
		if (bits == 32)
			R_DownsampleImage (&pic, width, height, iswall);
		if (iswall)
			image = R_LoadTex (name, pic, width, height);
		else
			image = R_LoadPic (name, pic, width, height, type);
	}

	if (!image)
		Com_WPrintf ("Cannot load texture %s\n", name);

	if (pic)
		appFree(pic);
	if (palette)
		appFree(palette);

	return image;
}


/*
===============
R_RegisterSkin
===============
*/
struct image_s *R_RegisterSkin (char *name)
{
	return R_FindImage (name, it_skin);
}


/*
================
R_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void R_FreeUnusedImages (void)
{
	int		i;
	image_t	*image;

	for (i=0, image=r_images ; i<numr_images ; i++, image++)
	{
		if (image->registration_sequence == registration_sequence)
		{
			Com_PageInMemory ((byte *)image->pixels[0], image->width*image->height);
			continue;		// used this sequence
		}
		if (!image->registration_sequence)
			continue;		// free texture
		if (image->type == it_pic)
			continue;		// don't free pics
		// free it
		appFree (image->pixels[0]);	// the other mip levels just follow
		memset (image, 0, sizeof(*image));
	}
}



/*
===============
R_InitImages
===============
*/
void	R_InitImages (void)
{
	void *buf;
	unsigned len;

	registration_sequence = 1;

	buf = FS_LoadFile ("pics/16to8.dat", &len);
	if (len != 65536 || !buf)
		Com_FatalError ("Couldn't load pics/16to8.dat");
	memcpy (d_16to8table, buf, 65536);

	sw_prefer8bittex = Cvar_Get ("sw_prefer8bittex", "0", CVAR_ARCHIVE);
}

/*
===============
R_ShutdownImages
===============
*/
void	R_ShutdownImages (void)
{
	int		i;
	image_t	*image;

	for (i=0, image=r_images ; i<numr_images ; i++, image++)
	{
		if (!image->registration_sequence)
			continue;		// free texture
		// free it
		appFree (image->pixels[0]);	// the other mip levels just follow
		memset (image, 0, sizeof(*image));
	}
}
