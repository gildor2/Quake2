skyparms - 2048 -
{
	map env/q1sky_back
	tcMod scale 4 4
	tcMod scroll -0.01 -0.05
}
{
	map env/q1sky_mask
	blendFunc GL_ONE GL_ONE // GL_ONE_MINUS_SRC_COLOR
	tcMod scale 4 4
	tcMod scroll -0.05 -0.1
}
