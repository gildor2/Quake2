#include "OpenGLDrv.h"
#include "gl_shader.h"
#include "gl_buffers.h"


namespace OpenGLDrv {


shader_t *gl_defaultShader;
shader_t *gl_identityLightShader;
shader_t *gl_identityLightShader2;
shader_t *gl_concharsShader;
shader_t *gl_videoShader;			// fullscreen video
shader_t *gl_defaultSkyShader;		// default sky shader (black image)
shader_t *gl_particleShader;
shader_t *gl_entityShader;
shader_t *gl_flareShader;
shader_t *gl_detailShader;
shader_t *gl_colorShellShader;
shader_t *gl_railSpiralShader, *gl_railRingsShader, *gl_railBeamShader;
shader_t *gl_skyShader;				// current sky shader (have mapped images)
shader_t *gl_alphaShader1, *gl_alphaShader2;


#define HASH_BITS		8
#define HASH_SIZE		(1 << HASH_BITS)
#define HASH_MASK		(HASH_SIZE - 1)

#define	MAX_SHADERS		1024

static shader_t *shadersArray[MAX_SHADERS];	// sorted in ascending order with key = sortParam
				// it's easier to keep array sorted, than sort it with "sortParam" later

static shader_t *hashTable[HASH_SIZE];
static int	shaderCount;

static CMemoryChain *shaderChain;


// name should be in a lower case
static int ComputeHash (const char *name)
{
	int h = 0;
	while (char c = *name++)
		h = (h ^ 0x25) + c;
	return h & HASH_MASK;
}


/*-----------------------------------------------------------------------------
	Initialization/finalization
-----------------------------------------------------------------------------*/

static void Shaderlist_f (bool usage, int argc, char **argv)
{
	static const char *shTypes[] = {"", "sky", "fog", "por"};
	static const char *boolNames[] = {" ", "+"};
	static const char *badNames[] = {"", S_RED" (errors)"S_WHITE};

	if (argc > 2 || usage)
	{
		Com_Printf ("Usage: shaderlist [<mask>]\n");
		return;
	}

	const char *mask = (argc == 2) ? argv[1] : NULL;

	int n = 0;
	Com_Printf ("----ns-lm-s--type-name--------\n");
	for (int i = 0; i < shaderCount; i++)
	{
		shader_t *sh = shadersArray[i];
		if (mask && !appMatchWildcard (sh->name, mask, true)) continue;
		n++;

		const char *lmInfo, *color;
		switch (sh->lightmapNumber)
		{
		case LIGHTMAP_NONE:
			lmInfo = " ";
			break;
		case LIGHTMAP_VERTEX:
			lmInfo = "v";
			break;
		case LIGHTMAP_RESERVE:
			lmInfo = S_RED"--"S_WHITE;	// this situation is error, but it can happens when shader have lightstyles (no static version)
			break;
		default:
			lmInfo = va("%d", sh->lightmapNumber);
		}
		if (sh->style & SHADER_WALL)
			color = S_GREEN;
		else if (sh->style & SHADER_SKIN)
			color = S_CYAN;
		else
			color = "";

		Com_Printf ("%-3d %d  %2s %-2g %3s  %s%s%s\n", i, sh->numStages, lmInfo,
			sh->sortParam, shTypes[sh->type], color, sh->name, badNames[sh->bad]);
	}
	Com_Printf ("Displayed %d/%d shaders\n", n, shaderCount);
}


void InitShaders (void)
{
	RegisterCommand ("shaderlist", Shaderlist_f);

	/*------- reading scripts --------*/
	//!!

	ResetShaders ();
}


void ShutdownShaders (void)
{
	if (shaderChain)
	{
		delete shaderChain;
		shaderChain = NULL;
	}
	shaderCount = 0;
	UnregisterCommand ("shaderlist");
}


/*-----------------------------------------------------------------------------
	Creating new shader
-----------------------------------------------------------------------------*/

static shader_t sh;
static shaderStage_t st[MAX_SHADER_STAGES];
static image_t *shaderImages[MAX_SHADER_STAGES * MAX_STAGE_TEXTURES];
static int numTcModStages;
static tcModParms_t tcMods[MAX_SHADER_STAGES * MAX_STAGE_TCMODS];


static void ResortShader (shader_t *shader, int startIndex)
{
	// compute secondary sort key
	int numTextures = 0;
	int sort2 = 0;
	int	i;
	shaderStage_t **pstages;
	for (i = 0, pstages = shader->stages; i < shader->numStages; i++, pstages++)		// iterate stages
	{
		shaderStage_t *st = *pstages;
		image_t *image = st->mapImage[0];

		int texnum;
		if (image && !(shader->lightmapNumber == LIGHTMAP_RESERVE && st->isLightmap))
			texnum = image->texnum;
		else	// ignore image if this is reserved lightmap
			texnum = 1023;		// large positive number (put it to the end of sortParam2, so ResortShader() will work later)
		sort2 = (sort2 << 10) | texnum;

		if (++numTextures == 3)	// not enough place in sortParam2
			break;
	}
	shader->sortParam2 = sort2;

	// insert into a shadersArray (sorted)
	float sort = shader->sortParam;
	for (i = startIndex - 1; i >= 0; i--)
	{
		shader_t *ash = shadersArray[i];	// array shader
		if (sort < ash->sortParam || (sort == ash->sortParam && sort2 < ash->sortParam2))
		{
			// move this shader down
			ash->sortIndex++;
			shadersArray[i + 1] = ash;
		}
		else
		{
			// insert new shader after this place
			i++;
			break;
		}
	}
	if (i < 0) i = 0;	// will be first (for example, when shaderCount == 0)
	shadersArray[i] = shader;
	shader->sortIndex = i;
	if (i <= gl_state.maxUsedShaderIndex)
		InsertShaderIndex (i);
}


static void ClearTempShader (void)
{
	memset (&sh, 0, sizeof(sh));
	CALL_CONSTRUCTOR(&sh);
	memset (&st, 0, sizeof(st));
	memset (&shaderImages, 0, sizeof(shaderImages));
	memset (&tcMods, 0, sizeof(tcMods));
	numTcModStages = 0;
}


static tcModParms_t *NewTcModStage (shaderStage_t *stage)
{
	if (stage->numTcMods > MAX_STAGE_TCMODS)
		Com_DropError ("Too many tcMod stages in shader \"%s\"\n", sh.name);
	//?? check index overflow (no MAX_STAGE_TCMODS but MAX_SHADER_TCMODS ??)
	tcModParms_t *par = &tcMods[numTcModStages++];	// alloc
	if (!stage->numTcMods)
		stage->tcModParms = par;		// if 1st tcMod in stage -- setup pointer
	stage->numTcMods++;
	return par;
}


// Insert shader (sh) into shaders array
static shader_t *CreateShader (void)
{
	if (shaderCount >= MAX_SHADERS)
	{
		Com_WPrintf ("CreateShader(%s): MAX_SHADERS hit\n", sh.name);
		return gl_defaultShader;
	}

	// allocate and copy new shader
	shader_t *nsh = (shader_t*) shaderChain->Alloc(sizeof(shader_t) + (sh.numStages-1) * sizeof(shaderStage_t*));
	*nsh = sh;
	CALL_CONSTRUCTOR(nsh);				// after copy

	// allocate and copy stages
	for (int i = 0; i < sh.numStages; i++)
	{
		int size = sizeof(shaderStage_t) + (st[i].numAnimTextures-1) * sizeof(image_t*);
		shaderStage_t *nst = (shaderStage_t*) shaderChain->Alloc(size);
		*nst = st[i];					// copy contents
		nsh->stages[i] = nst;			// register stage
		// setup mapImage[]
		memcpy (&nst->mapImage, &shaderImages[i * MAX_STAGE_TEXTURES], st[i].numAnimTextures * sizeof(image_t*));
	}

	// insert into a hash table
	int hash = ComputeHash (sh.name);
	nsh->hashNext = hashTable[hash];
	hashTable[hash] = nsh;

	ResortShader (nsh, shaderCount);
	shaderCount++;

	return nsh;
}


// Expand shader from valid shader_t structure to our temporary vars (sh, st ...)
// (reverse to CreatePermanentShader())
// WARNING: this function will not extract tcMods
static void ExtractShader (shader_t *shader)
{
	ClearTempShader ();
	memcpy (&sh, shader, sizeof(shader_t));
	for (int i = 0; i < shader->numStages; i++)
	{
		shaderStage_t *stage = shader->stages[i];
		st[i] = *stage;
		memcpy (&shaderImages[i * MAX_STAGE_TEXTURES], stage->mapImage, stage->numAnimTextures * sizeof(image_t*));
	}
}


static shader_t *FinishShader (void)	//!!!! rename function
{
	if (sh.type == SHADERTYPE_SKY)
		sh.sortParam = SORT_SKY;
	else if (!sh.sortParam && sh.usePolygonOffset)
		sh.sortParam = SORT_DECAL;

	// enum and count stages
	for (int numStages = 0; numStages < MAX_SHADER_STAGES; numStages++)
	{
		shaderStage_t *s = &st[numStages];
		if (!s->numAnimTextures) break;

		// set default tcGenType
		if (!s->tcGenType) s->tcGenType = (s->isLightmap) ? TCGEN_LIGHTMAP : TCGEN_TEXTURE;

		// process rgbGen
		if (s->rgbGenType == RGBGEN_VERTEX && gl_config.identityLightValue_f == 1.0f)
			s->rgbGenType = RGBGEN_EXACT_VERTEX;	// faster to fill

		// process alphaGen
		if (s->alphaGenType == ALPHAGEN_IDENTITY)
		{
			s->rgbaConst.c[3] = 255;
			s->alphaGenType = ALPHAGEN_CONST;
		}

		// replace blend mode aliases with main modes (for easier processing)
		unsigned blend1 = s->glState & (GLSTATE_SRCMASK|GLSTATE_DSTMASK);
		unsigned blend2 = blend1;
		switch (blend1)
		{
		case GLSTATE_SRC_ONE|GLSTATE_DST_ZERO:		// src
			blend2 = 0;
			break;
		case GLSTATE_SRC_ZERO|GLSTATE_DST_SRCCOLOR:	// src*dst
			blend2 = GLSTATE_SRC_DSTCOLOR|GLSTATE_DST_ZERO;
			break;
		//?? can remove/signal stages with blend = (0,1=dst) (0,0=0) and no depthwrite (BUT: depth-sorted anyway ???)
		}
		// replace lightmap blend src*dst -> src*dst*2 when needed
		if (numStages > 0 && st[numStages-1].isLightmap)
		{
			if (blend2 == (GLSTATE_SRC_DSTCOLOR|GLSTATE_DST_ZERO))			// src*dst
			{
				if (gl_config.doubleModulateLM)
					blend2 = GLSTATE_SRC_DSTCOLOR|GLSTATE_DST_SRCCOLOR;		// src*dst*2
			}
			else
				Com_WPrintf ("R_FinishShader(%s): strange blend for lightmap in stage %d\n", sh.name, numStages);	//?? DPrintf
		}
		// store new blend mode
		s->glState = s->glState & ~(GLSTATE_SRCMASK|GLSTATE_DSTMASK) | blend2;

		// set sort param for blending
		if (blend2 && st[0].glState & (GLSTATE_SRCMASK|GLSTATE_DSTMASK))	//?? check this condition
		{	// have blending
			if (!sh.sortParam)
				sh.sortParam = (s->glState & GLSTATE_DEPTHWRITE) ? SORT_SEETHROUGH : SORT_SPRITE;
		}

		// alloc/copy tcMod
		if (s->numTcMods)
		{
			int size = s->numTcMods * sizeof(tcModParms_t);
			tcModParms_t *tc = (tcModParms_t*) shaderChain->Alloc(size);
			memcpy (tc, s->tcModParms, size);
			s->tcModParms = tc;
		}

		// check entity dependence
		if (s->rgbGenType == RGBGEN_ENTITY || s->rgbGenType == RGBGEN_ONE_MINUS_ENTITY || s->rgbGenType == RGBGEN_DIFFUSE ||
			s->alphaGenType == ALPHAGEN_ENTITY || s->alphaGenType == ALPHAGEN_ONE_MINUS_ENTITY ||
			s->frameFromEntity)
			sh.dependOnEntity = true;
	}
	sh.numStages = numStages;

	// if sortParam is not yet set - set it to opaque
	if (!sh.sortParam)
		sh.sortParam = SORT_OPAQUE;

	return CreateShader ();
}


image_t *GetLightmapImage (int num)
{
	if (num == LIGHTMAP_RESERVE)
		return NULL;

	return FindImage (va("*lightmap%d", num), IMAGE_CLAMP);
}


shader_t *SetShaderLightmap (shader_t *shader, int lightmapNumber)
{
	if (lightmapNumber == LIGHTMAP_NONE)
	{
		if (shader->lightmapNumber != LIGHTMAP_VERTEX || shader->numStages != 1)
		{
			Com_DPrintf ("SetLM(NONE) for non-vertexLM shader %s\n", shader->name);
			shader->lightmapNumber = LIGHTMAP_NONE;		// bad situation, but ...
		}
		else
		{
			// HERE: 1-stage vertex-light shader
			shader->lightmapNumber = LIGHTMAP_NONE;
			shader->stages[0]->rgbGenType = RGBGEN_IDENTITY_LIGHTING;
		}
		return shader;
	}

	if (shader->lightmapNumber == LIGHTMAP_VERTEX)
		return shader;

	int hash = ComputeHash (shader->name);
	shader_t *dest = NULL;
	for (shader_t *s = hashTable[hash]; s; s = s->hashNext)
		if (!strcmp (shader->name, s->name) && shader->style == s->style)
		{
			if (s->lightmapNumber == lightmapNumber)
				return s;	// found exact shader as required

			if (s->lightmapNumber == LIGHTMAP_RESERVE)
			{	// found shader with reserved lightmap
				dest = s;
				break;
			}
		}

	if (!dest)
	{
		// shader is not found -- duplicate source and change its lightmap
		ExtractShader (shader);
		sh.lightmapNumber = LIGHTMAP_RESERVE;	// temporarily mark as reserved (for CreateShader())
		dest = CreateShader ();
		// new shader's tcMod will point to the old shader's tcMod stages
	}

	dest->lightmapNumber = lightmapNumber;
	// find lightmap stage
	for (int i = 0; i < dest->numStages; i++)
	{
		shaderStage_t *stage = dest->stages[i];

		if (stage->isLightmap)
		{
			stage->mapImage[0] = GetLightmapImage (lightmapNumber);
			ResortShader (dest, dest->sortIndex);
			return dest;
		}
	}

	Com_DropError ("SetShaderLightmap(%s, %d): lightmap stage is not found", shader->name, lightmapNumber);
	return NULL;	// make compiler happy (will not go here)
}


shader_t *SetShaderLightstyles (shader_t *shader, unsigned styles)
{
	if (!styles || gl_config.vertexLight) return shader;

	char newname[MAX_QPATH];
	strcpy (newname, shader->name);
	char *s = strchr (newname, 0);			// find string end
	*s++ = '$';

	for (unsigned m = styles; m; m >>= 8)
	{
		// style -> char
		byte c = m & 0xFF;

		if (c < 10) c += '0';
		else c += 'a' - 10;
		*s++ = c;
	}
	*s = 0;

	shader_t *newshader = FindShader (newname, shader->style | SHADER_CHECKLOADED);
	if (newshader) return newshader;

	ExtractShader (shader);
	strcpy (sh.name, newname);
	sh.lightStyles_i = styles;
	return CreateShader ();
}


shader_t *GetAlphaShader (shader_t *shader)
{
	if (shader->alphaShader)
		return shader->alphaShader;			// already done

	if (shader->stages[0]->glState & (GLSTATE_SRCMASK|GLSTATE_DSTMASK))
	{	// already have blend mode
		shader->alphaShader = shader;
		Com_DPrintf ("GetAlphaShader(%s): already have blend\n", shader->name);
		return shader;
	}

	// clone shader
	ExtractShader (shader);
	strcat (sh.name, "#a");					// just make different name
	// setup alpha/blend
	sh.sortParam = SORT_SEETHROUGH;			//?? SORT_SPRITE? check SHADER_WALL ??
	st[0].alphaGenType = ALPHAGEN_ENTITY;
	st[0].glState |= GLSTATE_SRC_SRCALPHA|GLSTATE_DST_ONEMINUSSRCALPHA;
	st[0].glState &= ~GLSTATE_DEPTHWRITE;	// allow to see inside alpha-model
	// setup shader images
	for (int i = 0; i < sh.numStages; i++)
		for (int j = 0; j < st[i].numAnimTextures; j++)
		{
			int idx = i * MAX_STAGE_TEXTURES + j;
			image_t *img = shaderImages[idx];
			if (img->name[0] != '*' && !(img->flags & IMAGE_SYSTEM) && img->flags & IMAGE_NOALPHA)
				shaderImages[idx] = FindImage (img->name, img->flags & ~IMAGE_NOALPHA);
		}

	shader->alphaShader = FinishShader ();
	return shader->alphaShader;
}


// "mipmap" is used only for auto-generated shaders
shader_t *FindShader (const char *name, unsigned style)
{
	char	name2[MAX_QPATH], *s;
	shader_t *shader;

	guard(FindShader);

	// compute image flags
	unsigned imgFlags = (style & (SHADER_WALL|SHADER_SKIN)) ? (IMAGE_PICMIP|IMAGE_MIPMAP) : 0;
	if (!(style & (SHADER_ALPHA|SHADER_FORCEALPHA|SHADER_TRANS33|SHADER_TRANS66)))
		imgFlags |= IMAGE_NOALPHA;
	if (style & SHADER_WALL) imgFlags |= IMAGE_WORLD;
	if (style & SHADER_SKIN) imgFlags |= IMAGE_SKIN;
	if (style & SHADER_CLAMP) imgFlags |= IMAGE_CLAMP;

	if (style & SHADER_ENVMAP && !gl_reflImage)	// remove reflection if nothing to apply
		style &= ~SHADER_ENVMAP;

	// debug: gl_singleShader
	if (gl_singleShader->integer && name[0] != '*')
	{
		if (style & SHADER_LIGHTMAP)
		{
			style &= ~SHADER_ANIM;	// no animation in this mode
			name = "*default";		// use this image
		}
		else if (style & SHADER_SKIN && gl_singleShader->integer == 2)
			name = "*default";
	}

	appCopyFilename (name2, name, sizeof(name2));
	if (s = strrchr (name2, '.')) *s = 0;		// cut extension

	int lightmapNumber = LIGHTMAP_NONE;
	if (style & SHADER_LIGHTMAP)
	{
		if (gl_config.vertexLight)
			lightmapNumber = LIGHTMAP_VERTEX;
		else
			lightmapNumber = LIGHTMAP_RESERVE;
	}

	/*----- find shader using hash table -----*/
	int hash = ComputeHash (name2);
	for (shader = hashTable[hash]; shader; shader = shader->hashNext)
		if (!strcmp (shader->name, name2))
		{
			if (shader->bad)					// generated by bad script
				return shader;

			if (shader->style == (style & SHADER_STYLEMASK) || shader->scripted)
			{
				if (shader->lightmapNumber == lightmapNumber)
					return shader;				// exact as required

				if (lightmapNumber == LIGHTMAP_RESERVE)
					return shader;				// want to get reserved -- return any

				if (shader->lightmapNumber == LIGHTMAP_RESERVE)
					return SetShaderLightmap (shader, lightmapNumber);	// reserved -> used
			}
		}
	if (style & SHADER_CHECKLOADED) return NULL;

	// prepate common shader fields
	ClearTempShader ();
	strcpy (sh.name, name2);
	sh.lightmapNumber = lightmapNumber;

//!!	if ()
	/*------------ find script ---------------*/
//	{	WARNING: fill shader->width and height with params of the main image
//	}
//	else
	/*----- create shader without script -----*/
	{
		image_t *img;

		sh.style = style & SHADER_STYLEMASK;

		if (style & SHADER_SKY)
		{
			sh.type = SHADERTYPE_SKY;

			if (style & SHADER_ABSTRACT)
				return FinishShader ();

			for (int i = 0; i < 6; i++)
			{
				static const char *suff[6] = {"rt", "lf", "bk", "ft", "up", "dn"};

				img = FindImage (va("%s%s", name2, suff[i]), IMAGE_CLAMP);
				if (!img)
				{
					sh.bad = true;
					break;
				}
				if (!i)
				{
					sh.width = img->width;
					sh.height = img->height;
				}
				sh.skyFarBox[i] = img;
			}
			if (!sh.bad)							// valid sky
				return FinishShader ();
			else
				return gl_defaultSkyShader;
		}

		// regular shader
		st[0].numAnimTextures = 1;

		if (style & SHADER_ABSTRACT)
			img = gl_defaultImage;					// just any image (will be removed later anyway)
		else
			img = FindImage (name, imgFlags);	// use "name", not "name2" (full name, may be - with extension)

		if (!img)
		{
			if (style & SHADER_CHECK)
				return NULL;	// do not return default shader

			Com_WPrintf ("R_FindShader: couldn't find image \"%s\"\n", name);
			sh.bad = true;
			img = gl_defaultImage;
		}
//	??	else -- if we enable this, map will not be loaded when wall texture absent (no lightmap stage will be created)
		{
			sh.width = img->width;
			sh.height = img->height;

			shaderStage_t *stage = &st[0];
			int stageIdx = 0;

			if (style & SHADER_TRYLIGHTMAP)
			{
				lightmapNumber = LIGHTMAP_NONE;
				stage->rgbGenType = (style & (SHADER_TRANS33|SHADER_TRANS66)) ? RGBGEN_BOOST_VERTEX : RGBGEN_EXACT_VERTEX;
				sh.lightmapNumber = LIGHTMAP_VERTEX;
			}

			if (lightmapNumber >= 0)
			{
				shaderImages[0] = GetLightmapImage (lightmapNumber);
				stage->isLightmap = true;
				stage->glState = GLSTATE_DEPTHWRITE;
				stage->rgbGenType = RGBGEN_IDENTITY;
				stage->alphaGenType = ALPHAGEN_IDENTITY;
				stage++;	// add next stage
				stageIdx++;
				stage->numAnimTextures = 1;
			}

			shaderImages[stageIdx * MAX_STAGE_TEXTURES] = img;
			if (style & SHADER_ANIM)
			{
				char *pname = strchr (name, 0) + 1;
				stage->animMapFreq = 2;			// standard Quake2 animation frequency
				stage->frameFromEntity = true;	// entities can animate with a different frames than world
				for (int i = 1; *pname && i < MAX_STAGE_TEXTURES; i++, pname = strchr(pname, 0)+1)
				{
					img = FindImage (pname, imgFlags);
					if (!img)
					{
						Com_WPrintf ("R_FindShader: couldn't find image \"%s\"\n", pname);
						stage->numAnimTextures = 1;
						sh.bad = true;
						break;
					}
					shaderImages[stageIdx * MAX_STAGE_TEXTURES + i] = img;
					stage->numAnimTextures++;
				}
			}

			/*--------- processing style -------------*/
			if (lightmapNumber >= 0)
			{
				stage->glState = GLSTATE_SRC_DSTCOLOR|GLSTATE_DST_ZERO;		// 2nd stage -- texture (src*dst)
				stage->rgbGenType = RGBGEN_IDENTITY;
				stage->alphaGenType = ALPHAGEN_IDENTITY;
			}
			else if (lightmapNumber == LIGHTMAP_VERTEX)
			{
				stage->glState = GLSTATE_DEPTHWRITE;
				stage->rgbGenType = RGBGEN_EXACT_VERTEX;
				stage->alphaGenType = ALPHAGEN_VERTEX;	// no alpha for lightmapped surface, but rgb=vertex and alpha=vertex -- fast
			}
			else if (style & SHADER_FORCEALPHA)
			{
				stage->glState = GLSTATE_SRC_SRCALPHA|GLSTATE_DST_ONEMINUSSRCALPHA|GLSTATE_ALPHA_GT0;
						// image has no alpha, but use glColor(x,x,x,<1)
				stage->alphaGenType = ALPHAGEN_VERTEX;
			}
			else if (style & SHADER_ALPHA && img->alphaType)
			{
				if (style & SHADER_WALL)
					stage->glState = GLSTATE_SRC_SRCALPHA|GLSTATE_DST_ONEMINUSSRCALPHA|GLSTATE_DEPTHWRITE|GLSTATE_ALPHA_GT0;
				else
				{
					if (img->alphaType == 1)
						stage->glState = GLSTATE_ALPHA_GE05;
					else
						stage->glState = GLSTATE_SRC_SRCALPHA|GLSTATE_DST_ONEMINUSSRCALPHA|GLSTATE_ALPHA_GT0;
				}
			}
			else if (style & (SHADER_TRANS33|SHADER_TRANS66))
			{
				float	alpha;

				stage->glState = GLSTATE_SRC_SRCALPHA|GLSTATE_DST_ONEMINUSSRCALPHA|GLSTATE_DEPTHWRITE;
				if (style & SHADER_TRANS33)
				{
					if (style & SHADER_TRANS66)	alpha = 0.22;	// both flags -- make it more translucent
					else						alpha = 0.33;
				}
				else alpha = 0.66;

#if 0
				stage->alphaGenType = ALPHAGEN_CONST;
				stage->rgbaConst.c[3] = appRound (alpha * 255);
#else
				stage->alphaGenType = ALPHAGEN_ONE_MINUS_DOT;
				stage->alphaMin = alpha * 2 / 3;
				stage->alphaMax = 0.8;
#endif
			}
//??			if (!(stage->glState & (GLSTATE_SRCMASK|GLSTATE_DSTMASK)) && lightmapNumber == LIGHTMAP_NONE)
//				stage->glState |= GLSTATE_DEPTHWRITE;

			if (style & SHADER_TURB)
			{
				tcModParms_t	*tcmod;
#if 0
				// scale
				tcmod = NewTcModStage (stage);
				tcmod->type = TCMOD_SCALE;
				tcmod->sScale = tcmod->tScale = 1.0f / 64;
				// turb
				tcmod = NewTcModStage (stage);
				tcmod->type = TCMOD_TURB;
				tcmod->wave.amp = 1.0f / 16;
				tcmod->wave.phase = 0;
				tcmod->wave.freq = 1.0f / (2.0f * M_PI);
#else
				tcmod = NewTcModStage (stage);
				tcmod->type = TCMOD_WARP;
#endif

#if 0
				{
					deformParms_t	*deform;

					// deform verts
					deform = &sh.deforms[0];
					deform->type = DEFORM_WAVE;
					deform->wave.type = FUNC_SIN;
					deform->wave.amp = 2;
					deform->wave.freq = 0.3;
					deform->wave.base = 0;
					deform->wave.phase = 0.5;
					deform->waveDiv = 1.0/(64*16);
					sh.numDeforms = 1;
				}
#endif
				// signal for map loader to subtesselate surface
				sh.tessSize = 64;
			}

			if (style & SHADER_SCROLL)
			{
				tcModParms_t *par = NewTcModStage (stage);
				par->type = TCMOD_SCROLL;
				if (style & SHADER_TURB)
					par->sSpeed = -0.5;
				else
					par->sSpeed = -64.0f / 40;
				par->tSpeed = 0;
			}

			if (!stage->rgbGenType)
			{
				if (style & SHADER_WALL)
					stage->rgbGenType = RGBGEN_IDENTITY_LIGHTING;	// world surface, not lightmapped
				else if (style & SHADER_SKIN)
					stage->rgbGenType = RGBGEN_DIFFUSE;
				else
					stage->rgbGenType = RGBGEN_VERTEX;				// HUD image
			}
			if (style & (SHADER_WALL|SHADER_SKIN))					// 3D shader
				stage->glState = stage->glState & ~GLSTATE_NODEPTHTEST | GLSTATE_DEPTHWRITE;
			else													// 2D shader
				stage->glState = stage->glState & ~GLSTATE_DEPTHWRITE | GLSTATE_NODEPTHTEST;
			if (lightmapNumber >= 0)
				stage->glState &= ~GLSTATE_DEPTHWRITE;				// depthwrite performed on lightmap stage (only) - for small speedup

			if (style & SHADER_ABSTRACT)
				st[0].numAnimTextures = 0;			// remove all stages

			if (style & (SHADER_ENVMAP|SHADER_ENVMAP2))
			{
				stage++;	// add next stage
				stageIdx++;
				stage->numAnimTextures = 1;
				shaderImages[stageIdx * MAX_STAGE_TEXTURES] = style & SHADER_ENVMAP ? gl_reflImage : gl_reflImage2;
				stage->glState = GLSTATE_SRC_SRCALPHA|GLSTATE_DST_ONE;
				stage->rgbGenType = RGBGEN_EXACT_VERTEX;
				// ?? should be VERTEX for non-[vertex-]lightmapped surfs, EXACT_VERTEX for LM
				//    Why: vertex color computed already lightscaled (for Q2 maps, at least) -- EXACT_VERTEX; when
				//    using VERTEX in backend, it will be lightscaled again -- darker when overbright>0.
				//    SHADER_ENVMAP[2] is always for this sort of surfaces ? (always world, Q2 (non-Q3))
				stage->alphaGenType = ALPHAGEN_CONST;
				stage->rgbaConst.c[3] = 128;
				stage->tcGenType = TCGEN_ENVIRONMENT;
			}
			if (/* style & (SHADER_ENVMAP|SHADER_ENVMAP2) && */ style & (SHADER_TRANS33|SHADER_TRANS66))
				sh.tessSize = 64;		//!! add cvar to enable/disable this (high-quality alpha (lighting/envmapping))
		}
	}
	return FinishShader ();

	unguardf(("%s", name));
}


shader_t *GetShaderByNum (int num)
{
	if (num >= 0 && num < shaderCount)
		return shadersArray[num];
	return gl_defaultShader;
}


void ResetShaders (void)
{
	tcModParms_t	*tcmod;
	deformParms_t	*deform;

	guard(ResetShaders);

	if (shaderChain) delete shaderChain;
	shaderChain = new CMemoryChain;

	memset (hashTable, 0, sizeof(hashTable));
	memset (shadersArray, 0, sizeof(shadersArray));
	shaderCount = 0;

	/*---------------- creating system shaders --------------------*/
	//!! most of this shaders can be created with script

	// abstract shaders should be created in reverse (with relation to sortParam) order
	gl_alphaShader2 = FindShader ("*alpha2", SHADER_ABSTRACT);
	gl_alphaShader2->sortParam = SORT_SEETHROUGH + 1;
	gl_entityShader = FindShader ("*entity", SHADER_ABSTRACT);
	gl_entityShader->sortParam = SORT_SEETHROUGH;
	gl_particleShader = FindShader ("*particle", SHADER_ABSTRACT);	//!! NOCULL
	gl_particleShader->sortParam = SORT_SEETHROUGH;
	gl_alphaShader1 = FindShader ("*alpha1", SHADER_ABSTRACT);
	gl_alphaShader1->sortParam = SORT_SEETHROUGH;

	gl_defaultShader = FindShader ("*default", SHADER_WALL);
	gl_videoShader = FindShader ("*video", SHADER_CLAMP);

	gl_identityLightShader = FindShader ("*identityLight", SHADER_FORCEALPHA|SHADER_WALL);
	gl_identityLightShader->stages[0]->rgbGenType = RGBGEN_EXACT_VERTEX;
	gl_identityLightShader->stages[0]->glState |= GLSTATE_NODEPTHTEST;		// remove depth test/write (this is 2D shader)
	gl_identityLightShader->stages[0]->glState &= ~GLSTATE_DEPTHWRITE;

	// create 2nd "identityLight" (with depth test/write and different name)
	strcpy (sh.name, "*identitylight2");
	st[0].rgbGenType = RGBGEN_EXACT_VERTEX;
	gl_identityLightShader2 = FinishShader ();

	strcpy (sh.name, "*flare");
	sh.sortParam = SORT_SPRITE;
	sh.cullMode = CULL_NONE;
	st[0].rgbGenType = RGBGEN_VERTEX;
#if 0
//	st[0].alphaGenType = ALPHAGEN_VERTEX;
	st[0].glState = GLSTATE_NODEPTHTEST|GLSTATE_SRC_SRCCOLOR|GLSTATE_DST_ONE;
#else
	st[0].alphaGenType = ALPHAGEN_VERTEX;
	st[0].glState = GLSTATE_NODEPTHTEST|GLSTATE_SRC_SRCALPHA|GLSTATE_DST_ONE;
#endif
	shaderImages[0] = FindImage ("sprites/corona", IMAGE_MIPMAP|IMAGE_TRUECOLOR);
	if (shaderImages[0])
		gl_flareShader = FinishShader ();
	else
		gl_flareShader = NULL;

	//---------------------------------------------
	ClearTempShader ();
	strcpy (sh.name, "*colorShell");
	sh.sortParam = SORT_SEETHROUGH;
	st[0].rgbGenType = RGBGEN_ENTITY;
#if 1
	st[0].alphaGenType = ALPHAGEN_IDENTITY;
	st[0].glState = GLSTATE_SRC_ONE|GLSTATE_DST_ONE|GLSTATE_DEPTHWRITE;
#else
	st[0].alphaGenType = ALPHAGEN_DOT;
	st[0].alphaMin = 0;
	st[0].alphaMax = 1;
	st[0].glState = GLSTATE_SRC_SRCALPHA|GLSTATE_DST_ONE|GLSTATE_DEPTHWRITE;
#endif
	st[0].tcGenType = TCGEN_ENVIRONMENT;
	// rotate
	tcmod = NewTcModStage (st);
	tcmod->type = TCMOD_ROTATE;
	tcmod->rotateSpeed = 30;
	// scroll
	tcmod = NewTcModStage (st);
	tcmod->type = TCMOD_SCROLL;
	tcmod->sSpeed = 1;
	tcmod->tSpeed = 0.1;
	// deform verts
	deform = &sh.deforms[0];
	deform->type = DEFORM_WAVE;
	deform->wave.type = FUNC_SIN;
	deform->wave.amp = 0;
//	deform->wave.freq = 0;
	deform->wave.base = 2;		//!! fromEntity: 3 / 0.5
//	deform->wave.phase = 0;
//	deform->waveDiv = 0;
	sh.numDeforms = 1;
	shaderImages[0] = FindImage ("fx/colorshell", IMAGE_MIPMAP);
	sh.lightmapNumber = LIGHTMAP_NONE;
	st[0].numAnimTextures = 1;
	gl_colorShellShader = FinishShader ();

	//---------------------------------------------
	ClearTempShader ();
	strcpy (sh.name, "*railBeam");
	sh.sortParam = SORT_SEETHROUGH;
	sh.cullMode = CULL_NONE;
	st[0].rgbGenType = RGBGEN_VERTEX;
	st[0].alphaGenType = ALPHAGEN_VERTEX;
	st[0].glState = GLSTATE_SRC_SRCALPHA|GLSTATE_DST_ONE;//MINUSSRCALPHA/*|GLSTATE_DEPTHWRITE|GLSTATE_ALPHA_GT0*/;
	st[0].tcGenType = TCGEN_TEXTURE;
	// scale
	tcmod = NewTcModStage (st);
	tcmod->type = TCMOD_SCALE;
	tcmod->tScale = 1.0f / 12;
	tcmod->sScale = 1;
	shaderImages[0] = FindImage ("fx/rail2", 0);		// no mipmaps here
	sh.lightmapNumber = LIGHTMAP_NONE;
	st[0].numAnimTextures = 1;
	gl_railBeamShader = FinishShader ();

	//---------------------------------------------
	ClearTempShader ();
	strcpy (sh.name, "*railSpiral");
	sh.sortParam = SORT_SEETHROUGH;
	sh.cullMode = CULL_NONE;
	st[0].rgbGenType = RGBGEN_VERTEX;
	st[0].alphaGenType = ALPHAGEN_VERTEX;
	st[0].glState = GLSTATE_SRC_SRCALPHA|GLSTATE_DST_ONE;//MINUSSRCALPHA/*|GLSTATE_DEPTHWRITE|GLSTATE_ALPHA_GT0*/;
	st[0].tcGenType = TCGEN_TEXTURE;
	// scale
	tcmod = NewTcModStage (st);
	tcmod->type = TCMOD_SCALE;
	tcmod->tScale = 1.0f / 63;
	tcmod->sScale = 1;
	// scroll
	tcmod = NewTcModStage (st);
	tcmod->type = TCMOD_SCROLL;
	tcmod->tSpeed = 2;
	shaderImages[0] = FindImage ("fx/rail", 0);			// no mipmaps here
	sh.lightmapNumber = LIGHTMAP_NONE;
	st[0].numAnimTextures = 1;
	gl_railSpiralShader = FinishShader ();

	//---------------------------------------------
	ClearTempShader ();
	strcpy (sh.name, "*railRings");
	sh.sortParam = SORT_SEETHROUGH;
	sh.cullMode = CULL_NONE;
	st[0].rgbGenType = RGBGEN_VERTEX;
	st[0].alphaGenType = ALPHAGEN_VERTEX;
	st[0].glState = GLSTATE_SRC_SRCALPHA|GLSTATE_DST_ONE;//MINUSSRCALPHA/*|GLSTATE_DEPTHWRITE|GLSTATE_ALPHA_GT0*/;
	st[0].tcGenType = TCGEN_TEXTURE;
	// scale
	tcmod = NewTcModStage (st);
	tcmod->type = TCMOD_SCALE;
	tcmod->sScale = 0;
	tcmod->tScale = 1.0f / 35;
	// scroll
	tcmod = NewTcModStage (st);
	tcmod->type = TCMOD_SCROLL;
	tcmod->tSpeed = 4;
	shaderImages[0] = FindImage ("fx/rail", 0);			// no mipmaps here
	sh.lightmapNumber = LIGHTMAP_NONE;
	st[0].numAnimTextures = 1;
	gl_railRingsShader = FinishShader ();

	//---------------------------------------------

	gl_skyShader = gl_defaultSkyShader = FindShader ("*sky", SHADER_SKY|SHADER_ABSTRACT);

	gl_detailShader = FindShader ("*detail", SHADER_ALPHA|SHADER_WALL);
	if (gl_detailShader)
	{
		gl_detailShader->cullMode = CULL_NONE;
		gl_detailShader->stages[0]->glState = GLSTATE_NODEPTHTEST|GLSTATE_SRC_DSTCOLOR|GLSTATE_DST_SRCCOLOR;
	}

#if 1
	gl_concharsShader = FindShader ("pics/conchars", SHADER_ALPHA);
#else
	//?? font with shadow: require bitmap modification + avoid shadow from one char to another char
	ClearTempShader ();
	strcpy (sh.name, "pics/conchars");
	sh.style = SHADER_ALPHA;
	sh.sortParam = SORT_OPAQUE;	//??
	sh.cullMode = CULL_NONE;
	sh.lightmapNumber = LIGHTMAP_NONE;
	// 1st stage
	st[0].rgbGenType = RGBGEN_IDENTITY;	//?? same as stage2
	st[0].alphaGenType = ALPHAGEN_IDENTITY;
	st[0].glState = GLSTATE_ALPHA_GE05|GLSTATE_NODEPTHTEST|GLSTATE_SRC_ZERO|GLSTATE_DST_ONEMINUSSRCALPHA;
	st[0].tcGenType = TCGEN_TEXTURE;
	shaderImages[0] = FindImage ("pics/conchars", IMAGE_CLAMP);
	st[0].numAnimTextures = 1;
	// tcMod for 1st stage
	tcmod = NewTcModStage (st);
	tcmod->type = TCMOD_OFFSET;
	tcmod->sOffset = tcmod->tOffset = -1.0f / 128;
	// 2nd stage
	st[1].rgbGenType = RGBGEN_VERTEX;
	st[1].alphaGenType = ALPHAGEN_IDENTITY;	//?? same as rgb
	st[1].glState = GLSTATE_ALPHA_GE05|GLSTATE_NODEPTHTEST|GLSTATE_SRC_ONE|GLSTATE_DST_ONEMINUSSRCALPHA;
	st[1].tcGenType = TCGEN_TEXTURE;
	shaderImages[MAX_STAGE_TEXTURES] = FindImage ("pics/conchars", IMAGE_CLAMP);
	st[1].numAnimTextures = 1;

	gl_concharsShader = FinishShader ();
#endif

	unguard;
}


/*-----------------------------------------------------------------------------
	shader_t
-----------------------------------------------------------------------------*/

void shader_t::Reload ()
{
	//?? needs to reload shader script -- for debug only
	if (numStages && stages[0]->numAnimTextures)
	{
		image_t *img = stages[0]->mapImage[0];	//?? all images (non-system, not "*name" etc), all anims, all stages
		FindImage (img->name, img->flags | IMAGE_RELOAD);
	}
}


} // namespace
