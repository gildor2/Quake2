
class COutputDeviceStdout : public COutputDevice
{
public:
	void Write (const char *str)
	{
		printf ("%s", str);
	}
};
