#include "CorePrivate.h"


/*-----------------------------------------------------------------------------
	String comparision
-----------------------------------------------------------------------------*/

int appStrcmp (const char *s1, const char *s2)
{
	char c1, c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;
	} while (c1 && c1 == c2);

	return c1 - c2;
}


int appStrncmp (const char *s1, const char *s2, int count)
{
	char c1, c2;

	do
	{
		if (!count--) return 0;
		c1 = *s1++;
		c2 = *s2++;
	} while (c1 && c1 == c2);

	return c1 - c2;
}


int appStricmp (const char *s1, const char *s2)
{
	char c1, c2;

	do
	{
		c1 = toLower (*s1++);
		c2 = toLower (*s2++);
	} while (c1 && c1 == c2);

	return c1 - c2;
}


int appStrnicmp (const char *s1, const char *s2, int count)
{
	char c1, c2;

	do
	{
		if (!count--) return 0;
		c1 = toLower (*s1++);
		c2 = toLower (*s2++);
	} while (c1 && c1 == c2);

	return c1 - c2;
}


/*-----------------------------------------------------------------------------
	String copying
-----------------------------------------------------------------------------*/

void appStrcpy (char *dst, const char *src)
{
	char c;
	do
	{
		c = *src++;
		*dst++ = c;
	} while (c);
}


void appStrncpy (char *dst, const char *src, int count)
{
	char c;
	do
	{
		if (!count--) return;
		c = *src++;
		*dst++ = c;
	} while (c);
}


void appStrncpylwr (char *dst, const char *src, int count)
{
	if (count <= 0) return;

	char c;
	do
	{
		if (!--count)
		{
			// out of dst space -- add zero to the string end
			*dst = 0;
			return;
		}
		c = toLower (*src++);
		*dst++ = c;
	} while (c);
}


void appStrncpyz (char *dst, const char *src, int count)
{

	if (count <= 0) return;	// zero-length string

	char c;
	do
	{
		if (!--count)
		{
			// out of dst space -- add zero to the string end
			*dst = 0;
			return;
		}
		c = *src++;
		*dst++ = c;
	} while (c);
}


// NOTE: not same, as strncat(dst, src, count): strncat's "count" is a maximum length of "src", but
//	here "count" is maximal result size of "dst" ...
void appStrcatn (char *dst, int count, const char *src)
{
	char *p = strchr (dst, 0);
	int maxLen = count - (p - dst);
	if (maxLen > 1)
		appStrncpyz (p, src, maxLen);
}


void appCopyFilename (char *dest, const char *src, int len)
{
	guardSlow(appCopyFilename);

	char c;
	// copy name with replacing '\' -> '/' and lowercasing
	const char *s = src;
	char *d = dest;
	while (len--)
	{
		c = *s++;
		if (c == '\\')
			c = '/';
		else if (c >= 'A' && c <= 'Z')
			c += 32;
		*d++ = c;
		if (!c) break;
	}
	if (len < 0) *(--d) = 0;

	s = d = dest;
	len = 0;
	do
	{
		c = *s++;
		if (c == '/')
		{
			// replace '//' -> '/' (may be when "path/" + "/name") when not at start (allow win32 network path)
			if (len) while (*s == '/') s++;
			if (s[0] == '.' && s[1] == '.' && s[2] == '/'&& len && *(s-2) != '.')	// cut "dir/../", but leave "../.."
			{
				do
				{
					d--;
					len--;
				} while (len && *d != '/');
				if (*d == '/')
				{
					d++;
					len++;
				}
				c = s[3];
				s += 4;
			}
			else if (s[0] == '.' && s[1] == '/')	// cut "/./"
				s += 2;
		}
		*d++ = c;
		len++;
	} while (c);

	unguardSlow;
}


/*-----------------------------------------------------------------------------
	Colorized strings support
-----------------------------------------------------------------------------*/

int appCStrlen (const char *str)
{
	int len = 0;
	while (char c = *str++)
	{
		if (c == COLOR_ESCAPE)
		{
			char c1 = *str;
			if (c1 >= '0' && c1 <= '7')
			{
				str++;
				continue;
			}
		}
		len++;
	}

	return len;
}


void appUncolorizeString (char *dst, const char *src)
{
	char c;

	if (!src) src = dst;
	do
	{
		c = *src++;
		if (c == COLOR_ESCAPE)
		{
			char c1 = *src;
			if (c1 < '0' || c1 > '7')
				*dst++ = c;
			else
				src++;
		}
		else
			*dst++ = c;
	} while (c);
}


/*-----------------------------------------------------------------------------
	String formatting
-----------------------------------------------------------------------------*/

#define VA_GOODSIZE		512
#define VA_BUFSIZE		2048

