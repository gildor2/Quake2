
#if MEM_DEBUG
/* NOTES:
 *	1.	some compilers (not VC) may have problems with "Class::operator new" hooking by MEM_ALLOCATOR()
 *		(register-allocated arguments, implicit arguments etc.). In this case, make this function to be inline
 *		wrapper around non-cpp cdecl function, which will use MEM_ALLOCATOR().
 *	2.	use MEM_ALLOCATOR() for wrappers, which will call appMalloc() ONLY (directly or indirectly; no memory chains!);
 *		if call to appMalloc() suppressed, should use MEM_ALLOCATOR(NULL) if MEM_ALLOCATOR(arg) was used. (??)
 */
CORE_API extern address_t GCurrentMemAllocator;
#	define MEM_ALLOCATOR(firstarg)	GCurrentMemAllocator = GET_RETADDR(firstarg)
#else
#	define MEM_ALLOCATOR(firstarg)
#endif

#define DEFAULT_ALIGNMENT	8
#define MEM_CHUNK_SIZE		0x2000		// 8Kb

// If we use alignment=8, this will allocate 7 additional bytes to ensure this condition,
// so - use alignment=1 by default (appMalloc uses malloc(), which itself returns aligned
// blocks (in VC by 8 bytes) )
CORE_API void *appMalloc(size_t size, int alignment = 1);
CORE_API void appFree(void *ptr);


// WARNING: when used multiple inheritance, and one of base classes is CMemoryChain, it should
// be 1st in a base class list (because "operator new" initializes object with this assumption)
class CORE_API CMemoryChain
{
#if MEM_DEBUG
	friend int BuildAllocatorsTable(void *info, int maxCount);
#endif
private:
	CMemoryChain *next;
	int		size;
	byte	*data;
	byte	*end;
#if MEM_DEBUG
	address_t owner;
	CMemoryChain *dPrev, *dNext;
	static CMemoryChain *first;
	void	Link();
	void	Unlink();
#endif
public:
	void* Alloc(size_t size, int alignment = DEFAULT_ALIGNMENT);
	// creating chain
	void* operator new(size_t size, int dataSize = MEM_CHUNK_SIZE);
	// deleting chain
	void operator delete(void* ptr);
	// stats
	int GetSize() const;
};


// VC and GCC does not allow to make "operator new (size,alignment=1)" (def. param) ONLY; so - we
// have 2 versions of this operators ...

FORCEINLINE void* operator new(size_t size)
{
	return appMalloc(size, 1);
}

FORCEINLINE void* operator new(size_t size, int alignment)
{
	return appMalloc(size, alignment);
}

FORCEINLINE void operator delete(void* ptr)
{
	appFree(ptr);
}

// allocating block from chain
FORCEINLINE void* operator new(size_t size, CMemoryChain *chain, int alignment = DEFAULT_ALIGNMENT)
{
	return chain->Alloc(size, alignment);
}

#if __GNUC__
// for GCC: operator new[]/delete[]
// exact copy of non-[] operators ...

FORCEINLINE void* operator new[](size_t size)
{
	return appMalloc(size, 1);
}

FORCEINLINE void* operator new[](size_t size, int alignment)
{
	return appMalloc(size, alignment);
}

FORCEINLINE void operator delete[](void *ptr)
{
	appFree(ptr);
}

FORCEINLINE void* operator new[](size_t size, CMemoryChain *chain, int alignment = DEFAULT_ALIGNMENT)
{
	return chain->Alloc(size, alignment);
}

#endif

// hack to initialize VMT for objects, created without using "operator new"
FORCEINLINE void* operator new(unsigned size, char *buffer)
{
	return buffer;
}
//#define CALL_CONSTRUCTOR(obj,type)		obj = new ((char*)obj) type
template<class T> FORCEINLINE void CALL_CONSTRUCTOR(T *c)
{
	c = new ((char*)c) T;
}
