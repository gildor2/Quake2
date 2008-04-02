#define TBL_IGNORE(name)	{ STR(name), NULL }

#define IS(n,str)	(!stricmp(argv[n], str))

// parser helpers

static unsigned GetColor(int argc, char **argv, int pos, bool parenthesis = false)
{
	if (parenthesis)
	{
		if (!IS(pos-1, "(") || !IS(pos+3, ")"))
		{
			shaderError = "missing parenthesis";
			return RGB(1,1,1);
		}
	}
	if (pos + 3 > argc)
	{
		shaderError = "invalid rgbGen const";
		return RGB(1,1,1);
	}
	float r = atof(argv[pos]);
	float g = atof(argv[pos+1]);
	float b = atof(argv[pos+2]);
	return RGBS(r, g, b);
}

static void GetWave0(int argc, char **argv, int pos, waveParams_t &wave)
{
	if (pos + 4 > argc)
		ERROR_IN_SHADER("invalid wave arguments");
	wave.base  = atof(argv[pos]);
	wave.amp   = atof(argv[pos+1]);
	wave.phase = atof(argv[pos+2]);
	wave.freq  = atof(argv[pos+3]);
}

static void GetWave(int argc, char **argv, int pos, waveParams_t &wave)
{
	if (pos + 5 > argc)
		ERROR_IN_SHADER("invalid wave arguments");
	static const char *waveNames[] = {
		"sin", "square", "triangle", "sawtooth" //!! "noise"
	};
	bool inv = false;
	const char *func = argv[pos];
	if (!strnicmp(func, "inverse", 7))
	{
		inv = true;
		func += 7;		// skip "inverse"
	}
	int i;
	for (i = 0; i < ARRAY_COUNT(waveNames); i++)
		if (!stricmp(func, waveNames[i])) break;
	if (i == ARRAY_COUNT(waveNames)) ERROR_IN_SHADER(va("bad wave func: %s", argv[pos]));
	wave.type  = (waveFunc_t) i;
	GetWave0(argc, argv, pos+1, wave);
	// undocumented: any "inverse" function
	if (inv)
	{
		switch (wave.type)
		{
		// functions in 0..1 range
		case FUNC_SAWTOOTH:
			wave.base = wave.base + wave.amp;
			wave.amp  = -wave.amp;
			break;
		// functions in -1..1 range
		case FUNC_SIN:
		case FUNC_SQUARE:
			wave.amp  = -wave.amp;
			break;
		}
	}
}

static void GetVec(int argc, char **argv, int pos, CVec3 &dst)
{
	if (pos + 5 > argc ||
		argv[pos][0] != '(' || argv[pos][1] != 0 ||
		argv[pos+4][0] != ')' || argv[pos+4][1] != 0)
		ERROR_IN_SHADER("invalid vector param");
	dst[0] = atof(argv[pos+1]);
	dst[1] = atof(argv[pos+2]);
	dst[2] = atof(argv[pos+3]);
}


/*-----------------------------------------------------------------------------
	Shader-level functions
-----------------------------------------------------------------------------*/

#define FUNC(name)			static void Shader_##name(int argc, char **argv)
#define TBL(name)			{ STR(name), Shader_##name }
#define TBL2(name, alias)	{ alias, Shader_##name }


FUNC(cull)
{
	gl_cullMode_t cull;
	if (IS(1, "none") || IS(1, "disable") || IS(1, "twosided"))
		cull = CULL_NONE;
	else if (IS(1, "back") || IS(1, "backside") || IS(1, "backsided"))
		cull = CULL_BACK;
	else if (IS(1, "front"))
		cull = CULL_FRONT;
	else
		ERROR_IN_SHADER(va("bad cull param: %s", argv[1]));
	sh.cullMode = cull;
}


FUNC(polygonOffset)
{
	sh.usePolygonOffset = true;
}


FUNC(force32bit)
{
	sh_imgFlags |= IMAGE_TRUECOLOR;
}


FUNC(nomipmaps)
{
	sh_imgFlags &= ~(IMAGE_MIPMAP|IMAGE_PICMIP);
}


FUNC(nopicmip)
{
	sh_imgFlags &= ~IMAGE_PICMIP;
}


