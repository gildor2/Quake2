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
	inline const char *GetText ()
	{
		return buffer;
	}
	void Write (const char *str)
	{
		int len = strlen (str);
		if (used + len + 1 >= size - 2)	// 2 is place for S_WHITE
		{
			// shrink len
			len = size - 2 - used - 1;
		}
		if (len > 0)
		{
			// add text to buffer
			memcpy (buffer + used, str, len);
			used += len;
			buffer[used++] = COLOR_ESCAPE;
			buffer[used++] = '0' + C_WHITE;
			buffer[used] = 0;
		}
	}
};
