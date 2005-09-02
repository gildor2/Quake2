#include "Core.h"


/*?? TODO:
 *	- implement #include-like commands (needed? if so, should implement #define etc too)
 *	- implement text file cache (for includes ?)
 */


const char *CSimpleParser::GetLine ()
{
	while (true)
	{
		// skip leading spaces
		if (!(parseFlags & PARSER_KEEP_SPACES))
			while (textPtr[0] && textPtr[0] <= ' ') textPtr++;

		if (parseFlags & PARSER_SEPARATE_BRACES)
		{
			// process {} separately (allow few on a single line, without spaces between them)
			if (textPtr[0] == '{' || textPtr[0] == '}')
			{
				static char brace[2];
				brace[0] = *textPtr++;
				return brace;
			}
		}

		char *d = lineBuffer;
		char c = 0;
		while (d < lineBuffer + sizeof(lineBuffer) - 2)
		{
			c = *textPtr;
			if (!c) break;						// end of text
			textPtr++;
			if (c == '\r')						// ignore CR - used in MSDOS text format only, in conjunction with LF
				continue;
			if (c == '\n')						// end of line
				break;
			if (c == '\t')						// tab -> space
				c = ' ';
			*d++ = c;
		}
		*d++ = 0;
		// cut comments
		if (parseFlags & PARSER_CPP_COMMENTS)
		{
			// cut "//" comments
			for (d = lineBuffer; d[0]; d++)
				if (d[0] == '/' && d[1] == '/')	//?? later: not inside quotes
				{
					*d = 0;
					break;
				}
			//!! cut /* ... */ comments
		}
		// cut trailing spaces
		if (!(parseFlags & PARSER_KEEP_SPACES))
		{
			for (d = strchr (lineBuffer, 0) - 1; d >= lineBuffer; d--)
			{
				if (d[0] != ' ') break;
				d[0] = 0;
			}
		}

		//!! add line continuation "\"

		if (!c && !lineBuffer[0]) return NULL;	// end of text
		if (lineBuffer[0]) return lineBuffer;	// non-empty line
		// here: empty line, but text is not finished
		if (parseFlags & PARSER_KEEP_EMPTY_LINES)
			return "";
	}
}


const char *CSimpleParser::SkipBraces ()
{
	int braces = 0;
	const char *end = textPtr;
	while (const char *line = GetLine ())
	{
		if (line[0] == '{')
			braces++;
		else if (line[0] == '}')
		{
			if (!braces)
				return end;
			braces--;
		}
		end = textPtr;
	}
	return NULL;
}


/*-----------------------------------------------------------------------------
	Support for quoted strings
-----------------------------------------------------------------------------*/

//?? should be "const", but what to do with return type?
// will return NULL, when line unexpectedly finished
// will return pointer to next-after-closing-quote char otherwise
//?? rename to appSkipString(); process non-quoted strings; skip leading spaces
//?? CORE_API: in header only!!
char* appSkipQuotedString (char *str)
{
	char c = *str;
	if (c != '"') return str;					//?? not started with quotes ?!
	str++;

	while (true)
	{
		c = *str++;
		if (c == 0 || c == '\n') return NULL;	// unexpected end of line
		if (c == '"') return str;				// found
		if (c != '\\') continue;				// next char ...
		// C-like escape sequence (no oct/hex codes!)
		c = *str++;
		if (c == 0 || c == '\n') return NULL;	// char expected !
	}
}


bool appQuoteString (char *dst, int dstSize, const char *src)
{
	// place starting quote
	if (!dstSize--) return false;
	*dst++ = '\"';

	while (true)
	{
		char c = *src++;
		if (!c) break;
		if (dstSize < 2) return false;			// at least, no space for char + closing quote
		if (c == '\n')
		{
			*dst++ = '\\'; *dst++ = 'n';
			dstSize -= 2;
		}
		else if (c == '\t')
		{
			*dst++ = '\\'; *dst++ = 't';
			dstSize -= 2;
		}
		else if (c == '"')
		{
			*dst++ = '\\'; *dst++ = '"';
			dstSize -= 2;
		}
		else
		{
			*dst++ = c;
			dstSize--;
		}
	}

	// place closing quote
	if (!dstSize) return false;
	*dst = '\"';

	return true;
}


bool appUnquoteString (char *dst, int dstSize, const char *src)
{
	char c = *src++;
	if (c != '"') return false;					//?? not started with quotes ?!

	while (true)
	{
		c = *src++;
		if (c == 0 || c == '\n') return false;	// unexpected end of line
		if (!dstSize--) return false;			// no space
		if (c == '"')							// found closing quote
		{
			*dst = 0;
			return true;
		}
		if (c != '\\')
		{
			*dst++ = c;
			continue;
		}
		// C-like escape sequence (no oct/hex codes!)
		c = *src++;
		if (c == 0 || c == '\n') return false;	// char expected !
		if (c == 'n')
			*dst++ = '\n';
		else if (c == 't')
			*dst++ = '\t';
		else
			*dst++ = c;
	}
}