FUNC(sort)
{
	static const struct {
		const char *name;
		int		sort;
	} sortNames[] = {
		{"portal",		SORT_PORTAL		},
		{"sky",			SORT_SKY		},
		{"opaque",		SORT_OPAQUE		},
		{"decal",		SORT_DECAL		},
		{"seeThrough",	SORT_SEETHROUGH	},
		{"banner",		SORT_BANNER		},
		{"underwater",	SORT_UNDERWATER	},
		{"additive",	SORT_SPRITE		},
		{"nearest",		16				}
	};
	int sort = SORT_BAD;
	for (int i = 0; i < ARRAY_COUNT(sortNames); i++)
		if (IS(1, sortNames[i].name))
		{
			sort = sortNames[i].sort;
			break;
		}
	if (sort == SORT_BAD)
		sort = atoi(argv[1]);
	sh.sortParam = sort;
}


FUNC(deformVertexes)
{
	static const struct {
		const char *name;
		deformType_t type;
	} deformNames[] = {
		{"wave",		DEFORM_WAVE			},
		{"autoSprite",	DEFORM_AUTOSPRITE	},
		{"autoSprite2", DEFORM_AUTOSPRITE2	},
		{"move",		DEFORM_MOVE			},
		{"bulge",		DEFORM_BULGE		}
		//!! other
	};
	deformType_t type;
	int i;
	for (i = 0; i < ARRAY_COUNT(deformNames); i++)
		if (IS(1, deformNames[i].name))
		{
			type = deformNames[i].type;
			break;
		}
	if (i == ARRAY_COUNT(deformNames))
		ERROR_IN_SHADER(va("unknown deform type: %s\n", argv[1]));

	if (sh.numDeforms >= MAX_SHADER_DEFORMS)
		ERROR_IN_SHADER("too much deforms");

	deformParms_t &deform = sh.deforms[sh.numDeforms++];
	deform.type = type;

	switch (type)
	{
	case DEFORM_WAVE:
		deform.waveDiv = atof(argv[2]);
		if (deform.waveDiv == 0)
			ERROR_IN_SHADER("bad deform wave div")
		else
			deform.waveDiv = 1.0f / deform.waveDiv;
		GetWave(argc, argv, 3, deform.wave);
		break;
	case DEFORM_MOVE:
		deform.move[0] = atof(argv[2]);
		deform.move[1] = atof(argv[3]);
		deform.move[2] = atof(argv[4]);
		GetWave(argc, argv, 5, deform.moveWave);
		break;
	case DEFORM_BULGE:
		deform.bulgeWidth  = atof(argv[2]);
		deform.bulgeHeight = atof(argv[3]);
		deform.bulgeSpeed  = atof(argv[4]);
		break;
	}
}


FUNC(surfaceparm)
{
	// helper function, most flags may be silently ignored
	if (IS(1, "nodraw"))
		sh.noDraw = true;
}


FUNC(tessSize)
{
	sh.tessSize = atof(argv[1]);
}


FUNC(fogParms)
{
	sh.type = SHADERTYPE_FOG;
	if (argc != 7)
		ERROR_IN_SHADER("bad fogparms");
	sh.fogColor.rgba = GetColor(argc, argv, 2, true);
	sh.fogDist       = atof(argv[6]);
}


FUNC(skyParms)
{
	sh.type = SHADERTYPE_SKY;
	if (argc != 4)
		ERROR_IN_SHADER("bad skyparms");
	if (!IS(1, "-"))
	{
		if (sh.numStages)
			ERROR_IN_SHADER("skybox after stages");
		SetShaderSkyBox(argv[1]);			// NOTE: here is no default skybox dir
		sh.numStages = 1;
	}
	sh.cloudHeight = atof(argv[2]);
	sh_imgFlags = IMAGE_MIPMAP;				// override texture settings for skies
	// argv[3] is innerbox, is is not supported in q3
}


FUNC(q3map_surfacelight)
{
	sh.lightValue = atof(argv[1]);
}


static void SetShaderColorTex(const char *name, bool priority)
{
	if (ShaderLightImage[0] && !priority)
		return;			// already set
	TString<64> Name; Name.filename(name);
	char *s = Name.rchr('.');
	if (s) *s = 0;		// cut extension for ImageExists()
	if (!ImageExists(Name)) return;
	// use this texture
	ShaderLightImage = Name;
}


