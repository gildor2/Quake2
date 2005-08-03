struct CTextRec
{
	char	*text;
	CTextRec *next;
};


CORE_API class CTextContainer
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


template<class R, int BufferSize> class TTextContainer : public CTextContainer
{
private:
	char	buffer[BufferSize];
public:
	inline TTextContainer ()
	{
		textBuf = buffer;
		bufSize = BufferSize;
		recSize = sizeof(R);
	}
	inline R *Add (const char *text)
	{
		return static_cast<R*>(CTextContainer::Add (text));
	}
	inline void Enumerate (void (*func) (const R *rec))
	{
		CTextContainer::Enumerate ((void(*)(const CTextRec*)) func);
	}
};
