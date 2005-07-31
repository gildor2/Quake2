
#if 0
// using templates is very tedious - will require to use Bound(float,0,10) -> Bound(float,0.0f,10.0f)
// (not double!) and even Bound(int,1,10) will give error abount ambiguous "int" and "unsigned" ...
template<class T> inline T Max (const T a, const T b)
{
	return (a >= b) ? a : b;
}

template<class T> inline T Min (const T a, const T b)
{
	return (a <= b) ? a : b;
}

template<class T> inline T Bound (const T v, const T minval, const T maxval)
{
	return v < minval ? minval : v > maxval ? maxval : v;
}
#endif

#define min(a,b)  (((a) < (b)) ? (a) : (b))
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#define bound(a,minval,maxval)  ( ((a) > (minval)) ? ( ((a) < (maxval)) ? (a) : (maxval) ) : (minval) )

// Align integer or pointer of any type
template<class T> inline T Align (const T ptr, int alignment)
{
	return (T) (((unsigned)ptr + alignment - 1) & ~(alignment - 1));
}

template<class T> inline int SizeToAlign (const T ptr, int alignment)
{
	return (unsigned)Align (ptr, alignment) - (unsigned)ptr;
}

template<class T> inline T OffsetPointer (const T ptr, int offset)
{
	return (T) ((unsigned)ptr + offset);
}

//?? make 2 functions: for float and for int; as CORE_API
template<class T> int Sign (T value)
{
	if (value < 0) return -1;
	else if (value > 0) return 1;
	return 0;
}


template<class T> inline void Exchange (T& A, T& B)
{
	const T tmp = A;
	A = B;
	B = tmp;
}


template<class T> T Lerp (const T& A, const T& B, float Alpha)
{
	return A + Alpha * (B-A);
}


/*
template<class T> int Cmp (const T& A, const T& B)
{
	return memcmp (&A, &B, sizeof(T));
}
*/


template<class T> void Zero (T& A)
{
	memset (&A, 0, sizeof(T));
}


#define EXPAND_BOUNDS(a,minval,maxval)	\
	if (a < minval) minval = a;			\
	if (a > maxval) maxval = a;

#define VECTOR_ARG(name)	name[0],name[1],name[2]
#define ARRAY_ARG(array)	array, sizeof(array)/sizeof(array[0])
#define ARRAY_COUNT(array)	(sizeof(array)/sizeof(array[0]))


// use "STR(any_value)" to convert it to string (may be float value)
#define STR2(s) #s
#define STR(s) STR2(s)

// field offset macros
#define FIELD2OFS(struc, field)		((unsigned) &((struc *)NULL)->field)		// get offset of the field in struc
#define OFS2FIELD(struc, ofs, type)	(*(type*) ((byte*)(struc) + ofs))			// get field of type by offset inside struc

#define EXEC_ONCE(code)	\
	{					\
		static bool _flg = false; \
		if (!_flg) {	\
			_flg = true; \
			code;		\
		}				\
	}
