#include "qcommon.h"


/*------- Memory chain allocation (by chunks) -----------*/


#define CHUNK_SIZE 0x2000 // 8Kb

// sequence of blocks (with using "next" field): 1->5->4->3->2->NULL
typedef struct chainBlock_s
{
	struct chainBlock_s *next;
	int		free;	// size of free space in current block
	byte	*data;	// pointer to a free space
	int		size;	// just for memory usage statistics
} chainBlock_t;


#define BLOCK_ALIGN	16

static chainBlock_t *AllocChain(int size)
{
	int		alloc;
	chainBlock_t *chain;

	alloc = size + BLOCK_ALIGN - 1;
	chain = (chainBlock_t*)malloc (alloc);
	if (!chain)
		Com_FatalError ("AllocChunk: failed on allocation of %d bytes", size);
	chain->size = size;
	chain->next = NULL;
	chain->data = (byte*) (((int) (chain + 1) + BLOCK_ALIGN-1) & ~(BLOCK_ALIGN-1));
	chain->free = (byte*)chain + alloc - (byte*)chain->data;

	memset (chain->data, 0, chain->free);

	return chain;
}


void *CreateMemoryChain (void)
{
	return AllocChain (CHUNK_SIZE);
}


void FreeMemoryChain (void *chain)
{
	chainBlock_t *curr, *next;

	for (curr = (chainBlock_t*)chain; curr; curr = next)
	{
		// free memory block
		next = curr->next;
//		DebugPrintf("%08X/%X :"RETADDR_STR"\n", curr, curr->size, GET_RETADDR(chain));//!!!!!!
		free (curr);
	}
}


void *AllocChainBlock (void *chain, int size)
{
	chainBlock_t *first, *b;
	byte	*data;

	if (!chain || !size) return NULL;

	first = (chainBlock_t*)chain;
	if (first->next)
		b = first->next;
	else
		b = first;

	if (b->free < size)
	{
		//?? may be, search in other blocks ...
		// allocate in the new block (round up to CHUNK_SIZE)
		b = AllocChain ((size + sizeof(chainBlock_t) + CHUNK_SIZE - 1) & ~(CHUNK_SIZE - 1));
		// insert new block immediately after 1st block (==chain)
		b->next = first->next;
		first->next = b;
	}

	data = b->data;
	b->data += size;
	b->free -= size;

	return data;
}


/*---------------------- Strings ------------------------*/

char *ChainCopyString (const char *in, void *chain)
{
	char	*out;

	int len = strlen (in) + 1;
	out = (char*)AllocChainBlock (chain, len);
	memcpy (out, in, len);
	return out;
}


/*--------------- Named structure lists -----------------*/


// Allocates structure {char *name, ....} and name as one memory block;
// name is lowercased; name points to destination struc + sizeof(struc)
// (this used at least in "files.c" for validating FILE2 structure)
basenamed_t *AllocNamedStruc (int size, char *name)
{
	basenamed_t *s;
	char *n, c;

#ifdef Z_DEBUG
	allocator = GET_RETADDR(size);
#endif
	s = (basenamed_t*)appMalloc (size + strlen (name) + 1);
	n = (char*)s + size; // place of a new name
        // set namefield of allocated structure
	*(char**)s = n;
	do
	{
		c = *name++;
		if (c >= 'A' && c <= 'Z') c += 32;
		*n++ = c;
	} while (c);
	return s;
}


basenamed_t *ChainAllocNamedStruc (int size, char *name, void *chain)
{
	basenamed_t *s;
	char *n, c;
	int len;

	len = strlen (name) + 1;
	s = (basenamed_t*)AllocChainBlock (chain, size + len);
	n = (char*)s + size;	// place of a new name
	// set name field of the allocated structure
	*(char**)s = n;
	do
	{
		c = *name++;
		if (c >= 'A' && c <= 'Z') c += 32;
		*n++ = c;
	} while (c);
	return s;
}


// Look case insensitive for "name" in list of basenamed_t strucs, started with "first".
// Returns found struct if found or NULL elsewhere. If not found, returns place, where to
// insert newly allocated struc in "where" (where=NULL if new entry must be first).
// "where" can be NULL if we are not interested in insertion place.
basenamed_t *FindNamedStruc (char *name, basenamed_t *first, basenamed_t **where)
{
	int cmp;
	basenamed_t *p;

	if (where) *where = NULL;
	for (p = first; p; p = p->next)
	{
		cmp = stricmp (name, p->name);
		if (!cmp)						// found
		{
			if (where) *where = NULL;	// we do not need to create new entry
			return p;
		}
		if (cmp < 0) return NULL;		// insert new item before "p" (and after "where")
		if (where) *where = p;
	}
	return NULL;
}

// returns basenamed_t struc placed at "index" place in "first" list; if out of list -- NULL
basenamed_t *FindNamedStrucByIndex (basenamed_t *first, int index)
{
	basenamed_t *p;
	int		i;

	for (p = first, i = 0; p && i < index; p = p->next, i++) ;
	return p;
}

// returns -1 if not found
int IndexOfNamedStruc (char *name, basenamed_t *first)
{
	int		i;
	basenamed_t *p;

	for (p = first, i = 0; p; p = p->next, i++)
		if (!stricmp (p->name, name))
			return i;
	return -1;
}


basenamed_t *AddToNamedList (char *name, basenamed_t *list)
{
	basenamed_t *item, *insitem;

	if (FindNamedStruc (name, list, &insitem))
		return list; // already exists
	// allocate new item
	item = AllocNamedStruc (sizeof(basenamed_t), name);
	if (list)
	{
		if (insitem)
		{	// insert after insitem
			item->next = insitem->next;
			insitem->next = item;
		}
		else
		{	// insert first
			item->next = list;
			list = item;
		}
	} else list = item; // 1st item in list
	return list;
}


basenamed_t *ChainAddToNamedList (char *name, basenamed_t *list, void *chain)
{
	basenamed_t *item, *insitem;

	if (FindNamedStruc (name, list, &insitem))
		return list; // already exists
	// allocate new item
	item = ChainAllocNamedStruc (sizeof(basenamed_t), name, chain);
//??	if (list)
//	{
		if (insitem)
		{	// insert after insitem
			item->next = insitem->next;
			insitem->next = item;
		}
		else
		{	// insert first
			item->next = list;
			list = item;
		}
//	} else list = item; // 1st item in list
	return list;
}


void FreeNamedList (basenamed_t *list)
{
	basenamed_t *s;

	while (list)
	{
		s = list->next;
		FreeNamedStruc (list);
		list = s;
	}
}
