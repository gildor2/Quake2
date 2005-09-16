//cull none
nopicmip
sort seeThrough
{
	map fx/colorshell
//	map $whiteimage
	rgbGen lightingDiffuse
	alphaGen oneMinusDot 0.1 0.7
//	alphaGen dot 0 0.5 // identity
	tcGen environment
	depthwrite
	blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
}
