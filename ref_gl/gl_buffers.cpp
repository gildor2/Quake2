#include "OpenGLDrv.h"
#include "gl_shader.h"
#include "gl_model.h"
#include "gl_backend.h"

namespace OpenGLDrv {


/*-----------------------------------------------------------------------------
	Dynamic surface buffer
-----------------------------------------------------------------------------*/

#define MAX_DYNAMIC_BUFFER		(384 * 1024)
static byte	*dynamicBuffer;		// [MAX_DYNAMIC_BUFFER]
static int	dynamicBufferSize;

static void *lastDynamicPtr;
static int	lastDynamicSize;


void *AllocDynamicMemory (int size)
{
	if (dynamicBufferSize + size > MAX_DYNAMIC_BUFFER)
	{
		//?? make this message developer-only
		DrawTextLeft (va("R_AllocDynamicMemory(%d) failed\n", size), RGB(1, 0, 0));
//		Com_FatalError ("R_AllocDynamicMemory(%d) failed\n", size);
		lastDynamicPtr = NULL;
		return NULL;
	}

	void *ptr = &dynamicBuffer[dynamicBufferSize];
	lastDynamicSize = dynamicBufferSize;
	lastDynamicPtr = ptr;
	dynamicBufferSize += size;
	memset (ptr, 0, size);

	return ptr;
}


// This function should be called only for REDUCING size of allocated block
void ResizeDynamicMemory (void *ptr, int newSize)
{
	int		n;

	if (!ptr || ptr != lastDynamicPtr)
		Com_FatalError ("R_ResizeDynamicMemory: bad pointer");
	n = lastDynamicSize + newSize;
	if (n > MAX_DYNAMIC_BUFFER)
		Com_FatalError ("R_ResizeDynamicMemory: out of memory in %d bytes", n - MAX_DYNAMIC_BUFFER);
	dynamicBufferSize = n;
}


/*-----------------------------------------------------------------------------
	Surface arrays
-----------------------------------------------------------------------------*/


static surfaceInfo_t *surfaceBuffer;	// [MAX_SCENE_SURFACES]
static int numSurfacesTotal;


// Add surface to a current scene (to a "vp" structure)
void AddSurfaceToPortal (surfaceBase_t *surf, shader_t *shader, int entityNum, int numDlights)
{
	surfaceInfo_t *si;

//	LOG_STRING(va("add surf %s ent=%X n_dl=%d\n", shader->name, entityNum, numDlights));
	if (numSurfacesTotal >= MAX_SCENE_SURFACES - 1) return;			// buffer is full
	if (numDlights > DLIGHTNUM_MASK) numDlights = DLIGHTNUM_MASK;
	si = vp.surfaces + vp.numSurfaces++;
	si->sort = (shader->sortIndex << SHADERNUM_SHIFT) | (entityNum << ENTITYNUM_SHIFT) | (numDlights << DLIGHTNUM_SHIFT);
	si->surf = surf;
	numSurfacesTotal++;
	// update maxUsedShaderIndex
	if (shader->sortIndex > gl_state.maxUsedShaderIndex)
		gl_state.maxUsedShaderIndex = shader->sortIndex;
}


void InsertShaderIndex (int index)
{
	int		i;
	surfaceInfo_t *si;

	int n = 0;
	for (i = 0, si = surfaceBuffer; i < numSurfacesTotal; i++, si++)
	{
		int s = (si->sort >> SHADERNUM_SHIFT) & SHADERNUM_MASK;
		if (s >= index)	// shader index incremented
		{
			si->sort += (1 << SHADERNUM_SHIFT);		// increment shader index field
			n++;
		}
	}
//	Com_DPrintf ("R_InsertShaderIndex(%d): changed %d indexes\n", index, n);
}


/*-----------------------------------------------------------------------------
	Work with "vp"
-----------------------------------------------------------------------------*/

void ClearPortal (void)
{
	vp.surfaces = &surfaceBuffer[numSurfacesTotal];
	vp.numSurfaces = 0;
}


/*-----------------------------------------------------------------------------
	Radix sort surfaces
-----------------------------------------------------------------------------*/

#define SORT_BITS	8	// 11
#define SORT_SIZE	(1 << SORT_BITS)
#define SORT_MASK	(SORT_SIZE - 1)

void SortSurfaces (viewPortal_t *port, surfaceInfo_t **destination)
{
	int		i, k;
	surfaceInfo_t *s, **src1, **dst1;
	surfaceInfo_t **src, **dst;
	int		alpha1, alpha2;		// indexes of "*alpha1" and "*alpha2" shaders

	surfaceInfo_t	*sortFirst[SORT_SIZE], *sortFirst2[SORT_SIZE],
					// we will use sortFirst and sortFirst2 by turns
					*sortLast[SORT_SIZE];
					// save pointer to a last chain element (fast insertion to the end)

	if (!gl_sortAlpha->integer)
	{
		alpha1 = gl_alphaShader1->sortIndex << SHADERNUM_SHIFT;
		alpha2 = gl_alphaShader2->sortIndex << SHADERNUM_SHIFT;
	}
	else
	{	// debug mode (make alpha1 > alpha2)
		alpha1 = 1;
		alpha2 = 0;
	}

#define GET_SORT(value,result)	\
	{							\
		int		tmp;			\
		tmp = value & (SHADERNUM_MASK << SHADERNUM_SHIFT);	\
		if (tmp > alpha1 && tmp < alpha2)	\
			result = value & ~(SHADERNUM_MASK << SHADERNUM_SHIFT | ENTITYNUM_MASK << ENTITYNUM_SHIFT) | alpha2;	\
		else					\
			result = value;		\
	}

	/* Sort by least significant SORT_BITS. Get source directly from
	 * surfaces array and put it to sortFirst
	 */
	memset (sortFirst, 0, sizeof(sortFirst));
	for (i = 0, s = port->surfaces; i < port->numSurfaces; i++, s++)
	{
		int		b;

		GET_SORT(s->sort,b);
		b &= SORT_MASK;
		if (!sortFirst[b])
			sortFirst[b] = s;					// insert it first
		else
			sortLast[b]->sortNext = s;			// insert to the chain end
		s->sortNext = NULL;
		sortLast[b] = s;
	}

	src1 = &sortFirst[0];
	dst1 = &sortFirst2[0];
	/* Sort other bits. Use sortFirst, sortFirst2, sortFirst ... as source.
	 * This is a modified variant of the previous loop.
	 */
	for (k = SORT_BITS; k < 32; k += SORT_BITS)	// shift index
	{
		src = src1;
		dst = dst1;
		src1 = dst1; dst1 = src;				// swap src1 and dst1 (for next loop)

		memset (dst, 0, sizeof(sortFirst));		// clear dst heads
		for (i = 0; i < SORT_SIZE; i++)
		{
			surfaceInfo_t *next;

			for (s = *src++; s; s = next)		// next chain
			{
				int		b;

				next = s->sortNext;				// s->sortNext will be set to NULL below
				GET_SORT(s->sort,b);
				b = (b >> k) & SORT_MASK;
				if (!dst[b])
					dst[b] = s;					// 1st element in chain
				else
					sortLast[b]->sortNext = s;	// add to the chain end
				s->sortNext = NULL;				// no next element
				sortLast[b] = s;
			}
		}
	}
	/*------ fill sortedSurfaces array --------*/
	src = src1;
	dst = destination;
	for (i = 0; i < SORT_SIZE; i++)
	{
		for (s = *src1++; s; s = s->sortNext)
			*dst++ = s;
	}
#undef GET_SORT
}


/*-----------------------------------------*/

// prepare buffers for new scene
void ClearBuffers (void)
{
	numSurfacesTotal = 0;
	gl_numEntities = 0;
	gl_numDlights = 0;
	dynamicBufferSize = 0;
}


/*-----------------------------------------------------------------------------
	Initialization/finalization
-----------------------------------------------------------------------------*/

void CreateBuffers (void)
{
	dynamicBuffer = new byte [MAX_DYNAMIC_BUFFER];
	surfaceBuffer = new surfaceInfo_t [MAX_SCENE_SURFACES];
}

void FreeBuffers (void)
{
	if (!dynamicBuffer) return;
	delete dynamicBuffer;
	dynamicBuffer = NULL;

	delete surfaceBuffer;
}


} // namespace
