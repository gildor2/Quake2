{
	map $whiteimage
	rgbGen const 0.1 0.5 0.1
}
{
	map *detail
	tcMod scroll 1 -1
	tcMod rotate 30
	blendFunc blend
	rgbGen const 0.7 0 0
	alphaGen dot -1 2
}
