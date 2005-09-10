
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
CORE_API void appStrncpylwr (char *dst, const char *src, int count = BIG_NUMBER);
// Same as appStrncpy, but when dst buffer is overflowed, it will be null-terminated
// (0 at [count-1] position)
CORE_API void appStrncpyz (char *dst, const char *src, int count);
// Copy filename from src to dst with some path compression, replacing '\' with '/' and lowercasing;
// dst string will fit count chars and null-terminated.
CORE_API void appCopyFilename (char *dst, const char *src, int count = BIG_NUMBER);
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
CORE_API bool appIsWildcard (const char *string);

//?? rename to appStrdup()
CORE_API char *CopyString (const char *str);
CORE_API char *CopyString (const char *str, CMemoryChain *chain);


/*-----------------------------------------------------------------------------
	String lists
-----------------------------------------------------------------------------*/

// can comment next #define, when no classes with VMT derived from CStringItem
#define STRING_ITEM_TRICK

// Base item: link to next + string
class CStringItem
{
	friend class CStringList;
	friend class CListIterator;
//	template<class T> friend class TList; -- not compiled in VC6
private:
#ifdef STRING_ITEM_TRICK
	static char *AllocatedName;
#endif
public:
	char	*name;
	CStringItem *next;
		// really, this field should be "protected" and CStringList should be a friend of CStringItem,
		// but VC6 will generate error "cannot access protected member (C2248)" for TList<>
#ifdef STRING_ITEM_TRICK
	CORE_API CStringItem ();	// will initialize "name" with AllocatedName' value
#endif
	CORE_API void *operator new (size_t size, const char *str);
	CORE_API void *operator new (size_t size, const char *str, CMemoryChain *chain);
};


// Base list of strings
class CStringList
{
	friend class CListIterator;
protected:
	CStringItem *first;
	// searching items in a sorted list
	CORE_API CStringItem *Find (const char *name, CStringItem **after);
	CORE_API const CStringItem *Find (const char *name) const;
	CORE_API CStringItem *Find (int index);
	// unsorted list insertion
	inline void InsertFirst (CStringItem *item)
	{
		item->next = first;
		first = item;
	}
	CORE_API void InsertLast (CStringItem *item);
public:
	inline CStringList ()
	:	first (NULL)
	{}
	// function for implicit list initialization
	inline void Reset ()
	{
		first = NULL;
	}
	inline operator bool ()
	{
		return first != NULL;
	}
	// sorted search
	CORE_API int IndexOf (const char *str);
	//?? IndexOfWildcard (str)
	//?? unsorted search: FindUnsorted(str)
	//!! Sort(cmpFunc) -- resort with any strategy
	//!!   cmpFunc may be of 2 types: (CStringItem*,CStringItem*) or (char*,char*)
	// unlink item from list
	CORE_API bool Remove (CStringItem *item);
	// freeing list of items, allocated in generic memory
	CORE_API void Free ();
};


// Template for list of any objects, derived from CStringItem
template<class T> class TList : public CStringList
{
public:
	// list enumeration
	T *First ()
	{
		return static_cast<T*>(first);
	}
	T *Next (const T* prev)
	{
		return static_cast<T*>(prev->next);
	}
	// searching items
	T *Find (const char *name, T **after = NULL)
	{
		return static_cast<T*>(CStringList::Find (name, (CStringItem **) after));
	}
	const T *Find (const char *name) const
	{
		return static_cast<const T*>(CStringList::Find (name));
	}
	T *Find (int index)
	{
		return static_cast<T*>(CStringList::Find (index));
	}
	// unsorted list insertion
	void InsertFirst (T *item)
	{
		CStringList::InsertFirst (item);
	}
	void InsertLast (T *item)
	{
		CStringList::InsertLast (item);
	}
	void InsertAfter (T *Item, T *Point)
	{
		if (Point)
		{
			Item->next = Point->next;
			Point->next = Item;
		}
		else
		{
			Item->next = static_cast<T*>(first);
			first = Item;
		}
	}
	// sorted list insertion
	bool Insert (T *item)
	{
		T *prev;
		Find (item->name, &prev);		// allow duplicates
		InsertAfter (item, prev);
		return true;
	}
	// creation and sorted insertion
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
		return static_cast<T&>(*CStringList::Find (index));
	}
};


// Iterator for CStringList
class CListIterator
{
protected:
	CStringItem *item;
public:
	// initialization
	CListIterator ()
	:	item (NULL)
	{}
	CListIterator (CStringList &list)
	:	item (list.first)
	{}
	CListIterator& operator= (const CStringList &list)
	{
		item = list.first;
		return *this;
	}
	// enumerator
	void operator++ ()		// prefix form
	{
		item = item->next;
	}
	// do not define postfix form to require use of "++item" - style (better readability)
//	void operator++ (int)	// postfix form
//	{
//		item = item->next;
//	}
	operator bool ()
	{
		return item != NULL;
	}
	// accessing enumerated data
	CStringItem* operator-> ()
	{
		return item;
	}
	CStringItem* operator* ()
	{
		return item;
	}
};


