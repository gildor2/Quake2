#include "qcommon.h"


/*-----------------------------------------------------------------------------
	Simple text parser
	?? change, use another etc
	?? move to Core (note: not using any Quake2 stuff)
-----------------------------------------------------------------------------*/

static const char *parserText;

void SetupTextParser (const char *text)
{
	parserText = text;
}

const char *GetParserPos ()
{
	return parserText;
}


const char *GetLine ()
{
	static char line[1024];

	char *d = line;
	char c = 0;
	while (d < line + sizeof(line) - 2)
	{
		c = *parserText;
		if (!c) break;						// end of text
		parserText++;
		if (c == '\r' || c == '\n')			// end of line
			break;
		if (c == '\t') c = ' ';				// tab -> space
		*d++ = c;
	}
	*d++ = 0;
	// cut "//" comments
	for (d = line; d[0]; d++)
		if (d[0] == '/' && d[1] == '/')		//?? later: not inside quotes
		{
			*d = 0;
			break;
		}
	// cut trailing spaces
	for (d = strchr (line, 0) - 1; d >= line; d--)
	{
		if (d[0] != ' ') break;
		d[0] = 0;
	}
	// skip leading spaces
	d = line;
	while (d[0] == ' ') d++;

	return (c || d[0]) ? d : NULL;
}

// returns non-empty line or NULL
const char *GetScriptLine ()
{
	// skip leading spaces
	while (parserText[0] && parserText[0] <= ' ') parserText++;
	// process {} separately (allow few on a single line, without spaces between them)
	if (parserText[0] == '{' || parserText[0] == '}')
	{
		static char brace[2];
		brace[0] = *parserText++;
		return brace;
	}

	const char *line;
	while (line = GetLine ())
	{
		if (line[0]) break;
	}
	//?? cut comments and trailing spaces here?
	return line;
}