FUNC(q3map_lightimage)
{
	SetShaderColorTex(argv[1], true);
}


FUNC(qer_editorimage)
{
	SetShaderColorTex(argv[1], false);
}


FUNC(q3map_sun)
{
	// get color
	sh.sunColor[0] = atof(argv[1]);
	sh.sunColor[1] = atof(argv[2]);
	sh.sunColor[2] = atof(argv[3]);
	sh.sunColor.Normalize();
	// get intensity
	float intens = atof(argv[4]);
	sh.sunColor.Scale(intens);
	// get direction (note: q3map uses opposite direction, so negate vector)
	float yaw   = atof(argv[5]) / 180 * M_PI;
	float pitch = atof(argv[6]) / 180 * M_PI;
	sh.sunDirection[0] = - cos(yaw) * cos(pitch);
	sh.sunDirection[1] = - sin(yaw) * cos(pitch);
	sh.sunDirection[2] = - sin(pitch);
}


// registration

static CSimpleCommand shaderFuncs[] = {
	TBL(cull),
	TBL(polygonOffset),
	TBL(force32bit),
	TBL(nomipmaps),
	TBL(nopicmip),
	TBL(sort),
	TBL(deformVertexes),
	TBL(surfaceparm),
	TBL(tessSize),
	// fog
	TBL(fogParms),
	// sky, light
	TBL(skyParms),
	TBL(q3map_surfacelight),
	TBL(q3map_lightimage),
	TBL(qer_editorimage),
	TBL(q3map_sun),
	// ignored keywords
	TBL_IGNORE(light),
	TBL_IGNORE(entityMergable)
};


#undef FUNC
#undef TBL
#undef TBL2

/*-----------------------------------------------------------------------------
	Stage-level functions
-----------------------------------------------------------------------------*/

#define FUNC(name)			static void Stage_##name(int argc, char **argv)
#define TBL(name)			{ STR(name), Stage_##name }
#define TBL2(name, alias)	{ alias, Stage_##name }


// texture mapping

static image_t *ShaderImage(const char *name, unsigned addFlags = 0)
{
	char buf[256];
	// if image name starts with "./" -- replace with shader path
	if (name[0] == '.' && name[1] == '/')
	{
		strcpy(buf, sh.Name);
		char *s = strrchr(buf, '/');
		if (!s) s = buf;
		else	s++;			// skip '/'
		strcpy(s, name+2);
		name = buf;
	}
	return FindImage(name, sh_imgFlags | addFlags);
}


FUNC(map)
{
	image_t *img;
	shaderStage_t &s = st[sh.numStages];
	if (IS(1, "$lightmap"))
	{
		s.isLightmap = true;
		if (sh.lightmapNumber >= 0)
		{
			img = GetLightmapImage(sh.lightmapNumber);
			s.glState      |= GLSTATE_DEPTHWRITE;
			s.rgbGenType   = RGBGEN_IDENTITY;
			s.alphaGenType = ALPHAGEN_IDENTITY;
		}
		else
			img = NULL;				// $whiteimage
	}
	else if (IS(1, "$whiteimage"))
		img = NULL;
	else
	{
		img = ShaderImage(IS(1,"$texture") ? sh.Name : argv[1]);
		if (!img) ERROR_IN_SHADER(va("no texture: %s", argv[1]));
		if (!sh.width && !sh.height)
		{
			sh.width  = img->width;
			sh.height = img->height;
		}
	}
	SetShaderImage(img, sh.numStages);
	s.numAnimTextures = 1;
}


FUNC(clampMap)
{
	image_t *img = ShaderImage(IS(1,"$texture") ? sh.Name : argv[1], IMAGE_CLAMP);
	if (!img) ERROR_IN_SHADER(va("no texture: %s", argv[1]));
	SetShaderImage(img, sh.numStages);
	if (!sh.width && !sh.height)
	{
		sh.width  = img->width;
		sh.height = img->height;
	}
	st[sh.numStages].numAnimTextures = 1;
}


