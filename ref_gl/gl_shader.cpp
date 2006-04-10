#include "OpenGLDrv.h"
#include "gl_shader.h"
#include "gl_buffers.h"

//#define DEBUG_SHADERS		1
#if MAX_DEBUG
#define DEBUG_SHADERS		1
#endif


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
shader_t *gl_skyShader;				// current sky shader (have mapped images)
shader_t *gl_alphaShader1, *gl_alphaShader2;

#if DEBUG_SHADERS
static cvar_t *gl_debugShaders;
#endif


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


// forwards
static void FindShaderScripts ();
static void FreeShaderScripts ();
static bool InitShaderFromScript (const char *srcName, const char *text = NULL);


/*-----------------------------------------------------------------------------
	Initialization/finalization
-----------------------------------------------------------------------------*/

static void Shaderlist_f (bool usage, int argc, char **argv)
{
	static const char *shTypes[] = {"", "sky", "fog", "por"};
	static const char *boolNames[] = {" ", "+"};

	if (argc > 2 || usage)
	{
		appPrintf ("Usage: shaderlist [<mask>]\n");
		return;
	}

	const char *mask = (argc == 2) ? argv[1] : NULL;

	int n = 0;
	appPrintf ("----ns-lm-s--type-scr-name--------\n");
	for (int i = 0; i < shaderCount; i++)
	{
		shader_t *sh = shadersArray[i];
		if (mask && !appMatchWildcard (sh->Name, mask, true)) continue;
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

		const char *rem = "";
		if (sh->noDraw) rem = S_YELLOW"   (nodraw)"S_WHITE;
		if (sh->bad) rem = S_RED"   (errors)"S_WHITE;
		if (sh->style & SHADER_ABSTRACT) rem = S_YELLOW"   (abstract)"S_WHITE;
		appPrintf ("%-3d %d  %2s %-2d %3s  %-3s %s%s%s\n", i, sh->numStages, lmInfo,
			sh->sortParam, shTypes[sh->type], boolNames[sh->scripted], color, *sh->Name, rem);
	}
	appPrintf ("Displayed %d/%d shaders\n", n, shaderCount);
}


void InitShaders ()
{
	RegisterCommand ("shaderlist", Shaderlist_f);
#if DEBUG_SHADERS
	gl_debugShaders = Cvar_Get ("gl_debugShaders", "0");
#endif

	// reading scripts
	FindShaderScripts ();
	ResetShaders ();
}


void ShutdownShaders ()
{
	if (shaderChain)
	{
		delete shaderChain;
		shaderChain = NULL;
	}
	shaderCount = 0;
	UnregisterCommand ("shaderlist");
	FreeShaderScripts ();
}


/*-----------------------------------------------------------------------------
	Creating new shader
-----------------------------------------------------------------------------*/

static shader_t			sh;
static shaderStage_t	st[MAX_SHADER_STAGES];
static const image_t	*shaderImages[MAX_SHADER_STAGES * MAX_STAGE_TEXTURES];
static int				numTcModStages;
static tcModParms_t		tcMods[MAX_SHADER_STAGES * MAX_STAGE_TCMODS];


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
		const image_t *image = st->mapImage[0];

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
	int sort = shader->sortParam;
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


static void ClearTempShader ()
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
		Com_DropError ("Too many tcMod stages in shader \"%s\"\n", *sh.Name);	//?? ERROR_IN_SHADER()
	//?? check index overflow (no MAX_STAGE_TCMODS but MAX_SHADER_TCMODS ??)
	tcModParms_t *par = &tcMods[numTcModStages++];	// alloc
	if (!stage->numTcMods)
		stage->tcModParms = par;		// if 1st tcMod in stage -- setup pointer
	stage->numTcMods++;
	return par;
}