// name of this function is a short form of "VarArgs"
const char *va (const char *format, ...)
{
	guardSlow(va);

	static char buf[VA_BUFSIZE];
	static int bufPos = 0;
	// wrap buffer
	if (bufPos >= VA_BUFSIZE - VA_GOODSIZE) bufPos = 0;

	va_list argptr;
	va_start (argptr, format);

	// print
	char *str = buf + bufPos;
	int len = vsnprintf (str, VA_BUFSIZE - bufPos, format, argptr);
	if (len < 0 && bufPos > 0)
	{
		// buffer overflow - try again with printing to buffer start
		bufPos = 0;
		str = buf;
		len = vsnprintf (buf, VA_BUFSIZE, format, argptr);
	}

	va_end (argptr);

	if (len < 0)					// not enough buffer space
	{
		const char suffix[] = " ... (overflow)";		// it is better, than return empty string
		memcpy (buf + VA_BUFSIZE - sizeof(suffix), suffix, sizeof(suffix));
		return str;
	}

	bufPos += len + 1;
	return str;

	unguardSlow;
}


int appSprintf (char *dest, int size, const char *fmt, ...)
{
	guardSlow(appSprintf);

	va_list	argptr;
	va_start (argptr, fmt);
	int len = vsnprintf (dest, size, fmt, argptr);
	va_end (argptr);
	if (len < 0 || len >= size - 1)
		appWPrintf ("appSprintf: overflow of %d (called by \"%s\")\n", size, appSymbolName (GET_RETADDR(dest)));

	return len;

	unguardSlow;
}


/*-----------------------------------------------------------------------------
	Simple wildcard matching
-----------------------------------------------------------------------------*/

// Mask variants:
// 1) *      - any file
// 2) *.*    - any file
// 3) *rest  - name ends with "rest" (for example, ".ext")
// 4) start* - name starts with "start"
// 4) text   - name equals "text"
// Comparision is case-sensitive, when ignoreCase == false (default)
// A few masks can be separated with ','
bool appMatchWildcard (const char *name, const char *mask, bool ignoreCase)
{
	guardSlow(appMatchWildcard);

	if (!name[0] && !mask[0]) return true;		// empty strings matched

	TString<256> MaskCopy, NameCopy;
	if (ignoreCase)
	{
		NameCopy.toLower (name);
		name = NameCopy;
		MaskCopy.toLower (mask);
	}
	else
		MaskCopy = mask;
	int namelen = strlen (name);

	char *next;
	for (mask = MaskCopy; mask; mask = next)
	{
		// find next wildcard (comma-separated)
		next = strchr (mask, ',');
		int masklen;
		if (next)
		{
			masklen = next - mask;
			next++;					// skip ','
		}
		else
			masklen = strlen (mask);

		if (!masklen)
		{
			// used something like "mask1,,mask3" (2nd mask is empty)
//??			Com_DPrintf ("appMatchWildcard: skip empty mask in \"%s\"\n", mask);
			continue;
		}

		// check for a trivial wildcard
		if (mask[0] == '*')
		{
			if (masklen == 1 || (masklen == 3 && mask[1] == '.' && mask[2] == '*'))
				return true;		// "*" or "*.*" -- any name valid
		}

		// "*text*" mask
		if (masklen >= 3 && mask[0] == '*' && mask[masklen-1] == '*')
		{
			int		i;

			mask++;
			masklen -= 2;
			for (i = 0; i <= namelen - masklen; i++)
				if (!memcmp (&name[i], mask, masklen)) return true;
		}
		else
		{
			// "*text" or "text*" mask
			char *suff = strchr (mask, '*');
			if (next && suff >= next) suff = NULL;		// suff can be in next wildcard
			if (suff)
			{
				int preflen = suff - mask;
				int sufflen = masklen - preflen - 1;
				suff++;

				if (namelen < preflen + sufflen)
					continue;		// name is not long enough
				if (preflen && memcmp (name, mask, preflen))
					continue;		// different prefix
				if (sufflen && memcmp (name + namelen - sufflen, suff, sufflen))
					continue;		// different suffix

				return true;
			}
			// exact match ("text")
			if (namelen == masklen && !memcmp (name, mask, namelen))
				return true;
		}
	}

	return false;
	unguardSlow;
}


bool appIsWildcard (const char *string)
{
	if (strchr (string, '*')) return true;
	if (strchr (string, ',')) return true;
	return false;
}


/*-----------------------------------------------------------------------------
	String allocation
-----------------------------------------------------------------------------*/

char *appStrdup (const char *str)
{
	MEM_ALLOCATOR(str);
	int size = strlen (str) + 1;
	char *out = (char*)appMalloc (size);
	memcpy (out, str, size);
	return out;
}

char *appStrdup (const char *str, CMemoryChain *chain)
{
	int size = strlen (str) + 1;
	char *out = (char*)chain->Alloc (size);
	memcpy (out, str, size);
	return out;
}