static void DoAnimMap(int argc, char **argv, unsigned imgFlags)
{
	if (argc < 3) ERROR_IN_SHADER("not enough animmap args");
	st[sh.numStages].animMapFreq = atof(argv[1]);
	if (argc > MAX_STAGE_TEXTURES + 2) ERROR_IN_SHADER("too much animmap textures");

	for (int i = 2; i < argc; i++)
	{
		image_t *img = ShaderImage(argv[i], imgFlags);
		if (!img) ERROR_IN_SHADER(va("no texture: %s", argv[i]));
		if (!sh.width && !sh.height)
		{
			sh.width  = img->width;
			sh.height = img->height;
		}
		SetShaderImage(img, sh.numStages, i-2);
		st[sh.numStages].numAnimTextures++;
	}
}


FUNC(animMap)
{
	DoAnimMap(argc, argv, sh_imgFlags);
}

FUNC(animClampMap)
{
	DoAnimMap(argc, argv, sh_imgFlags | IMAGE_CLAMP);
}


// GL_State

FUNC(alphaFunc)
{
	unsigned alpha = 0;
	if (IS(1, "GT0"))
		alpha = GLSTATE_ALPHA_GT0;
	else if (IS(1, "LT128"))
		alpha = GLSTATE_ALPHA_LT05;
	else if (IS(1, "GE128"))
		alpha = GLSTATE_ALPHA_GE05;
	else
		ERROR_IN_SHADER(va("bad alpha mode: %s", argv[1]));
	unsigned &glState = st[sh.numStages].glState;
	glState = glState & ~GLSTATE_ALPHAMASK | alpha;
}


FUNC(blendfunc)
{
	static const char *blendNames[] = {
		"GL_ZERO",					//	1
		"GL_ONE",					//	2
		"GL_SRC_COLOR",				//	3
		"GL_ONE_MINUS_SRC_COLOR",	//	4
		"GL_SRC_ALPHA",				//	5
		"GL_ONE_MINUS_SRC_ALPHA",	//	6
		"GL_DST_COLOR",				//	7
		"GL_ONE_MINUS_DST_COLOR",	//	8
		"GL_DST_ALPHA",				//	9
		"GL_ONE_MINUS_DST_ALPHA",	//	0xA
		"GL_SRC_ALPHA_SATURATE"		//	0xB
	};

	unsigned blend = 0;
	if (IS(1, "add"))
		blend = BLEND(1,1);
	else if (IS(1, "blend"))
		blend = BLEND(S_ALPHA, M_S_ALPHA);
	else if (IS(1, "filter"))
		blend = BLEND(0, S_COLOR);
	else
	{
		int i;
		for (i = 0; i < ARRAY_COUNT(blendNames); i++)
			if (IS(1, blendNames[i])) break;
		if (i == ARRAY_COUNT(blendNames))
			ERROR_IN_SHADER(va("bad blend arg: %s", argv[1]));
		int j;
		for (j = 0; j < ARRAY_COUNT(blendNames); j++)
			if (IS(2, blendNames[j])) break;
		if (j == ARRAY_COUNT(blendNames))
			ERROR_IN_SHADER(va("bad blend arg: %s", argv[2]));

		blend = ((i+1) << GLSTATE_SRCSHIFT) | ((j+1) << GLSTATE_DSTSHIFT);
	}
	unsigned &glState = st[sh.numStages].glState;
	glState = glState & ~GLSTATE_BLENDMASK | blend;
}


FUNC(depthfunc)
{
	unsigned depth;
	if (IS(1, "lequal"))
		depth = 0;
	else if (IS(1, "equal"))
		depth = GLSTATE_DEPTHEQUALFUNC;
	else
		ERROR_IN_SHADER(va("bad depth argument: %s", argv[1]));
	st[sh.numStages].glState |= depth;
}


FUNC(depthwrite)
{
	st[sh.numStages].glState |= GLSTATE_DEPTHWRITE;
}


FUNC(nodepthtest)
{
	//?? this flag should be extended on all shader stages (when multi-pass shaders)?
	//?? or it should be used per-shader (not per-pass)
	st[sh.numStages].glState |= GLSTATE_NODEPTHTEST;
}


// gens

