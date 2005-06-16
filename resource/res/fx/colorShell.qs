sort seeThrough
nopicmip
deformVertexes wave 100 sin 1 0 0 0		//?? 3/0.5 -- depends on model size, may be, use base fromEntity
{
	map fx/colorshell
	rgbGen entity
	tcGen environment
	tcMod rotate 30
	tcMod scroll 1 0.1
	blendFunc add
}