// Insert shader (sh) into shaders array
static shader_t *CreateShader ()
{
	if (shaderCount >= MAX_SHADERS)
	{
		appWPrintf ("CreateShader(%s): MAX_SHADERS hit\n", *sh.Name);
		return gl_defaultShader;
	}

	// allocate and copy new shader
	shader_t *nsh = (shader_t*) shaderChain->Alloc(sizeof(shader_t) + (sh.numStages-1) * sizeof(shaderStage_t*));
	*nsh = sh;
	CALL_CONSTRUCTOR(nsh);				// after copy

//	appPrintf(S_GREEN"sh: %s\n",*sh.Name);
	// allocate and copy stages
	for (int i = 0; i < sh.numStages; i++)
	{
		int size = sizeof(shaderStage_t) + (st[i].numAnimTextures-1) * sizeof(image_t*);
		shaderStage_t *nst = (shaderStage_t*) shaderChain->Alloc(size);
		*nst = st[i];					// copy contents
		nsh->stages[i] = nst;			// register stage
		// setup mapImage[]
		memcpy (&nst->mapImage, &shaderImages[i * MAX_STAGE_TEXTURES], st[i].numAnimTextures * sizeof(image_t*));
//		for (int j = 0; j < nst->numAnimTextures; j++)
//			appPrintf("  %d.%d: %s\n",i,j,nst->mapImage[j] ? *nst->mapImage[j]->Name : "$white");
	}

	// insert into a hash table
	int hash = ComputeHash (sh.Name);
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


//!!!! rename function
static shader_t *FinishShader ()
{
	if (sh.type == SHADERTYPE_SKY)
		sh.sortParam = SORT_SKY;
	else if (!sh.sortParam && sh.usePolygonOffset)
		sh.sortParam = SORT_DECAL;

	// enum and count stages
	int numStages;
	for (numStages = 0; numStages < MAX_SHADER_STAGES; numStages++)
	{
		shaderStage_t *s = &st[numStages];
		if (!s->numAnimTextures) break;

		// set default tcGenType
		if (!s->tcGenType) s->tcGenType = (s->isLightmap) ? TCGEN_LIGHTMAP : TCGEN_TEXTURE;

		// set default rgbGenType (identity for lightmap stage, because overbright already corrected in texture)
		if (!s->rgbGenType) s->rgbGenType = (s->isLightmap) ? RGBGEN_IDENTITY : RGBGEN_IDENTITY_LIGHTING;

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
		unsigned blend1 = s->glState & GLSTATE_BLENDMASK;
		unsigned blend2 = blend1;
		switch (blend1)
		{
		case BLEND(1,0):							// src
			blend2 = 0;
			break;
		case BLEND(0,S_COLOR):						// src*dst
			blend2 = BLEND(D_COLOR,0);
			break;
		}
		if (!blend2) s->glState |= GLSTATE_DEPTHWRITE;						// required, when no blending
		// replace lightmap blend src*dst -> src*dst*2 when needed
		if (numStages > 0 && st[numStages-1].isLightmap)
		{
			if (blend2 == BLEND(D_COLOR,0))			// src*dst
			{
				if (gl_config.doubleModulateLM)
					blend2 = BLEND(D_COLOR,S_COLOR);// src*dst*2
			}
			else
				appWPrintf ("R_FinishShader(%s): strange blend for lightmap in stage %d\n", *sh.Name, numStages);	//?? DPrintf
		}
		// store new blend mode
		s->glState = s->glState & ~GLSTATE_BLENDMASK | blend2;

		// set sort param for blending
		if (blend2 && st[0].glState & GLSTATE_BLENDMASK)	//?? check this condition
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
		// check time dependency
		if (s->numAnimTextures > 1 || s->numTcMods || s->rgbGenType == RGBGEN_WAVE || s->alphaGenType == ALPHAGEN_WAVE)
			sh.dependOnTime = true;
		for (int i = 0; i < sh.numDeforms; i++)
			if (sh.deforms[i].type != DEFORM_AUTOSPRITE && sh.deforms[i].type != DEFORM_AUTOSPRITE2)
				sh.dependOnTime = true;
	}
	sh.numStages = numStages;
	if (!numStages && !(sh.style & SHADER_ABSTRACT)) sh.noDraw = true;

	// exchange 1st 2 stages, when 2nd stage uses DSTALPHA (to allow correct drawing in 16-bit modes)
	//?? check: is it correct, when 3rd stage uses DSTALPHA too?
	if (numStages >= 2)
	{
		unsigned blend1 = st[0].glState & (GLSTATE_BLENDMASK|GLSTATE_ALPHAMASK);
		unsigned blend2 = st[1].glState & (GLSTATE_BLENDMASK|GLSTATE_ALPHAMASK);
		if ( blend1 == 0 && ( blend2 == BLEND(M_D_ALPHA,D_ALPHA) || (blend2 == BLEND(D_ALPHA,M_D_ALPHA)) ) )
		{
			// exchange stages
			Exchange (st[0], st[1]);
			Exchange (st[0].glState, st[1].glState);
			st[0].glState &= ~GLSTATE_BLENDMASK;	// blend = 0
			st[1].glState &= ~GLSTATE_BLENDMASK;
			if (blend2 == BLEND(M_D_ALPHA,D_ALPHA))
				st[1].glState |= BLEND(S_ALPHA,M_S_ALPHA);
			else
				st[1].glState |= BLEND(M_S_ALPHA,S_ALPHA);
			// exchange textures
			image_t tmpImg[MAX_STAGE_TEXTURES];
			memcpy (tmpImg, shaderImages, sizeof(tmpImg));
			memcpy (shaderImages, shaderImages + MAX_STAGE_TEXTURES, sizeof(tmpImg));
			memcpy (shaderImages + MAX_STAGE_TEXTURES, tmpImg, sizeof(tmpImg));
			Com_DPrintf ("swapping first 2 stages in shader %s\n", *sh.Name);
		}
	}

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
			Com_DPrintf ("SetLM(NONE) for non-vertexLM shader %s\n", *shader->Name);
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

	int hash = ComputeHash (shader->Name);
	shader_t *dest = NULL;
	for (shader_t *s = hashTable[hash]; s; s = s->hashNext)
		if (shader->Name == s->Name && shader->style == s->style)
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

	Com_DropError ("SetShaderLightmap(%s, %d): lightmap stage is not found", *shader->Name, lightmapNumber);
	return NULL;	// make compiler happy (will not go here)
}


shader_t *SetShaderLightstyles (shader_t *shader, unsigned styles)
{
	if (!styles || gl_config.vertexLight) return shader;

	TString<64> NewName;
	NewName = shader->Name;
	char *s = NewName.chr (0);			// find string end
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

	shader_t *newshader = FindShader (NewName, shader->style | SHADER_CHECKLOADED);
	if (newshader) return newshader;

	ExtractShader (shader);
	sh.Name          = NewName;
	sh.lightStyles_i = styles;
	return CreateShader ();
}


shader_t *GetAlphaShader (shader_t *shader)
{
	if (shader->alphaShader)
		return shader->alphaShader;			// already done

	if (shader->stages[0]->glState & GLSTATE_BLENDMASK)
	{	// already have blend mode
		shader->alphaShader = shader;
		Com_DPrintf ("GetAlphaShader(%s): already have blend\n", *shader->Name);
		return shader;
	}

	// clone shader
	ExtractShader (shader);
	sh.Name += "#a";						// just make different name
	// setup alpha/blend
	sh.sortParam = SORT_SEETHROUGH;			//?? SORT_SPRITE? check SHADER_WALL ??
	st[0].alphaGenType = ALPHAGEN_ENTITY;
	st[0].glState |= BLEND(S_ALPHA,M_S_ALPHA);
	st[0].glState &= ~GLSTATE_DEPTHWRITE;	// allow to see inside alpha-model
	// setup shader images
	for (int i = 0; i < sh.numStages; i++)
		for (int j = 0; j < st[i].numAnimTextures; j++)
		{
			int idx = i * MAX_STAGE_TEXTURES + j;
			const image_t *img = shaderImages[idx];
			if (img->Name[0] != '*' && !(img->flags & IMAGE_SYSTEM) && img->flags & IMAGE_NOALPHA)
				shaderImages[idx] = FindImage (img->Name, img->flags & ~IMAGE_NOALPHA);
		}

	shader->alphaShader = FinishShader ();
	return shader->alphaShader;
}


static unsigned sh_imgFlags;
static unsigned fsModifyCount;		// local copy of GFileSystem->modifyCount

// "mipmap" is used only for auto-generated shaders
shader_t *FindShader (const char *name, unsigned style)
{
	guard(FindShader);

	if (!name || !name[0]) return gl_defaultShader;

	// check requirement to reload shader scripts
	if (GFileSystem->modifyCount != fsModifyCount)
	{
		Com_DPrintf ("FindShader: file system modified, reloading shader scripts\n");
		FindShaderScripts ();
	}

	// compute image flags
	sh_imgFlags = (style & (SHADER_WALL|SHADER_SKIN)) ? (IMAGE_PICMIP|IMAGE_MIPMAP) : 0;
	if (style & SHADER_WALL) sh_imgFlags |= IMAGE_WORLD;
	if (style & SHADER_SKIN) sh_imgFlags |= IMAGE_SKIN;

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

	TString<64> Name2;
	Name2.filename (name);
	char *s;
	if (s = Name2.rchr ('.')) *s = 0;			// cut extension

	int lightmapNumber = LIGHTMAP_NONE;
	if (style & SHADER_LIGHTMAP)
	{
		if (gl_config.vertexLight)
			lightmapNumber = LIGHTMAP_VERTEX;
		else
			lightmapNumber = LIGHTMAP_RESERVE;
	}

	/*----- find shader using hash table -----*/
	int hash = ComputeHash (Name2);
	for (shader_t *shader = hashTable[hash]; shader; shader = shader->hashNext)
		if (shader->Name == Name2)
		{
			if (shader->scripted)				// generated by script -- no flag difference
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
	sh.Name           = Name2;
	sh.lightmapNumber = lightmapNumber;
	sh.style          = style & SHADER_STYLEMASK;

	/*------------ find script ---------------*/
	if (InitShaderFromScript (sh.Name))
		return FinishShader ();

	// script is not found, or it was bad
	if (sh.bad)
	{
		// script have errors, clear shader again
		ClearTempShader ();
		sh.Name           = Name2;
		sh.lightmapNumber = lightmapNumber;
		sh.style          = style & SHADER_STYLEMASK;
		sh.bad            = true;		// stats
	}

	// non-scripted shaders: putge alpha-channel when not required
	if (!(style & (SHADER_ALPHA|SHADER_FORCEALPHA|SHADER_TRANS33|SHADER_TRANS66)))
		sh_imgFlags |= IMAGE_NOALPHA;	//?? add it always, when stage have no alpha-depending blend modes
	// check SHADER_CLAMP (for scripts, should explicitly use clampMap/animClampMap)
	if (style & SHADER_CLAMP) sh_imgFlags |= IMAGE_CLAMP;

	/*----- create shader without script -----*/
	const image_t *img;

	if (style & SHADER_SKY)
	{
		sh.type = SHADERTYPE_SKY;

		if (style & SHADER_ABSTRACT)
			return FinishShader ();

		for (int i = 0; i < 6; i++)
		{
			static const char *suff[6] = {"rt", "lf", "bk", "ft", "up", "dn"};

			img = FindImage (va("%s%s", *Name2, suff[i]), IMAGE_CLAMP);
			if (!img)
			{
				sh.bad = true;
				break;
			}
			if (!i)
			{
				sh.width  = img->width;
				sh.height = img->height;
			}
			sh.skyBox[i] = img;
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
		img = FindImage (name, sh_imgFlags);	// use "name", not "name2" (full name, may be - with extension)

	if (!img)
	{
		if (style & SHADER_CHECK)
			return NULL;	// do not return default shader

		appWPrintf ("R_FindShader: couldn't find image \"%s\"\n", name);
		sh.bad = true;
		img = (style & SHADER_SKIN) ? gl_identityLightImage : gl_defaultImage; // can use NULL as white, but will check img->... below ...
	}

	sh.width  = img->width;
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
		stage->isLightmap   = true;
		stage->glState      = GLSTATE_DEPTHWRITE;
		stage->rgbGenType   = RGBGEN_IDENTITY;
		stage->alphaGenType = ALPHAGEN_IDENTITY;
		stage++;	// add next stage
		stageIdx++;
		stage->numAnimTextures = 1;
	}

	shaderImages[stageIdx * MAX_STAGE_TEXTURES] = img;
	if (style & SHADER_ANIM)
	{
		const char *pname = strchr (name, 0) + 1;
		stage->animMapFreq = 2;			// standard Quake2 animation frequency
		stage->frameFromEntity = true;	// entities can animate with a different frames than world
		for (int i = 1; *pname && i < MAX_STAGE_TEXTURES; i++, pname = strchr(pname, 0)+1)
		{
			img = FindImage (pname, sh_imgFlags);
			if (!img)
			{
				appWPrintf ("R_FindShader: couldn't find image \"%s\"\n", pname);
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
		stage->glState      = BLEND(D_COLOR,0);	// 2nd stage -- texture (src*dst)
		stage->rgbGenType   = RGBGEN_IDENTITY;
		stage->alphaGenType = ALPHAGEN_IDENTITY;
	}
	else if (lightmapNumber == LIGHTMAP_VERTEX)
	{
		stage->glState      = GLSTATE_DEPTHWRITE;
		stage->rgbGenType   = RGBGEN_EXACT_VERTEX;
		stage->alphaGenType = ALPHAGEN_VERTEX;	// no alpha for lightmapped surface, but rgb=vertex and alpha=vertex -- fast
	}
	else if (style & SHADER_FORCEALPHA)
	{
		stage->glState = BLEND(S_ALPHA,M_S_ALPHA)|GLSTATE_ALPHA_GT0;
				// image has no alpha, but use glColor(x,x,x,<1)
		stage->alphaGenType = ALPHAGEN_VERTEX;
	}
	else if (style & SHADER_ALPHA && img->alphaType)
	{
		if (style & SHADER_WALL)
			stage->glState = BLEND(S_ALPHA,M_S_ALPHA)|GLSTATE_DEPTHWRITE|GLSTATE_ALPHA_GT0;
		else
		{
			if (img->alphaType == 1)
				stage->glState = GLSTATE_ALPHA_GE05;
			else
				stage->glState = BLEND(S_ALPHA,M_S_ALPHA)|GLSTATE_ALPHA_GT0;
		}
	}
	else if (style & (SHADER_TRANS33|SHADER_TRANS66))
	{
		float	alpha;

		stage->glState = BLEND(S_ALPHA,M_S_ALPHA)|GLSTATE_DEPTHWRITE;
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

	if (style & SHADER_TURB)
	{
		tcModParms_t	*tcmod;
#if 0
		// scale
		tcmod = NewTcModStage (stage);
		tcmod->type       = TCMOD_SCALE;
		tcmod->sScale     = tcmod->tScale = 1.0f / 64;
		// turb
		tcmod = NewTcModStage (stage);
		tcmod->type       = TCMOD_TURB;
		tcmod->wave.amp   = 1.0f / 16;
		tcmod->wave.phase = 0;
		tcmod->wave.freq  = 1.0f / (2.0f * M_PI);
#else
		tcmod = NewTcModStage (stage);
		tcmod->type = TCMOD_WARP;
#endif

#if 0
		{
			// deform verts
			deformParms_t *deform = &sh.deforms[0];
			deform->type       = DEFORM_WAVE;
			deform->wave.type  = FUNC_SIN;
			deform->wave.amp   = 2;
			deform->wave.freq  = 0.3;
			deform->wave.base  = 0;
			deform->wave.phase = 0.5;
			deform->waveDiv    = 1.0/(64*16);
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
		stage->glState = BLEND(S_ALPHA,1);
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

	return FinishShader ();

	unguardf(("%s", name));
}


/*-----------------------------------------------------------------------------
	Quake shader script support
-----------------------------------------------------------------------------*/

class shaderScript_t : public CStringItem
{
public:
	TString<64> FileName;
	bool	isTemplate;
	unsigned start;
	unsigned end;
};

static CMemoryChain   *scriptChain;
static TList<shaderScript_t> scriptList;


static void FindShaderScripts ()
{
	guard(FindShaderScripts);

	FreeShaderScripts ();
	scriptChain = new CMemoryChain;
	scriptList.Reset ();
	fsModifyCount = GFileSystem->modifyCount;

	int numFiles = 0, numScripts = 0;

	appPrintf ("Searching for shader scripts:\n");

	CFileList *scriptFiles = GFileSystem->List (va("scripts/*.shader"), FS_FILE);	//?? FS_PATH_NAMES
	for (TListIterator<CFileItem> file = *scriptFiles; file; ++file)
	{
		numFiles++;

		char *buf = (char*)GFileSystem->LoadFile (va("scripts/%s", file->name));	//?? FS_PATH_NAMES
		if (!buf) continue;			// should not happens

#if DEBUG_SHADERS
		if (gl_debugShaders->integer) appPrintf (S_GREEN"%s\n", file->name);
#endif
		CSimpleParser text;
		text.InitFromBuf (buf, PARSER_CPP_COMMENTS|PARSER_SEPARATE_BRACES);
		const char *errMsg = NULL;
		while (const char *line = text.GetLine ())
		{
			if (line[0] == '}' || line[0] == '{')
			{
				errMsg = va("unexpected \"%s\"", line);
				break;
			}
#if DEBUG_SHADERS
			if (gl_debugShaders->integer) appPrintf (S_RED"%s\n", line);
#endif
			TString<64> Name;
			Name.filename (line);
			line = text.GetLine ();
			if (line[0] != '{' || line[1] != 0)
			{
				errMsg = va("%s: \"{\" expected", *Name);
				break;
			}
			// remember start
			int start = text.GetPos () - buf;
			// skip shader body
			const char *endPos  = text.SkipBraces ();
			if (!endPos)
			{
				errMsg = "unexpected end of file";
				break;
			}
			int end = endPos - buf;

			// remember script info
			shaderScript_t *scr = scriptList.Find (Name);
			if (!scr)
			{
				// not found - create new; else - override previous script
				scr = new (Name, scriptChain) shaderScript_t;
				scriptList.Insert (scr);
			}
			scr->FileName.filename (file->name);
			scr->start      = start;
			scr->end        = end;
			scr->isTemplate = appIsWildcard (scr->name);

			numScripts++;
		}
#if DEBUG_SHADERS
		if (gl_debugShaders->integer && errMsg) appWPrintf ("ERROR in scripts/%s: %s\n", file->name, errMsg);	//?? FS_PATH_NAMES
#endif

		delete buf;
	}
	delete scriptFiles;

	appPrintf ("...found %d shader scripts in %d files\n", numScripts, numFiles);
	unguard;
}


static void FreeShaderScripts ()
{
	if (scriptChain) delete scriptChain;
	scriptChain = NULL;
}


static const char *shaderError;	// for script commands
#define ERROR_IN_SHADER(msg)	{ shaderError = msg; return; }
#include "gl_shadersyntax.h"


// will get params from "sh"
// if "text" is not NULL, shader will be created from it, otherwise, will
// be searched shader body for srcName
//?? add shader command, which will allow to init from different script
static bool InitShaderFromScript (const char *srcName, const char *text)
{
	guard(InitShaderFromScript);

	char *buf = NULL;

	if (!text)
	{
		// 1st: check .qs file  2nd: check *.shader files

		// try loading name.qs
		buf = (char*)GFileSystem->LoadFile (va("%s.qs", srcName));
		if (buf)
			text = buf;
		else
		{
			// find shader or template
			// 1st: shader
			shaderScript_t *scr = scriptList.Find (srcName);
			if (!scr)	// 2nd: template
				for (TListIterator<shaderScript_t> script = scriptList; script; ++script)
					if (script->isTemplate && appMatchWildcard (srcName, script->name, false))
					{
						scr = *script;
						break;
					}
			if (!scr) return false;

#if DEBUG_SHADERS
			if (gl_debugShaders->integer) appWPrintf ("found %s in %s %X-%X\n", scr->name, *scr->FileName, scr->start, scr->end);
#endif
			// load script file
			unsigned length;
			buf = (char*)GFileSystem->LoadFile (va("scripts/%s", *scr->FileName), &length);
			if (scr->end > length)
			{
				appWPrintf ("script \"%s\" beyond end of file \"%s\" ?", srcName, *scr->FileName);
				return false;
			}
			text = buf + scr->start;
			buf[scr->end] = 0;
		}
	}

#if DEBUG_SHADERS
	if (gl_debugShaders->integer) appPrintf (S_GREEN"%s:\n-----\n%s\n-----\n", srcName, text);
#endif

	sh.scripted = true;

	// parse script
	CSimpleParser parser;
	parser.InitFromBuf (text, PARSER_CPP_COMMENTS|PARSER_SEPARATE_BRACES);

	shaderError = NULL;
	while (const char *line = parser.GetLine ())
	{
		if (line[0] == '{')
		{
			// parse stage keywords
			if (line[1] != 0)
			{
				shaderError = "extra chars after '{'";
				break;
			}
			while (true)
			{
				line = parser.GetLine ();
				if (!line)
				{
					shaderError = "unexpected end of script";
					break;
				}
				if (line[0] == '}')
				{
					if (line[1] != 0)
					{
						shaderError = "extra chars after '}'";
						break;
					}
					break;
				}
				if (!ExecuteCommand (line, ARRAY_ARG(stageFuncs)))
				{
					shaderError = va("unknown stage command: \"%s\"", line);
					break;
				}
			}
			sh.numStages++;
		}
		else
		{
			if (!ExecuteCommand (line, ARRAY_ARG(shaderFuncs)))
			{
				if (strnicmp (line, "q3map_", 6) && strnicmp (line, "qer", 3))
					shaderError = va("unknown shader command: \"%s\"", line);
			}
		}
		if (shaderError) break;
	}
	bool result = true;
	if (shaderError)
	{
		sh.bad = true;
		appWPrintf ("ERROR in shader \"%s\": %s\n", srcName, shaderError);
		result = false;
	}
	else
	{
		// fill shader->width and height with params of the main image
		if (!sh.width)  sh.width  = 1;
		if (!sh.height) sh.height = 1;
	}

	if (buf) delete buf;
	return result;

	unguardf(("%s", srcName));
}


/*-----------------------------------------------------------------------------
	Miscellaneous
-----------------------------------------------------------------------------*/

shader_t *GetShaderByNum (int num)
{
	if (num >= 0 && num < shaderCount)
		return shadersArray[num];
	return gl_defaultShader;
}


void ResetShaders ()
{
	guard(ResetShaders);

	if (shaderChain) delete shaderChain;
	shaderChain = new CMemoryChain;

	memset (hashTable, 0, sizeof(hashTable));
	memset (shadersArray, 0, sizeof(shadersArray));
	shaderCount = 0;

	/*---------------- creating system shaders --------------------*/

	// abstract shaders should be created in reverse (with relation to sortParam) order
	gl_alphaShader2 = FindShader ("*alpha2", SHADER_ABSTRACT);
	gl_alphaShader2->sortParam = SORT_SEETHROUGH + 1;

	gl_entityShader = FindShader ("*entity", SHADER_ABSTRACT);
	gl_entityShader->sortParam = SORT_SEETHROUGH;

	gl_particleShader = FindShader ("*particle", SHADER_ABSTRACT);
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
	sh.Name = "*identitylight2";
	st[0].rgbGenType = RGBGEN_EXACT_VERTEX;
	gl_identityLightShader2 = FinishShader ();

	gl_flareShader = FindShader ("fx/flare", SHADER_WALL);
	if (gl_flareShader->bad) gl_flareShader = NULL;

	gl_skyShader = gl_defaultSkyShader = FindShader ("*sky", SHADER_SKY|SHADER_ABSTRACT);

	gl_detailShader = FindShader ("*detail", SHADER_ALPHA|SHADER_WALL);
	if (gl_detailShader)
	{
		gl_detailShader->cullMode = CULL_NONE;
		gl_detailShader->stages[0]->glState = BLEND(D_COLOR,S_COLOR)|GLSTATE_NODEPTHTEST;
	}

	gl_concharsShader = FindShader ("pics/conchars", SHADER_ALPHA);

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
		const image_t *img = stages[0]->mapImage[0];	//?? all images (non-system, not "*name" etc), all anims, all stages
		if (img)			// may be NULL when $white
			FindImage (img->Name, img->flags | IMAGE_RELOAD);
	}
}


} // namespace
