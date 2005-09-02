#include "OpenGLDrv.h"
#include "gl_model.h"
#include "gl_math.h"


namespace OpenGLDrv {


bool model_t::LerpTag (int frame1, int frame2, float lerp, const char *tagName, CCoords &tag) const
{
	Com_DropError ("model %s have no LerpTag caps", *Name);
	return false;
}


bool md3Model_t::LerpTag (int frame1, int frame2, float lerp, const char *tagName, CCoords &tag) const
{
	const char *name = tagNames;
	for (int i = 0; i < numTags; i++, name += MAX_QPATH)
		if (!strcmp (name, tagName))
			break;
	bool result = true;
	if (i == numTags)
	{
		//?? developer message
//		DrawTextLeft (va("no tag \"%s\" in %s", tagName, name), RGB(1,0,0));
		i = 0;			// use 1st tag
		result = false;
	}
	if (frame1 < 0 || frame1 > numFrames)
	{
		//?? message
		frame1 = 0;
	}
	if (frame2 < 0 || frame2 > numFrames)
	{
		//?? message
		frame2 = 0;
	}
	const CCoords &tag1 = tags[frame1 * numTags + i];
	const CCoords &tag2 = tags[frame2 * numTags + i];
	// fast non-lerp case
	if (frame1 == frame2 || lerp == 0)
		tag = tag1;
	else if (lerp == 1)
		tag = tag2;
	else
	{
		// interpolate tags
		//?? use quaternions
		for (i = 0; i < 3; i++)
		{
			// linear lerp axis vectors
			Lerp (tag1.axis[i], tag2.axis[i], lerp, tag.axis[i]);
			tag.axis[i].Normalize ();
		}
		// interpolate origin
		Lerp (tag1.origin, tag2.origin, lerp, tag.origin);
	}

	return result;
}


// Find xyz_index for each "st" (st have unique xyz, but xyz may have few st on texture seams)
// and fill "st" list.
// This function may be used for Q2 and KP models
static int ParseGlCmds (const char *name, surfaceMd3_t *surf, int *cmds, int *xyzIndexes)
{
	int		vertsIndexes[1024];		// verts per triangle strip/fan

	guard(ParseGlCmds);

	int numTris = 0;
	int allocatedVerts = 0;
	int *idx = surf->indexes;
	while (int count = *cmds++)
	{
		int		i;

		// cmds -> s1, t1, idx1, s2, t2, idx2 ...

		bool strip = count > 0;
		if (!strip)
			count = -count;
		numTris += count - 2;

		if (numTris > surf->numTris)
		{
			appWPrintf ("ParseGlCmds(%s): incorrect triangle count\n", name);
			return 0;
		}

		// generate vertexes and texcoords
		for (i = 0; i < count; i++, cmds += 3)
		{
			float s = ((float*)cmds)[0];
			float t = ((float*)cmds)[1];

			// find st in allocated vertexes
			int		index;
			float	*dst;
			for (index = 0, dst = surf->texCoords; index < allocatedVerts; index++, dst += 2)
				if (xyzIndexes[index] == cmds[2] && dst[0] == s && dst[1] == t) break;

			if (index == allocatedVerts)
			{	// vertex not found - allocate it
				if (allocatedVerts == surf->numVerts)
				{
					appWPrintf ("ParseGlCmds(%s): too much texcoords\n", name);
					return false;
				}
				dst[0] = s;
				dst[1] = t;
				xyzIndexes[allocatedVerts] = cmds[2];
				allocatedVerts++;
			}

			vertsIndexes[i] = index;
		}

		if (!idx)	// called to calculate numVerts - skip index generation
			continue;

		// generate indexes
		int i1 = vertsIndexes[0]; int i2 = vertsIndexes[1];
		for (i = 2; i < count; i++)
		{
			int i3 = vertsIndexes[i];
			// put indexes
			*idx++ = i1; *idx++ = i2; *idx++ = i3;
			// prepare next step
			if (strip)
			{
				if (!(i & 1))	i1 = i3;
				else			i2 = i3;
			}
			else				i2 = i3;
		}
	}

	surf->numTris = numTris;	// update triangle count
	return allocatedVerts;

	unguard;
}


static float ComputeMd3Radius (const CVec3 &localOrigin, const vertexMd3_t *verts, int numVerts)
{
	guard(ComputeMd3Radius);

	float radius2 = 0;
	for (int i = 0; i < numVerts; i++, verts++)
	{
		CVec3 p;
		p[0] = verts->xyz[0] * MD3_XYZ_SCALE;
		p[1] = verts->xyz[1] * MD3_XYZ_SCALE;
		p[2] = verts->xyz[2] * MD3_XYZ_SCALE;
		VectorSubtract (p, localOrigin, p);
		float tmp = dot(p, p);	// tmp = dist(p, localOrigin) ^2
		radius2 = max(tmp, radius2);
	}
	return sqrt (radius2);

	unguard;
}


static void ProcessMd2Frame (vertexMd3_t *verts, dMd2Frame_t *srcFrame, md3Frame_t *dstFrame, int numVerts, int *xyzIndexes)
{
	guard(ProcessMd2Frame);

	dstFrame->bounds.Clear ();
	int i;
	vertexMd3_t *dstVerts;
	for (i = 0, dstVerts = verts; i < numVerts; i++, dstVerts++)
	{
		dMd2Vert_t *srcVert = &srcFrame->verts[xyzIndexes[i]];
		// compute vertex
		CVec3 p;
		p[0] = srcFrame->scale[0] * srcVert->v[0] + srcFrame->translate[0];
		p[1] = srcFrame->scale[1] * srcVert->v[1] + srcFrame->translate[1];
		p[2] = srcFrame->scale[2] * srcVert->v[2] + srcFrame->translate[2];
		// update bounding box
		dstFrame->bounds.Expand (p);
		// put vertex in a "short" form
		dstVerts->xyz[0] = appRound (p[0] / MD3_XYZ_SCALE);
		dstVerts->xyz[1] = appRound (p[1] / MD3_XYZ_SCALE);
		dstVerts->xyz[2] = appRound (p[2] / MD3_XYZ_SCALE);
	}
	// compute bounding sphere center
	dstFrame->bounds.GetCenter (dstFrame->localOrigin);
	// and radius
	dstFrame->radius = ComputeMd3Radius (dstFrame->localOrigin, verts, numVerts);
	unguard;
}


static void BuildMd2Normals (surfaceMd3_t *surf, int *xyzIndexes, int numXyz)
{
	guard(BuildMd2Normals);

	int		i, j, *idx;
	CVec3	normals[MD3_MAX_VERTS];	// normal per xyzIndex
	short	norm_i[MD3_MAX_VERTS];
	vertexMd3_t *verts;

	for (i = 0, verts = surf->verts; i < surf->numFrames; i++, verts += surf->numVerts)
	{
		// clear normals array
		memset (normals, 0, sizeof(normals));

		for (j = 0, idx = surf->indexes; j < surf->numTris; j++, idx += 3)
		{
			CVec3 vecs[3], n;

			// compute triangle normal
			for (int k = 0; k < 3; k++)
			{
				VectorSubtract2 (verts[idx[k]].xyz, verts[idx[k == 2 ? 0 : k + 1]].xyz, vecs[k]);
				vecs[k].NormalizeFast ();
			}
			cross (vecs[1], vecs[0], n);
			n.NormalizeFast ();
			// add normal to verts
			for (k = 0; k < 3; k++)
			{
				float	ang;
#if 1
				ang = - dot (vecs[k], vecs[k == 0 ? 2 : k - 1]);
				ang = ACOS_FUNC(ang);
#else
				ang = acos (- dot (vecs[k], vecs[k == 0 ? 2 : k - 1]));
#endif
				VectorMA (normals[xyzIndexes[idx[k]]], ang, n);		// weighted normal: weight ~ angle
			}
		}
		// convert computed xyz normals to compact form
		for (j = 0; j < numXyz; j++)
		{
			byte	a, b;

			CVec3 &dst = normals[j];
#if 1
			dst.NormalizeFast ();
			a = appRound (ACOS_FUNC(dst[2]) / (M_PI * 2) * 255);
			if (dst[0])		b = appRound (ATAN2_FUNC (dst[1], dst[0]) / (M_PI * 2) * 255);
#else
			dst.Normalize ();
			a = appRound (acos (dst[2]) / (M_PI * 2) * 255);
			if (dst[0])		b = appRound (atan2 (dst[1], dst[0]) / (M_PI * 2) * 255);
#endif
			else			b = dst[1] > 0 ? 127 : -127;
			norm_i[j] = a | (b << 8);
		}
		// copy normals
		for (j = 0; j < surf->numVerts; j++)
			verts[j].normal = norm_i[xyzIndexes[j]];
	}

	unguard;
}


static void SetMd3Skin (const char *name, surfaceMd3_t *surf, int index, const char *skin)
{
	// try specified shader
	shader_t *shader = FindShader (skin, SHADER_CHECK|SHADER_SKIN);
	if (!shader)
	{
		// try to find skin forcing model directory
		TString<128> MName;			// model name; double size: will append skin name
		MName.filename (name);
		char *mPtr = MName.rchr ('/');
		if (mPtr)	mPtr++;			// skip '/'
		else		mPtr = MName;

		TString<64> SName;			// skin name
		SName.filename (skin);
		char *sPtr = SName.rchr ('/');
		if (sPtr)	sPtr++;			// skip '/'
		else		sPtr = SName;

		strcpy (mPtr, sPtr);		// make "modelpath/skinname"
		shader = FindShader (MName, SHADER_CHECK|SHADER_SKIN);
		if (!shader)
		{	// not found
			Com_DPrintf ("SetSkin(%s:%d): %s or %s is not found\n", name, index, skin, *MName);
			if (index > 0)
				shader = surf->shaders[0];		// better than default shader
			else
				shader = FindShader ("*identityLight", SHADER_SKIN); // white + diffuse lighting
		}
	}
	// set the shader
	surf->shaders[index] = shader;
}


#define MAX_XYZ_INDEXES		4096	// maximum number of verts in loaded md3 model

#if 0
#define BUILD_SCELETON
//???? rename, may be remove
static void CheckTrisSizes (surfaceMd3_t *surf, dMd2Frame_t *md2Frame = NULL, int md2FrameSize = 0)
{
	int		tri, *inds;
	bool	fixed[MAX_XYZ_INDEXES];

	//!! special cases:
	//!!	1. numFrames == 1 -- static model
	//!!	2. numFixed == numTris: model -> static + info about placement relative to model axis
	//!!		BUT: require to check presence of a SINGLE skeleton (model can have few parts, which are
	//!!		moved one along/around another)
	//!! TODO:
	//!!		- optimization: build list of adjancent tris, list of same verts (doubled on skin seams)
	//!!		- may be, check not tris -- check edges -- for building skeleton

	int numFixed = surf->numTris;
	memset (&fixed, 1, sizeof(fixed));	// fill by "true" ?????

	// compute max md2 frame scale (int * scale = float, so "scale" can be used as possible error)
	CVec3 scale;
	if (md2Frame)
	{
		scale.Zero();
		for (int frame = 0; frame < surf->numFrames; frame++, md2Frame = OffsetPointer (md2Frame, md2FrameSize))
		{
			if (md2Frame->scale[0] > scale[0]) scale[0] = md2Frame->scale[0];
			if (md2Frame->scale[1] > scale[1]) scale[1] = md2Frame->scale[1];
			if (md2Frame->scale[2] > scale[2]) scale[2] = md2Frame->scale[2];
		}
	}
	else
	{
		//?? for real md3 model can pass NULL instead of md2Frame and use constant scale[]
		scale[0] = scale[1] = scale[2] = 1.0f / MD3_XYZ_SCALE;
	}

	for (tri = 0, inds = surf->indexes; tri < surf->numTris; tri++, inds += 3)
	{
		int		i, j;

		// integer bounds
		int	deltaMin[3], deltaMax[3];
		// set deltaMin/deltaMax[i] bounds to a maximum diapason
		for (i = 0; i < 3; i++)
		{
			deltaMin[i] = -BIG_NUMBER;
			deltaMax[i] = BIG_NUMBER;
		}
//		float maxError2 = 0;
		float errorConst = dot (scale, scale);

		vertexMd3_t *verts = surf->verts;
		for (int frame = 0; frame < surf->numFrames; frame++, verts += surf->numVerts)
		{
			CVec3 d;

			// compute distance between 2 verts from triangle separately by each coord
			for (i = 0; i < 3; i++)
			{
				int k = (i == 2) ? 0 : i + 1;
				for (j = 0; j < 3; j++)
					d[j] = abs(verts[inds[i]].xyz[j] - verts[inds[k]].xyz[j]) * MD3_XYZ_SCALE;
				// squared distance
				float dist2 = dot(d, d);
				// quant-based error
				float error2 = (2 * dot(d, scale) + errorConst);	//!!!
				// distance-based error
				float dist = SQRTFAST(dist2);
				float err = dist * Cvar_VariableValue("err") / 100;	//!! percent from distance; make const
				// shrink deltaMin[i]/deltaMax[i] bounds to dist2 +/- error2
				float min = dist2 - error2;
				float min2 = (dist - err); min2 *= min2;
				if (min2 < min) min = min2;			// use maximum range: [min..max] or [min2..max2]
				if (deltaMin[i] < min)				// shrink deltaMin
					deltaMin[i] = min;
				float max = dist2 + error2;
				float max2 = (dist + err); max2 *= max2;
				if (max2 > max) max = max2;			// use maximum range
				if (deltaMax[i] > max)				// shrink deltaMax
					deltaMax[i] = max;
				if (deltaMin[i] > deltaMax[i])		// empty range
				{
					fixed[tri] = false;
					numFixed--;
					break;
				}
			}

			if (!fixed[tri]) break;
		}

		//?? can make indexes[tri] = 0 (exclude tri from drawing)
		if (!fixed[tri])
			inds[0] = inds[1] = inds[2] = 0;

		//???
//		appPrintf ("%s%3d\n", fixed[tri] ? S_GREEN : "", tri);
	}
	appPrintf (S_CYAN"FIXED: %d / %d\n", numFixed, surf->numTris);	//??
}

#endif


md3Model_t *LoadMd2 (const char *name, byte *buf, unsigned len)
{
	guard(LoadMd2);

	md3Model_t *md3;
	surfaceMd3_t *surf;
	int		i, numVerts, numTris;
	int		xyzIndexes[MAX_XYZ_INDEXES];

	//?? should add ri.LoadMd2 (buf, size) -- sanity check + swap bytes for big-endian machines
	dMd2_t *hdr = (dMd2_t*)buf;
	if (hdr->version != MD2_VERSION)
	{
		appWPrintf ("LoadMd2(%s): wrong version %d\n", name, hdr->version);
		return NULL;
	}

	if (hdr->numXyz <= 0 || hdr->numTris <= 0 || hdr->numFrames <= 0)
	{
		appWPrintf ("LoadMd2(%s): incorrect params\n", name);
		return NULL;
	}

	/* We should determine number of vertexes for conversion of md2 model into md3 format, because
	 * in md2 one vertex may have few texcoords (for skin seams); ParseGlCmds() will detect seams and
	 * duplicate vertex
	 */
	surf = (surfaceMd3_t*)appMalloc (sizeof(surfaceMd3_t) + MAX_XYZ_INDEXES * 2*sizeof(int));	// alloc space for MAX_XYZ_INDEXES texcoords
	surf->texCoords = (float*)(surf+1);
	surf->numVerts  = MAX_XYZ_INDEXES;
	surf->numTris   = MAX_XYZ_INDEXES - 2;
	numVerts = ParseGlCmds (name, surf, (int*)(buf + hdr->ofsGlcmds), xyzIndexes);
	numTris = surf->numTris;		// just computed
	if (numTris != hdr->numTris) appWPrintf ("LoadMd2(%s): computed numTris %d != %d\n", name, numTris, hdr->numTris);
	appFree (surf);
//	Com_DPrintf ("MD2(%s): xyz=%d st=%d verts=%d tris=%d frms=%d\n", name, hdr->numXyz, hdr->numSt, numVerts, numTris, hdr->numFrames);

	/* Allocate memory:
		md3Model_t		[1]
		md3Frame_t		[numFrames]
		surfaceMd3_t	[numSurfaces == 1]
		float			texCoords[2*numVerts]
		int				indexes[3*numTris]
		vertexMd3_t		verts[numVerts][numFrames]
		shader_t*		shaders[numShaders == numSkins]
	 */
	int size = sizeof(md3Model_t) + hdr->numFrames*sizeof(md3Frame_t)
		+ sizeof(surfaceMd3_t)
		+ numVerts*2*sizeof(float) + 3*hdr->numTris*sizeof(int)
		+ numVerts*hdr->numFrames*sizeof(vertexMd3_t) + hdr->numSkins*sizeof(shader_t*);
	md3 = (md3Model_t*) appMalloc (size);
	CALL_CONSTRUCTOR(md3);
	md3->Name = name;
	md3->size = size;

	/*-------- fill md3 structure --------*/
	md3->numSurfaces = 1;
	md3->numFrames = hdr->numFrames;
	md3->frames = (md3Frame_t*)(md3 + 1);

	md3->surf = surf = (surfaceMd3_t*)(md3->frames + md3->numFrames);
	CALL_CONSTRUCTOR(surf);

	/*-------- fill surf structure -------*/
	surf->shader    = gl_defaultShader;		// any, ignored
	surf->Name      = "single_surf";		// any name
	// counts
	surf->numFrames = hdr->numFrames;
	surf->numVerts  = numVerts;
	surf->numTris   = numTris;
	surf->numShaders = hdr->numSkins;
	// pointers
	surf->texCoords = (float*)(surf + 1);
	surf->indexes   = (int*)(surf->texCoords + 2*surf->numVerts);
	surf->verts     = (vertexMd3_t*)(surf->indexes + 3*surf->numTris);
	surf->shaders   = (shader_t**)(surf->verts + surf->numVerts*surf->numFrames);

START_PROFILE(..Md2::Parse)
	/*--- build texcoords and indexes ----*/
	if (!ParseGlCmds (name, surf, (int*)(buf + hdr->ofsGlcmds), xyzIndexes))
	{
		appFree (md3);
		return NULL;
	}
END_PROFILE

START_PROFILE(..Md2::Frame)
	/*---- generate vertexes/normals -----*/
	for (i = 0; i < surf->numFrames; i++)
		ProcessMd2Frame (surf->verts + i * numVerts,
				(dMd2Frame_t*)(buf + hdr->ofsFrames + i * hdr->frameSize),
				md3->frames + i, numVerts, xyzIndexes);
END_PROFILE
START_PROFILE(..Md2::Normals)
	BuildMd2Normals (surf, xyzIndexes, hdr->numXyz);
END_PROFILE

#ifdef BUILD_SCELETON	//????
	CheckTrisSizes (surf, (dMd2Frame_t*)(buf + hdr->ofsFrames), hdr->frameSize);
#endif

START_PROFILE(..Md2::Skin)
	/*---------- load skins --------------*/
	surf->numShaders = hdr->numSkins;
	const char *skin;
	for (i = 0, skin = (char*)(buf + hdr->ofsSkins); i < surf->numShaders; i++, skin += MD2_MAX_SKINNAME)
		SetMd3Skin (name, surf, i, skin);
END_PROFILE

	return md3;

	unguard;
}


md3Model_t *LoadMd3 (const char *name, byte *buf, unsigned len)
{
	guard(LoadMd3);

	int		i;
	dMd3Surface_t *ds;
	surfaceMd3_t *surf;

	//?? load LOD
	//?? should add ri.LoadMd3 (buf, size) -- sanity check + swap bytes for big-endian machines
	dMd3_t *hdr = (dMd3_t*)buf;
	if (hdr->version != MD3_VERSION)
	{
		appWPrintf ("LoadMd3(%s): wrong version %d\n", name, hdr->version);
		return NULL;
	}

	if (hdr->numSurfaces < 0 || hdr->numTags < 0 || hdr->numFrames <= 0)
	{
		appWPrintf ("LoadMd3(%s): incorrect params\n", name);
		return NULL;
	}

	// all-surface counts (total sums)
	int tNumVerts = 0;
	int tNumTris = 0;
	int tNumShaders = 0;
	ds = (dMd3Surface_t*)(buf + hdr->ofsSurfaces);
	for (i = 0; i < hdr->numSurfaces; i++)
	{
		// check: model.numFrames should be == all surfs.numFrames
		if (ds->numFrames != hdr->numFrames)
		{
			appWPrintf ("LoadMd3(%s): different frame counts in surfaces\n", name);
			return NULL;
		}
		// counts
		tNumVerts += ds->numVerts;
		tNumTris += ds->numTriangles;
		tNumShaders += ds->numShaders;
		// next surface
		ds = OffsetPointer (ds, ds->ofsEnd);
	}
//	Com_DPrintf ("MD3(%s): verts=%d tris=%d frms=%d surfs=%d tags=%d\n", name, tNumVerts, tNumTris, hdr->numFrames, hdr->numSurfaces, hdr->numTags);

	/* Allocate memory:
		md3Model_t		[1]
		md3Frame_t		[numFrames]
		char			[MAX_QPATH][numTags]   -- tag names
		CCoords			[numFrames][numTags]
		surfaceMd3_t	[numSurfaces]
		-- surfaces data [numSurfaces] --
		float			texCoords[2*s.numVerts]
		int				indexes[3*s.numTriangles]
		vertexMd3_t		verts[s.numVerts][numFrames]
		shader_t*		shaders[s.numShaders]
	 */
	int size = sizeof(md3Model_t) + hdr->numFrames*sizeof(md3Frame_t)
		+ hdr->numTags*MAX_QPATH + hdr->numTags*hdr->numFrames*sizeof(CCoords)
		+ hdr->numSurfaces*sizeof(surfaceMd3_t);
	int surfDataOffs = size;			// start of data for surfaces
	size += tNumVerts*2*sizeof(float) + tNumTris*3*sizeof(int) + tNumVerts*hdr->numFrames*sizeof(vertexMd3_t) + tNumShaders*sizeof(shader_t*);

	md3Model_t *md3 = (md3Model_t*) appMalloc (size);
	CALL_CONSTRUCTOR(md3);
	md3->Name = name;
	md3->size = size;

	/*-------- fill md3 structure --------*/
	// frames
	md3->numFrames = hdr->numFrames;
	md3->frames = (md3Frame_t*)(md3 + 1);
	dMd3Frame_t *fs = (dMd3Frame_t*)(buf + hdr->ofsFrames);
	for (i = 0; i < hdr->numFrames; i++, fs++)
	{
		// NOTE: md3 models, created with id model converter, have frame.localOrigin == (0,0,0)
		// So, we should recompute localOrigin and radius for more effective culling
		md3Frame_t *frm = &md3->frames[i];
		frm->bounds = fs->bounds;
		// compute localOrigin
		frm->bounds.GetCenter (frm->localOrigin);
		frm->radius = 0;		// will compute later
	}
	// tags
	md3->numTags = hdr->numTags;
	md3->tagNames = (char*)(md3->frames + md3->numFrames);
	md3->tags = (CCoords*)(md3->tagNames + hdr->numTags*MAX_QPATH);
	char *tagName = md3->tagNames;
	dMd3Tag_t *ts = (dMd3Tag_t*)(buf + hdr->ofsTags);	// tag source
	for (i = 0; i < hdr->numTags; i++, tagName += MAX_QPATH, ts++)
	{
//		appPrintf("model %s tag %d %s\n", name, i, ts->name);
		strcpy (tagName, ts->name);
	}
	ts = (dMd3Tag_t*)(buf + hdr->ofsTags);
	CCoords *td = md3->tags;							// tag destination
	for (i = 0; i < hdr->numFrames; i++)
		for (int j = 0; j < hdr->numTags; j++, ts++, td++)
		{
			if (strcmp (ts->name, md3->tagNames + j * MAX_QPATH))
			{
				appWPrintf ("LoadMd3(%s): volatile tag names\n", name);
				return NULL;
			}
			*td = ts->tag;
			td->axis[0].Normalize ();
			td->axis[1].Normalize ();
			td->axis[2].Normalize ();
		}
	// surfaces
	md3->numSurfaces = hdr->numSurfaces;
	md3->surf = (surfaceMd3_t*)(md3->tags + md3->numTags*hdr->numFrames);
	ds = (dMd3Surface_t*)(buf + hdr->ofsSurfaces);
	byte *surfData = (byte*)md3 + surfDataOffs;
	for (i = 0, surf = md3->surf; i < hdr->numSurfaces; i++, surf++)
	{
		CALL_CONSTRUCTOR(surf);
		surf->shader    = gl_defaultShader;	// any, ignored
		surf->Name.toLower (ds->name);
//		appPrintf("model %s surf %d %s\n", name, i, *surf->Name);
		// counts
		surf->numFrames = hdr->numFrames;
		surf->numVerts  = ds->numVerts;
		surf->numTris   = ds->numTriangles;
		surf->numShaders = ds->numShaders;
		// pointers
		surf->texCoords = (float*)surfData;
		surf->indexes   = (int*)(surf->texCoords + 2*surf->numVerts);
		surf->verts     = (vertexMd3_t*)(surf->indexes + 3*surf->numTris);
		surf->shaders   = (shader_t**)(surf->verts + surf->numVerts*surf->numFrames);
		surfData = (byte*)(surf->shaders + surf->numShaders);
		// triangles: same layout on disk and in memory
		memcpy (surf->indexes, (byte*)ds + ds->ofsTriangles, ds->numTriangles * sizeof(dMd3Triangle_t));
		// texcoords: same layout on disk and in memory
		memcpy (surf->texCoords, (byte*)ds + ds->ofsSt, ds->numVerts * sizeof(dMd3St_t));
		// verts: same layout on disk and in memory
		memcpy (surf->verts, (byte*)ds + ds->ofsXyzNormals, ds->numVerts * ds->numFrames * sizeof(dMd3XyzNormal_t));
		// shaders
		dMd3Shader_t *ss = (dMd3Shader_t*)((byte*)ds + ds->ofsShaders);
		for (int j = 0; j < ds->numShaders; j++, ss++)
			SetMd3Skin (name, surf, j, ss->name);
		// next surface
		ds = OffsetPointer (ds, ds->ofsEnd);
	}
	// compute frame radiuses
	md3Frame_t *frm = md3->frames;
	for (i = 0; i < md3->numFrames; i++, frm++) // iterate frames
	{
		float radius = 0;
		surf = md3->surf;
		for (int j = 0; j < md3->numSurfaces; j++, surf++) // iterate surfaces
		{
			// find surface vertexes for current frame
			vertexMd3_t *verts = surf->verts + surf->numVerts * i;
			float radius2 = ComputeMd3Radius (frm->localOrigin, verts, surf->numVerts);
			if (radius2 > radius) radius = radius2;
		}
		frm->radius = radius;
	}
	return md3;

	unguard;
}


/*-------------- Sprite models  ----------------*/


static void SetSprRadius (sprModel_t *spr)
{
	for (int i = 0; i < spr->numFrames; i++)
	{
		sprFrame_t &frm = spr->frames[i];
		float s = max (frm.localOrigin[0], frm.width - frm.localOrigin[0]);
		float t = max (frm.localOrigin[1], frm.height - frm.localOrigin[1]);
		float radius = sqrt (s * s + t * t);
		spr->radius = max (spr->radius, radius);
	}
}


sprModel_t *LoadSp2 (const char *name, byte *buf, unsigned len)
{
	dSp2_t *hdr = (dSp2_t*)buf;
	if (hdr->version != SP2_VERSION)
	{
		appWPrintf ("LoadSp2(%s): wrong version %d\n", name, hdr->version);
		return NULL;
	}
	if (hdr->numframes < 0)
	{
		appWPrintf ("LoadSp2(%s): incorrect frame count %d\n", name, hdr->numframes);
		return NULL;
	}

	int size = sizeof(sprModel_t) + (hdr->numframes-1) * sizeof(sprFrame_t);
	sprModel_t *spr = (sprModel_t*)appMalloc (size);
	CALL_CONSTRUCTOR(spr);
	spr->Name = name;
	spr->size = size;

	spr->numFrames = hdr->numframes;
	spr->radius = 0;
	// parse frames
	dSp2Frame_t *in = hdr->frames;
	sprFrame_t *out = spr->frames;
	for (int i = 0; i < hdr->numframes; i++, in++, out++)
	{
		out->width          = in->width;
		out->height         = in->height;
		out->localOrigin[0] = in->origin_x;
		out->localOrigin[1] = in->origin_y;

		out->shader = FindShader (in->name, SHADER_CHECK|SHADER_WALL|SHADER_FORCEALPHA);
		if (!out->shader)
		{
			Com_DPrintf ("LoadSp2(%s): %s is not found\n", name, in->name);
			out->shader = gl_defaultShader;
		}
	}
	SetSprRadius (spr);

	return spr;
}


static sprModel_t *loadSpr;

static void cSprShader (int argc, char **argv)
{
	shader_t *shader = loadSpr->frames[0].shader = FindShader (argv[1], SHADER_CHECK|SHADER_WALL|SHADER_FORCEALPHA);
	if (!shader)
	{
		Com_DPrintf ("LoadSpr(%s): %s is not found\n", *loadSpr->Name, argv[1]);
		loadSpr->frames[0].shader = gl_defaultShader;
	}
}

static void cSize (int argc, char **argv)
{
	int w = atoi (argv[1]);
	int h = atoi (argv[2]);
	loadSpr->frames[0].width          = w;
	loadSpr->frames[0].height         = h;
	loadSpr->frames[0].localOrigin[0] = w / 2;
	loadSpr->frames[0].localOrigin[1] = h / 2;
}


static const CSimpleCommand sprCommands[] = {
	{"shader",	cSprShader	},
	{"size",	cSize		}
};


sprModel_t *LoadSpr (const char *name, byte *buf, unsigned len)
{
	int size = sizeof(sprModel_t);
	sprModel_t *spr = (sprModel_t*)appMalloc (size);
	CALL_CONSTRUCTOR(spr);
	spr->Name = name;
	spr->size = size;

	spr->numFrames = 1;

	loadSpr = spr;
	CSimpleParser text;
	text.InitFromBuf ((char*)buf);
	while (const char *line = text.GetLine ())
		if (!ExecuteCommand (line, ARRAY_ARG(sprCommands)))
			appWPrintf ("%s: invalid line [%s]\n", name, line);

	shader_t *shader = spr->frames[0].shader;
	if (!shader)
	{
		appWPrintf ("LoadSpr(%s): no shader specified\n", name);
		appFree (spr);
		return NULL;
	}

	if (!spr->frames[0].width && !spr->frames[0].height)
	{
		//?? allow overriding from script
		spr->frames[0].width = shader->width;
		spr->frames[0].height = shader->height;
		spr->frames[0].localOrigin[0] = shader->width / 2;
		spr->frames[0].localOrigin[1] = shader->height / 2;
	}

	SetSprRadius (spr);

	return spr;
}


} // namespace
