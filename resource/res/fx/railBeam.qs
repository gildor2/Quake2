sort seethrough
nopicmip
nomipmaps
cull none
{
	map fx/rail2
	blendFunc GL_SRC_ALPHA GL_ONE
	rgbGen vertex
	alphaGen vertex
	tcMod scale 1 0.083	// 1, 1/12
}