FUNC(rgbGen)
{
	static const struct {
		const char *name;
		rgbGenType_t rgbGen;
	} rgbNames[] = {
		// simple
		{"identity",		RGBGEN_IDENTITY			},
		{"identityLighting",RGBGEN_IDENTITY_LIGHTING},
		{"entity",			RGBGEN_ENTITY			},
		{"oneMinusEntity",	RGBGEN_ONE_MINUS_ENTITY	},
		{"vertex",			RGBGEN_VERTEX			},
		{"oneMinusVertex",	RGBGEN_ONE_MINUS_VERTEX	},
		{"exactVertex",		RGBGEN_EXACT_VERTEX		},
		{"lightingDiffuse",	RGBGEN_DIFFUSE			},
		// parametrized
		{"const",			RGBGEN_CONST			},
		{"constant",		RGBGEN_CONST			}
	};

	rgbGenType_t rgbGen = RGBGEN_NONE;
	for (int i = 0; i < ARRAY_COUNT(rgbNames); i++)
		if (IS(1, rgbNames[i].name))
		{
			rgbGen = rgbNames[i].rgbGen;
			break;
		}
	int waveArg = 0;
	unsigned rgbaConst = RGB(1,1,1);
	if (rgbGen == RGBGEN_NONE)		// not in table
	{
		if (IS(1, "colorWave"))
		{
			waveArg = 5;
			rgbaConst = GetColor(argc, argv, 2);
		}
		else if (IS(1, "wave"))
			waveArg = 2;
		else
			ERROR_IN_SHADER(va("unknown rgbGen: %s", argv[1]));
	}

	if (rgbGen == RGBGEN_CONST)
		rgbaConst = GetColor(argc, argv, 3, true);

	shaderStage_t &s = st[sh.numStages];
	if (waveArg)
	{
		rgbGen = RGBGEN_WAVE;
		GetWave(argc, argv, waveArg, s.rgbGenWave);
	}
	s.rgbGenType = rgbGen;
	byte alpha = s.rgbaConst.c[3];	// save alpha value
	s.rgbaConst.rgba = rgbaConst;	// store color (with alpha == 255)
	s.rgbaConst.c[3] = alpha;		// restore alpha value
}


FUNC(alphaGen)
{
	static const struct {
		const char *name;
		alphaGenType_t alpha;
	} alphaNames[] = {
		{"wave",		ALPHAGEN_WAVE	},
		{"const",		ALPHAGEN_CONST	},
		{"identity",	ALPHAGEN_IDENTITY},
		{"entity",		ALPHAGEN_ENTITY	},
		{"oneMinusEntity", ALPHAGEN_ONE_MINUS_ENTITY},
		{"vertex",		ALPHAGEN_VERTEX	},
		{"oneMinusVertex", ALPHAGEN_ONE_MINUS_VERTEX},
		{"lightingSpecular", ALPHAGEN_LIGHTING_SPECULAR},
		{"portal",		ALPHAGEN_PORTAL},
		{"dot",			ALPHAGEN_DOT	},
		{"oneMinusDot",	ALPHAGEN_ONE_MINUS_DOT}
	};
	alphaGenType_t alpha;
	int i;
	for (i = 0; i < ARRAY_COUNT(alphaNames); i++)
		if (IS(1, alphaNames[i].name))
		{
			alpha = alphaNames[i].alpha;
			break;
		}
	if (i == ARRAY_COUNT(alphaNames))
		ERROR_IN_SHADER(va("unknown alphaGen: %s", argv[1]));

	shaderStage_t &s = st[sh.numStages];
	s.alphaGenType = alpha;

	switch (alpha)
	{
	case ALPHAGEN_DOT:
	case ALPHAGEN_ONE_MINUS_DOT:
		s.alphaMin = atof(argv[2]);
		s.alphaMax = atof(argv[3]);
		break;
	case ALPHAGEN_WAVE:
		GetWave(argc, argv, 2, s.alphaGenWave);
		break;
	case ALPHAGEN_CONST:
		s.rgbaConst.c[3] = appRound(atof(argv[2]) * 255);
		break;
	case ALPHAGEN_PORTAL:
		s.alphaPortalRange = atof(argv[2]);
		if (!s.alphaPortalRange)
			s.alphaPortalRange = 256;
		break;
	}
}


