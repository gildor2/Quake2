/*=============================================================================
	ZLib support functions
=============================================================================*/

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
