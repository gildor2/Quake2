struct CTextRec
{
	char	*text;
	CTextRec *next;
};


class CORE_API CTextContainer
{
protected:
	// static values
	int		recSize;
	char	*textBuf;
	int		bufSize;
	// dynamic values
	bool	filled;			// !empty
	int		fillPos;
	CTextRec *lastRec;

public:
	inline void Clear ()
	{
		filled = false;
	}
	CTextRec *Add (const char *text);
	void Enumerate (void (*func) (const CTextRec *rec));
};


//#define TEXT_CONTAINER_STATIC		// VC++6 will produce small code/data, but VC++7 will fill
									// constructed fields in compile-time, and data will be placed
									// in initialized section (lots of zeros ...)

template<class R, int BufferSize> class TTextContainer : public CTextContainer
{
#ifdef TEXT_CONTAINER_STATIC
private:
	char	buffer[BufferSize];
#endif
public:
	inline TTextContainer ()
	{
#ifdef TEXT_CONTAINER_STATIC
		textBuf = buffer;
#else
		textBuf = new char [BufferSize];
#endif
		bufSize = BufferSize;
		recSize = sizeof(R);
	}
#ifndef TEXT_CONTAINER_STATIC
	~TTextContainer ()
	{
		delete textBuf;
	}
#endif
	inline R *Add (const char *text)
	{
		return static_cast<R*>(CTextContainer::Add (text));
	}
	inline void Enumerate (void (*func) (const R *rec))
	{
		CTextContainer::Enumerate ((void(*)(const CTextRec*)) func);
	}
};
