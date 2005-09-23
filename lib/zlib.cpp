/*=============================================================================

	ZLib support functions

=============================================================================*/

//?? zlib will be linked to Core or other place ??
#define CORE_API

#include "../Core/Inc/Core.h"


// using Core memory manager

extern "C" void *zcalloc (int opaque, int items, int size)
{
	MEM_ALLOCATOR(opaque);
	return appMalloc (items * size);
}

extern "C" void zcfree (int opaque, void *ptr)
{
	appFree (ptr);
}
