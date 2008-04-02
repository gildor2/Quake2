
//!! NOTE: this is a superset of COutputDeviceMem, but with "scrolling" buffer
//!!		instead of simple clamping
class COutputDeviceMemTail : public COutputDevice
{
protected:
	char	*buffer;
	int		used;
	int		size;
public:
	COutputDeviceMemTail(int bufSize = 4096)
	:	size(bufSize)
	,	used(0)
	{
		buffer = new char [bufSize];
	}
	~COutputDeviceMemTail()
	{
		Unregister();
		delete buffer;
	}
	inline const char *GetText() const
	{
		return buffer;
	}
	const char *GetTail(int numLines)
	{
		const char *s = buffer+used;
		while (s > buffer)
		{
			if (*--s == '\n')
			{
				if (numLines-- <= 0)
					return s+1;
			}
		}
		return buffer;
	}
	virtual void Write(const char *str)
	{
		int len = strlen(str);
		if (len > size - 1)
		{
			// incoming "str" is too long - fill buffer with it ...
			const char *limit = str + len - (size - 2);
			const char *s     = str;
			// ... discretely by lines
			while (s < limit)
			{
				s = strchr(s, '\n');
				if (!s)
				{
					// single line exceeds buffer size - empty buffer and exit
					used      = 0;
					buffer[0] = 0;
					return;
				}
				s++;				// skip '\n'
			}
			used = len - (s - str);
			memcpy(buffer, s, used+1);
		}
		else if (used + len > size - 1)
		{
			// should cut previous text
			const char *limit = buffer + (used + len - size) + 2;
			const char *s     = buffer;
			while (s < limit)
			{
				s = strchr(s, '\n');
				if (!s)
				{
					// it's too few space in buffer to keep previous text's last line and "str"
					used = len;
					memcpy(buffer, str, used+1);
					return;
				}
				s++;				// skip '\n'
			}
			int move = used - (s - buffer);
			memcpy(buffer, s, move);
			memcpy(buffer+move, str, len+1);
			used = move + len;
		}
		else
		{
			// enough space in buffer for previous text and str
			memcpy(buffer+used, str, len+1);
			used += len;
		}
	}
};