FUNC(tcGen)
{
	static const struct {
		const char *name;
		tcGenType_t tcGen;
	} tcNames[] = {
		// simple
		{"texture",		TCGEN_TEXTURE	},
		{"base",		TCGEN_TEXTURE	},
		{"lightmap",	TCGEN_LIGHTMAP	},
		{"environment",	TCGEN_ENVIRONMENT},
		// parametrized
		{"vector",		TCGEN_VECTOR	}
	};
	tcGenType_t tcGen = TCGEN_NONE;
	int i;
	for (i = 0; i < ARRAY_COUNT(tcNames); i++)
		if (IS(1, tcNames[i].name))
		{
			tcGen = tcNames[i].tcGen;
			break;
		}
	if (i == ARRAY_COUNT(tcNames))
		ERROR_IN_SHADER(va("unknown tcGen: %s", argv[1]));
	shaderStage_t &s = st[sh.numStages];
	s.tcGenType = (tcGenType_t) i;

	if (tcGen == TCGEN_VECTOR)
	{
		GetVec(argc, argv, 2, s.tcGenVec[0]);
		if (shaderError) return; // do not try 2nd vector
		GetVec(argc, argv, 7, s.tcGenVec[1]);
	}
}


FUNC(tcMod)
{
	static const struct {
		const char *name;
		tcModType_t tcMod;
		int	numArgs;
	} tcModNames[] = {
		{"turb",		TCMOD_TURB,		4},
		{"scale",		TCMOD_SCALE,	2},
		{"scroll",		TCMOD_SCROLL,	2},
		{"stretch",		TCMOD_STRETCH,	5},
		{"transform",	TCMOD_TRANSFORM,6},
		{"rotate",		TCMOD_ROTATE,	1},
//		{"entityTranslate", TCMOD_ENTITYTRANSLATE, 0},	// unused in Q3
		{"offset",		TCMOD_OFFSET,	2},
		{"warp",		TCMOD_WARP,		0}
	};
	tcModType_t tcModType;
	int i;
	for (i = 0; i < ARRAY_COUNT(tcModNames); i++)
		if (IS(1, tcModNames[i].name))
		{
			if (tcModNames[i].numArgs + 2 < argc)
				ERROR_IN_SHADER(va("bad arguments for %s", argv[1]));
			tcModType = tcModNames[i].tcMod;
			break;
		}
	if (i == ARRAY_COUNT(tcModNames))
		ERROR_IN_SHADER(va("unknown tcMod: %s", argv[1]));
	tcModParms_t *tcMod = NewTcModStage(&st[sh.numStages]);
	if (!tcMod) return;

	tcMod->type = tcModType;

	float f1 = atof(argv[2]);
	float f2 = atof(argv[3]);

	switch (tcModType)
	{
	case TCMOD_TURB:
		GetWave0(argc, argv, 2, tcMod->wave);
		break;
	case TCMOD_SCALE:
		tcMod->sScale = f1;
		tcMod->tScale = f2;
		break;
	case TCMOD_SCROLL:
		tcMod->sSpeed = f1;
		tcMod->tSpeed = f2;
		break;
	case TCMOD_STRETCH:
		GetWave(argc, argv, 2, tcMod->wave);
		break;
	case TCMOD_ROTATE:
		tcMod->rotateSpeed = f1;
		break;
	case TCMOD_OFFSET:
		tcMod->sOffset = f1;
		tcMod->tOffset = f2;
		break;
	case TCMOD_TRANSFORM:
		tcMod->m[0][0] = f1;
		tcMod->m[0][1] = f2;
		tcMod->m[1][0] = atof(argv[4]);
		tcMod->m[1][1] = atof(argv[5]);
		tcMod->t[0]    = atof(argv[6]);
		tcMod->t[1]    = atof(argv[7]);
		break;
	}
}


// registration

static CSimpleCommand stageFuncs[] = {
	TBL(map),
	TBL(clampMap),
	TBL(animMap),
	TBL(animClampMap),
	TBL(alphaFunc),
	TBL(blendfunc),
	TBL(depthfunc),
	TBL(depthwrite),
	TBL(nodepthtest),
	TBL(rgbGen),
	TBL(alphaGen),
	TBL(tcGen),
	TBL2(tcGen, "texgen"),		// alias
	TBL(tcMod)
};

#undef FUNC
#undef TBL
#undef TBL2

#undef TBL_IGNORE
#undef IS
