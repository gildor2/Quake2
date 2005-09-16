sort seeThrough
nopicmip
nomipmaps
cull none
{
	map fx/rail
	blendFunc GL_SRC_ALPHA GL_ONE
	rgbGen vertex
	alphaGen vertex
	tcMod scale 0 0.0286	// 0, 1/35
	tcMod scroll 0 4
}
