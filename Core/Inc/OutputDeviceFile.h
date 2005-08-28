
class COutputDeviceFile : public COutputDevice
{
protected:
	FILE	*log;
public:
	COutputDeviceFile ()
	:	log (NULL)
	{}
	COutputDeviceFile (const char *fileName, bool erasePrevious = false)
	{
		log = fopen (fileName, erasePrevious ? "w" : "a");
		if (!log)
			appWPrintf ("Unable to open log file \"%s\"", fileName);
	}
	~COutputDeviceFile ()
	{
		fclose (log);
		log = NULL;
	}
	bool Write (const char *str)
	{
		if (!log || !str[0]) return true;
		// remove color codes
		char buf[4096];
		appUncolorizeString (buf, str);
		int len = strlen (buf);
		if (!len) return true;
		// write to file
		if (fwrite (buf, 1, len, log) < len)
		{
			appWPrintf ("Unable to write to log\n");
			fclose (log);
			log = NULL;
		}
		return true;
	}
};


class COutputDeviceStdout : public COutputDeviceFile
{
public:
	COutputDeviceStdout ()
	{
		log = stdout;
	}
};