template<class T> class TListIterator : public CListIterator
{
public:
	// initialization
	TListIterator (TList<T> &list)
	:	CListIterator (list)
	{}
	TListIterator& operator= (const T &list)
	{
		item = list.First();
		return *this;
	}
	// data access
	T* operator-> ()
	{
		return static_cast<T*>(item);
	}
	T* operator* ()
	{
		return static_cast<T*>(item);
	}
};

// How to use iterators:
// ---------------------
// CListIterator -- for CStringList only (or, when used for derived classes, will be used as CStringItem)
//	a) for (CListIterator item = list; item; ++item)
//	b) for (CListIterator item(list); item; ++item)							-- same as a)
//	c) for (CStringItem *item = list->first; item; item = item->next)		-- long form without iterators
// TListIterator -- for TStringList<>
//	a) for (TListIterator<CItem> item = ItemList; item; ++item)
//	b) for (TListIterator<CItem> item(ItemList); item; ++item)
//	c) for (CItem *item = ItemList.First(); item; item = ItemList.Next(item))
// When required access to current item, use "*item" (overloaded operator*())


/*-----------------------------------------------------------------------------
	TString template - template for static strings (use instead of "char str[N]")
-----------------------------------------------------------------------------*/

template<int N> class TString
{
private:
	char	str[N];
public:
// no constructors ... anyway, 1/2 of initialization will be done with filename()
#if 0
	inline TString ()						{}
	inline TString (const char *s)			{ appStrncpyz (str, s, N); }
#endif
	int len () const						{ return strlen (str); }
	char& operator[] (int idx)				{ return str[idx]; }
	const char& operator[] (int idx) const	{ return str[idx]; }
	// copying
	void cpy (const char *s2, int len = N)	{ strncpy (str, s2, min(N,len)); }
	void cpyz (const char *s2)				{ appStrncpyz (str, s2, N); }
	void filename (const char *s2)			{ appCopyFilename (str, s2, N); }	//?? rename function
	void cat (const char *s2)				{ appStrcatn (str, N, s2); }
	TString<N>& operator+= (const char *s2)	{ appStrcatn (str, N, s2); return *this; }
	int sprintf (const char *fmt, ...)
	{
		va_list	argptr;
		va_start (argptr, fmt);
		int len = vsnprintf (str, N, fmt, argptr);
		va_end (argptr);
		return len;
	}
	// searching
	char *chr (char c)						{ return strchr (str, c); }
	const char *chr (char c) const			{ return strchr (srt, c); }
	char *rchr (char c)						{ return strrchr (str, c); }
	const char *rchr (char c) const			{ return strrchr (str, c); }
	// lowercasing
	void toLower ()							{ appStrncpylwr (str, str, N); }
	void toLower (const char *s2, int len = N) { appStrncpylwr (str, s2, min(N,len)); }
	// assignment
	TString& operator= (const char *s2)
	{
		strncpy (str, s2, N);
		return *this;
	}
	// string comparision
	// NOTE: comparision as class members suitable for cases, when TString<> is on a left
	// side of operator; when TString on the right side, and "char*" on the left, by default,
	// will be executed "str <operator> char*(Str)", i.e. will be performed address comparision;
	// so, to correct this situation, we overloaded binary operator (char*,TString<>&) too
	int cmp (const char *s2) const			{ return strcmp (str, s2); }
	int icmp (const char *s2) const			{ return stricmp (str, s2); }
	bool operator< (const char *s2) const	{ return strcmp (str, s2) < 0; }
	bool operator<= (const char *s2) const	{ return strcmp (str, s2) <= 0; }
	bool operator== (const char *s2) const	{ return strcmp (str, s2) == 0; }
	template<int M> bool operator== (const TString<M> &S) const
	{ return strcmp (str, S.str) == 0; }
	bool operator!= (const char *s2) const	{ return strcmp (str, s2) != 0; }
	template<int M> bool operator!= (const TString<M> &S) const
	{ return strcmp (str, S.str) != 0; }
	bool operator> (const char *s2) const	{ return strcmp (str, s2) > 0; }
	bool operator>= (const char *s2) const	{ return strcmp (str, s2) >= 0; }
	// implicit use as "const char*"
	operator const char* () const			{ return str; }
	operator char* ()						{ return str; }
	// use "*Str" when used in printf()-like functions (no type conversion by default ...)
	const char* operator* () const			{ return str; }
#if 0
// do not make "operator bool", because this will introduce conflicts with "operator char*" -- compiler
// can not deside, which conversion to use in statements ...
	// check for empty string (true when non-empty)
	operator bool () const					{ return str[0] != 0; }
#endif
};

template<int N> inline bool operator== (const char *s1, TString<N> &s2)
{
	return strcmp (s1, *s2) == 0;
}

template<int N> inline bool operator!= (const char *s1, TString<N> &s2)
{
	return strcmp (s1, *s2) != 0;
}
