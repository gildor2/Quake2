
class COutputDeviceStdout : public COutputDevice
{
public:
	void Write (const char *str)
	{
		//?? use fwrite()?
		printf ("%s", str);
	}
};
