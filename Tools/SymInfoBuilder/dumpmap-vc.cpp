#include <stdio.h>
#include <windows.h>
#include <io.h>

#if 0
// use dbghelp
#	include <dbghelp.h>
#	pragma comment (lib, "dbghelp")
#else
// use imagehlp
#	include <imagehlp.h>
#	pragma comment (lib, "imagehlp")
#endif

//#define UND_FLAGS	(UNDNAME_COMPLETE)
#define UND_FLAGS	(UNDNAME_NO_LEADING_UNDERSCORES|UNDNAME_NO_LEADING_UNDERSCORES|UNDNAME_NO_ALLOCATION_LANGUAGE|UNDNAME_NO_ACCESS_SPECIFIERS)
//#define MAP_PATH	"../../Release/"		// test version; do not define to allow command-line


//#define DEBUG		1						// use to show unprocessed names


void Abort (const char *msg)
{
	fprintf (stderr, "ERROR: %s\n", msg);
	exit (1);
}


void ProcessMapFile (const char *name)
{
	FILE	*f;
	char	line[1024];
	unsigned baseAddr;

	fprintf (stderr, "Processing %s ...", name);
	if (!(f = fopen (name, "r"))) Abort ("\ncannot open file");

	// get base address
	while (true)
	{
		if (feof (f)) Abort ("unexpected EOF");
		fgets (line, sizeof(line), f);
		if (sscanf (line, " Preferred load address is %08X", &baseAddr) == 1)
			break;
	}
	fprintf (stderr, "\n");
//	fprintf (stderr, "base address is %08X\n", baseAddr);

	// get symbol info
	while (!feof (f))
	{
		unsigned seg, ofs, virtAddr;
		char decorated[1024], undecorated[1024];

		fgets (line, sizeof(line), f);
		if (sscanf (line, " %04X:%08X %s %08X", &seg, &ofs, &decorated, &virtAddr) != 4) continue;
		UnDecorateSymbolName (decorated, undecorated, sizeof(undecorated), UND_FLAGS);
		// process special VC class::`name'
		char *ptype = strchr (undecorated, '`');
		if ((ptype - undecorated >= 3) && (ptype[-1] == ':') && (ptype[-2] == ':')) // react on "::`" only
		{
			// split class::`name' -> name + type
			char name[1024], type[1024];
			// get 'type'
			strcpy (type, ptype+1);
			char *s = strchr (type, '\'');
			if (s) *s = 0;
			// get 'name'
			int len = ptype - undecorated - 2;
			memcpy (name, undecorated, len);
			name[len] = 0;
			if (!memcmp (name, "const ", 6))			// process "const ClassName::`vtable'"
				strcpy (name, name+6);
			// process type
			if (!strcmp (type, "scalar deleting destructor"))
				sprintf (undecorated, "%s::sdestruc", name);
			else if (!strcmp (type, "vector deleting destructor"))
				sprintf (undecorated, "%s::vdestruc", name);
			else if (!strcmp (type, "vftable"))
				sprintf (undecorated, "vmt(%s)", name);
			else
			{
#if DEBUG
				fprintf (stderr, "?? %s\n", undecorated);
#endif
				continue;
			}
		}
		// refine output names
		if (!memcmp (undecorated, "__real@", 7))		// unnamed constant
			continue;
		if (strpbrk (undecorated, "?$`'\\"))
		{
#if DEBUG
			fprintf (stderr, "-> %s\n", undecorated);
#endif
			continue;
		}
//		if (undecorated[0] == '_' && undecorated[1] == '_') continue;
//		if (!__iscsym (undecorated[0])) continue;

		printf ("%08X   %s\n", virtAddr - baseAddr, undecorated);
	}

	fclose (f);
}


void main (int argc, const char *argv[])
{
#ifdef MAP_PATH
	int		hFind;
	_finddata_t found;

	hFind = _findfirst (MAP_PATH "*.map", &found);
	if (hFind != -1)
	{
		do
		{
			char	fullName[256];

			sprintf (fullName, MAP_PATH "%s", found.name);
			ProcessMapFile (fullName);
		} while (_findnext (hFind, &found) != -1);
		_findclose (hFind);
	}
#else
	if (argc != 2)
	{
		fprintf (stderr, "Usage: dumpmap <map_file>\n");
		exit (1);
	}
	ProcessMapFile (argv[1]);
#endif
}