/*-----------------------------------------------------------------------------
	CStringList
-----------------------------------------------------------------------------*/

#ifdef STRING_ITEM_TRICK
// We allocate string in the same block, as CStringItem; but we cannot initialize
// CStringItem.name field in common case: offset to this field depends on VMT presense
// in derived class (basic CStringItem - have no virtual methods, but derived
// class can have it ...)
// To solve this problem, we using a simple trick with static AllocatedName and default
// constructor for CStringItem.
char *CStringItem::AllocatedName;

// used in conjunction with "CStringItem::operator new" only!
CStringItem::CStringItem ()
:	name (AllocatedName)			// properly initialize CStringItem::name field
{}
#endif

void* CStringItem::operator new (size_t size, const char *str)
{
	guardSlow(CStringItem::new);
	MEM_ALLOCATOR(size);
	int len = strlen (str) + 1;
	CStringItem *item = (CStringItem*) appMalloc (size + len);
#ifndef STRING_ITEM_TRICK
	item->name    = (char*) OffsetPointer (item, size);
	memcpy (item->name, str, len);	// faster than strcpy()
#else
	AllocatedName = (char*) OffsetPointer (item, size);
	memcpy (AllocatedName, str, len);
#endif

	return item;
	unguardSlow;
}


void* CStringItem::operator new (size_t size, const char *str, CMemoryChain *chain)
{
	guardSlow(CStringItem::new(chain));
#if 0
	// allocate CStringItem and string in a single block
	int len = strlen (str) + 1;
	CStringItem *item = (CStringItem*) chain->Alloc (size + len);
	item->name = (char*) OffsetPointer (item, size);
	memcpy (item->name, str, len);	// faster than strcpy()
#else
	// 2 separate blocks: may be more effective memory usage (CMemoryChain architecture)
	CStringItem *item = (CStringItem*) chain->Alloc (size);
  #ifndef STRING_ITEM_TRICK
	item->name    = appStrdup (str, chain);
  #else
	AllocatedName = appStrdup (str, chain);
  #endif
#endif

	return item;
	unguardSlow;
}


int CStringList::GetCount ()
{
	int i = 0;
	for (CStringItem *item = first; item; item = item->next)
		i++;
	return i;
}


CStringItem *CStringList::Find (const char *name, CStringItem **after)
{
	guardSlow(CStringItem::Find(str,after));
	if (after) *after = NULL;
	for (CStringItem *item = first; item; item = item->next)
	{
		int cmp = appStricmp (name, item->name);
		if (!cmp)
			// found exact item
			return item;

		if (cmp < 0)
			return NULL;			// list is alpha-sorted, and item should be before this place
		if (after) *after = item;
	}

	return NULL;
	unguardSlow;
}


const CStringItem *CStringList::Find (const char *name) const
{
	guardSlow(CStringItem::Find(str) const);
	for (const CStringItem *item = first; item; item = item->next)
	{
		int cmp = appStricmp (name, item->name);
		if (!cmp)
			// found exact item
			return item;

		if (cmp < 0)
			return NULL;			// list is alpha-sorted, and item should be before this place
	}

	return NULL;
	unguardSlow;
}


CStringItem* CStringList::Find (int index)
{
	guardSlow(CStringItem::Find(idx));
	CStringItem *item;
	for (item = first; item && index > 0; item = item->next, index--) ;
	return item;
	unguardSlow;
}


//!! note: this function works with unsorted list, but Find() -- with sorted; should change!
int CStringList::IndexOf (const char *str)
{
	guardSlow(CStringList::IndexOf);
	int		index;
	CStringItem *item;
	for (item = first, index = 0; item; item = item->next, index++)
		if (!appStricmp (str, item->name))		//!! NOTE: when list is alpha-sorted, can return NULL if cmp<0
			return index;
	return -1;
	unguardSlow;
}


void CStringList::InsertLast (CStringItem *item)
{
	guardSlow(CStringItem::InsertLast);
	if (!first)
	{
		first = item;
		return;
	}
	// here: at least 1 element in list
	CStringItem *curr;
	for (curr = first; curr->next; curr = curr->next) ; // find list end
	curr->next = item;
	unguardSlow;
}


bool CStringList::Remove (CStringItem *item)
{
	guardSlow(CStringList::Remove);
	CStringItem *prev = NULL;
	for (CStringItem *curr = first; curr; curr = curr->next)
	{
		if (curr == item)
		{
			// unlink item
			if (prev)
				prev->next = curr->next;
			else
				first = curr->next;
			return true;
		}
		prev = curr;
	}
	return false;					// not found
	unguardSlow;
}


void CStringList::Free ()
{
	guardSlow(CStringList::Free);
	CStringItem *item, *next;
	for (item = first; item; item = next)
	{
		next = item->next;
		appFree (item);
	}
	unguardSlow;
}
