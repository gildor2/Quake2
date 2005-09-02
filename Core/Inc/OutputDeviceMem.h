//!! call Flush() when buffer filled (by 1/2 ?); if not flushed (memory-only) -- do not try again??
//!! bool: flushOnFull, flushOnHalf ...

class COutputDeviceMem : public COutputDevice
{
private:
	char	*buffer;
	int		used;
	int		size;
public:
	COutputDeviceMem (int bufSize = 4096)
	{
		size = bufSize;
		buffer = new char [bufSize];
//		buffer[0] = 0;
		used = 0;
	}
	~COutputDeviceMem ()
	{
		Unregister ();
		delete buffer;
	}
	inline const char *GetText () const
	{
		return buffer;
	}
	void Write (const char *str)
	{
		int len = strlen (str);
		//?? Move S_WHITE part to appPrintf(); add when string is colored only!
		//?? Problem with this: when string not fits to buffer in 1 byte,
		//?? COLOR_ESCAPE will be added, but C_WHITE - not
		int len2 = len;
		if (!NoColors) len2 += 2;			// place for S_WHITE
		if (used + len2 + 1 >= size)
		{
			// shrink len
			len2 = size - used - 1;
			if (!NoColors) len = len2 - 2;	// ...
		}
		if (len <= 0) return;
		// add text to buffer
		memcpy (buffer + used, str, len);
		used += len;
		// add S_WHITE to the end of string
		if (!NoColors)
		{
			buffer[used++] = COLOR_ESCAPE;
			buffer[used++] = '0' + C_WHITE;
		}
		// add trailing zero
		buffer[used] = 0;
	}
};
