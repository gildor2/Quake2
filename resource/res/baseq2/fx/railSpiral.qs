sort seeThrough
nopicmip
nomipmaps
cull none
{
	map fx/rail
	blendFunc GL_SRC_ALPHA GL_ONE
	rgbGen vertex
	alphaGen vertex
	tcMod scale 1 0.0159	// 1, 1/63
	tcMod scroll 0 2
}
