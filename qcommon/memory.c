#include "qcommon.h"


/*-------------- Zone memory allocation -----------------*/

#define		Z_MAGIC		0x5678
#define		Z_BLOCKSIZE	4096

// debug stuff:
//#define 	Z_DEBUG
#define		Z_FREE		0x1234
#define		Z_DEBUGMARK	0xABCDEF12
/*!! Additional debug:
  - store "Z_Malloc called from" field
  - command "z_mark" - mark all allocated blocks (newly created are unmarked)
  - command "z_show" (?) - display info about all unmarked blocks (createdFrom, size etc.)
*/

typedef struct zhead_s
{
	struct zhead_s *prev, *next;
	short	magic;			// Z_MAGIC or Z_FREE
	short	tag;			// for group free
	int		size;
} zhead_t;

static zhead_t	z_chain;
static int		z_count, z_bytes;


void Z_Free (void *ptr)
{
	zhead_t	*z;
#ifdef Z_DEBUG
	int mark;
#endif

	z = ((zhead_t *)ptr) - 1;

#ifdef Z_DEBUG
	if (!ptr)
	{
		Com_WPrintf ("Z_Free(NULL) " RETADDR_STR "\n", GET_RETADDR(ptr));
		return;
	}

	if (z->magic == Z_FREE)
		Com_Error (ERR_FATAL, "Z_Free: double freeing block");
	mark = *(int*)((char*)z + z->size - 4);
	if (mark != Z_DEBUGMARK)
		Com_Error (ERR_FATAL, "Z_Free: memory overrun detected (block size=%d, diff=%X)",
			z->size - sizeof(zhead_t) - 4, mark ^ Z_DEBUGMARK);
#endif
	if (z->magic != Z_MAGIC)
		Com_Error (ERR_FATAL, "Z_Free: bad magic %X" RETADDR_STR, z->magic, GET_RETADDR(ptr));

#ifdef Z_DEBUG
	z->magic = Z_FREE;
#endif

	z->prev->next = z->next;
	z->next->prev = z->prev;

	z_count--;
	z_bytes -= z->size;
	free (z);
}


void Z_FreeTags (int tag)
{
	zhead_t	*z, *next;

	for (z = z_chain.next; z != &z_chain; z = next)
	{
		next = z->next;
		if (z->tag == tag)
			Z_Free ((void *)(z+1));
	}
}


void *Z_TagMalloc (int size, int tag)
{
	zhead_t	*z;

	size += sizeof(zhead_t);
#ifdef Z_DEBUG
	size += 4;	// reserve space for our mark
#endif

	z = malloc(size);
	if (!z)
		Com_Error (ERR_FATAL, "Z_Malloc: failed on allocation of %i bytes", size);
	memset (z, 0, size);
#ifdef Z_DEBUG
	*((int*)((char*)z + size - 4)) = Z_DEBUGMARK; // place mark in reserved space
#endif
	z_count++;
	z_bytes += size;
	z->magic = Z_MAGIC;
	z->tag = tag;
	z->size = size;

	z->next = z_chain.next;
	z->prev = &z_chain;
	z_chain.next->prev = z;
	z_chain.next = z;

	return (void *)(z+1);
}


void *Z_Malloc (int size)
{
#ifdef Z_DEBUG
	return Z_TagMalloc (size, 0);
#else
	if (size <= Z_BLOCKSIZE * 2)
		return Z_TagMalloc (size, 0);
	else
		return Z_BlockMalloc (size);
#endif
}


// WARNING: cannot add block align here - we should use result pointer for Z_Free() call
void *Z_BlockMalloc (int size)
{
	int blocksize;

	blocksize = ((size + sizeof(zhead_t) + Z_BLOCKSIZE - 1) & ~(Z_BLOCKSIZE - 1)) - sizeof(zhead_t);
	return Z_TagMalloc (blocksize, 0);
}


/*------- Memory chain allocation (by chunks) -----------*/


#define CHUNK_SIZE 0x2000 // 8Kb

// sequence of blocks (with using "next" field): 1->5->4->3->2->NULL
typedef struct chainBlock_s
{
	struct chainBlock_s *next;
	int		free;	// size of free space in current block
	void	*data;	// pointer to a free space
	int		size;	// just for memory usage statistics
} chainBlock_t;


#define BLOCK_ALIGN	16

static chainBlock_t *AllocChain(int size)
{
	chainBlock_t *chain;

	chain = malloc (size + BLOCK_ALIGN-1);
	if (!chain)
		Com_Error (ERR_FATAL, "AllocChunk: failed on allocation of %d bytes", size);
	chain->free = size - sizeof(chainBlock_t);
	chain->next = NULL;
	chain->data = (void*) (((int) (chain + 1) + BLOCK_ALIGN-1) & ~(BLOCK_ALIGN-1));
	chain->size = size;

	memset (&chain[1], 0, size - sizeof(chainBlock_t));

	// update z_stats info
	z_bytes += size;
	z_count++;

	return (void*) chain;
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
		// update z_stats info
		z_bytes -= curr->size;
		z_count--;
		// free memory block
		next = curr->next;
		free (curr);
	}
}


void *AllocChainBlock (void *chain, int size)
{
	chainBlock_t *first, *b;
	void *data;

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
	(char *)(b->data) += size;
	b->free -= size;
	return data;
}


/*---------------------- Strings ------------------------*/


char *CopyString (char *in)
{
	char	*out;

	out = Z_Malloc (strlen (in) + 1);
	strcpy (out, in);
	return out;
}


char *ChainCopyString (char *in, void *chain)
{
	char	*out;

	out = AllocChainBlock (chain, strlen (in) + 1);
	strcpy (out, in);
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

	s = Z_Malloc (size + strlen (name) + 1);
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
	s = AllocChainBlock (chain, size + len);
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


// Memory status console command
static void Z_Stats_f (void)
{
	Com_Printf ("%d bytes in %d blocks\n", z_bytes, z_count);
}


void Z_Init (void)
{
	z_chain.next = z_chain.prev = &z_chain;
	Cmd_AddCommand ("z_stats", Z_Stats_f);
}
