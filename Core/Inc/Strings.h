
#define	MAX_QPATH			64		// max length of a game pathname
#define	MAX_OSPATH			128		// max length of a filesystem pathname
#define MAX_STRING_CHARS	1024


inline char toLower (char c)
{
	return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
}


inline char toUpper (char c)
{
	return c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
}


//?? define strcpy() appStrcpy() etc, do not use appFunctions directly ?
//?? if use own appStrFuncs -- add strlen
CORE_API int appStrcmp (const char *s1, const char *s2);
CORE_API int appStrncmp (const char *s1, const char *s2, int count);
CORE_API int appStricmp (const char *s1, const char *s2);
CORE_API int appStrnicmp (const char *s1, const char *s2, int count);


// Copy null-terminated string src to dst
CORE_API void appStrcpy (char *dst, const char *src);
// Copy src to dst; dst string will be no longer than count chars.
// If buffer is overflowed, dst string is not null-terminated
CORE_API void appStrncpy (char *dst, const char *src, int count);
// Same as appStrncpyz, but destination string will be lowercased
CORE_API void appStrncpylwr (char *dst, const char *src, int count);
// Same as appStrncpy, but when dst buffer is overflowed, it will be null-terminated
// (0 at [count-1] position)
CORE_API void appStrncpyz (char *dst, const char *src, int count);
// Copy filename from src to dst with some path compression, replacing '\' with '/' and lowercasing;
// dst string will fit count chars and null-terminated.
CORE_API void appCopyFilename (char *dst, const char *src, int count);
CORE_API void appStrcatn (char *dst, int count, const char *src);

//?? quake3-style ?? make single-byte color codes + conversion q3<->our style (codes should be i+0..i+7, i != 0 (!!))
//?? + add string codes for colors S_COLOR_RED,S_COLOR_BLACK etc. (diff. names?), + inline funcs for converting color_str -> num (??)
//?? + S_DEFAULT_COLOR, S_WARN_COLOR, S_ERROR_COLOR (not needs S_COLOR_RED/GREEN/etc. with this ?!)
//!! + app[Un]QuoteString(): C-style string [un]quoting using "\?"; should process color codes too
// Visible length of colorized string
CORE_API int appCStrlen (const char *str);
// Remove color marks from string
CORE_API void appUncolorizeString (char *dst, const char *src = NULL);


CORE_API const char *va (const char *format, ...);
CORE_API int appSprintf (char *dest, int size, const char *fmt, ...);


CORE_API bool appMatchWildcard (const char *name, const char *mask, bool ignoreCase = false);


CORE_API char *CopyString (const char *str);
CORE_API char *CopyString (const char *str, CMemoryChain *chain);


/*-----------------------------------------------------------------------------
	String lists
-----------------------------------------------------------------------------*/

// Base item: link to next + string
class CStringItem
{
public:
	char	*name;
	CStringItem *next;
		// really, this field should be "protected" and CStringList should be a friend of CStringItem,
		// but compiler (tested with VC6) will generate error "cannot access protected member (C2248)"
	CORE_API void *operator new (size_t size, const char *str);
	CORE_API void *operator new (size_t size, const char *str, CMemoryChain *chain);
	friend class CStringList;
};


// Base list of strings
class CStringList
{
protected:
	CStringItem *first;

	// manipulating items
	CORE_API CStringItem *Find (const char *name, CStringItem **after);
	CORE_API CStringItem *Find (int index);
public:
	CStringList ()
	{
		first = NULL;
	}
	// function for implicit list initialization
	void Reset ()
	{
		first = NULL;
	}
	CORE_API int IndexOf (const char *str);
	//!! IndexOfWildcard (str)
	// unlink item from list
	CORE_API bool Remove (CStringItem *item);
	// freeing list of items, allocated in generic memory
	CORE_API void Free (void);
};


// Template for list of any objects, derived from CStringItem
template <class T> class TList : public CStringList
{
public:
	// list enumeration
	T *First (void)
	{
		return (T*) first;
	}
	T *Next (T* prev)
	{
		return (T*) prev->next;
	}
	// searching items
	T *Find (const char *name, T **after = NULL)
	{
		return (T*) CStringList::Find (name, (CStringItem **) after);
	}
	T *Find (int index)
	{
		return (T*) CStringList::Find (index);
	}
	// list insertion
	void InsertAfter (T *Item, T *Point)
	{
		if (Point)
		{
			Item->next = Point->next;
			Point->next = Item;
		}
		else
		{
			Item->next = (T*) first;
			first = Item;
		}
	}
	bool Insert (T *item)
	{
		T *prev;
		Find (item->name, &prev);		// allow duplicates
		InsertAfter (item, prev);
		return true;
	}
	// creation and insertion
	T *CreateAndInsert (const char *name)
	{
		T *item = new (name) T;
		Insert (item);
		return item;
	}
	T *CreateAndInsert (const char *name, CMemoryChain *chain)
	{
		T *item = new (name, chain) T;
		Insert (item);
		return item;
	}
	// WARNING: this function can return &NULL when index is out of bounds (generate appError ??)
	T& operator[] (int index)
	{
		return (T&) *CStringList::Find (index);
	}
};


/*-----------------------------------------------------------------------------
	TString template - template for static strings (use instead of "char str[N]")
-----------------------------------------------------------------------------*/

template <int N> class TString
{
public:
	char	str[N];
	int len ()					{ return strlen (str); }
	char& operator[] (int idx)	{ return str[idx]; }

	int cmp (const char *s2)	{ return strcmp (str, s2); }
	int icmp (const char *s2)	{ return stricmp (str, s2); }
	void cpy (const char *s2, int len = N)	{ strncpy (str, s2, Min(N,len)); }
	void cpyz (const char *s2)	{ appStrncpyz (str, s2, N); }
	void cat (const char *s2)	{ appStrcatn (str, N, s2); }
	char chr (char c)			{ return strchr (str, c); }
	char rchr (char c)			{ return strrchr (str, c); }
	void toLower () { appStrncpylwr (str, str, N); }
	TString& operator= (const char *s2)
	{
		strncpy (str, s2, N);
		return *this;
	}
	bool operator< (const char *s2)		{ return strcmp (str, s2) < 0; }
	bool operator<= (const char *s2)	{ return strcmp (str, s2) <= 0; }
	bool operator== (const char *s2)	{ return strcmp (str, s2) == 0; }
	template <int M> bool operator== (const TString<M> &S) { return strcmp (str, S.str) == 0; }
	bool operator!= (const char *s2)	{ return strcmp (str, s2) != 0; }
	template <int M> bool operator!= (const TString<M> &S) { return strcmp (str, S.str) != 0; }
	bool operator> (const char *s2)		{ return strcmp (str, s2) > 0; }
	bool operator>= (const char *s2)	{ return strcmp (str, s2) >= 0; }
	operator char*()					{ return str; }
};
