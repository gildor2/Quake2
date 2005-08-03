
// flags:
#define PARSER_CPP_COMMENTS			1		// cut "//..." comments
#define PARSER_KEEP_EMPTY_LINES		2		// GetLine() can get line == ""
#define PARSER_KEEP_SPACES			4		// do not remove traling/leading spaces
#define PARSER_SEPARATE_BRACES		8		// line "{{text ..." will be "{"+"{"+"text..."
//?? PARSER_CPP_ESCAPE_CHARS				// treat "\x" as control characters

class CORE_API CSimpleParser
{
private:
	char	lineBuffer[1024];
	const char *textPtr;
	unsigned parseFlags;
public:
	//?? add InitFromBuf (text, textLen, flags)
	inline void InitFromBuf (const char *buf, unsigned flags = PARSER_CPP_COMMENTS)
	{
		textPtr = buf;
		parseFlags = flags;
	}
	inline const char *GetPos ()
	{
		return textPtr;
	}
	const char *GetLine ();
	// assume one "{" entered; return NULL when no matching "}" (unexpected end of buffer),
	// or pointer to a line with matched "}"
	// NOTE: textPtr will point to line AFTER matched "}"
	const char *SkipBraces ();
};


CORE_API char* appSkipQuotedString (char *str);
// return false when not enough dstSize
CORE_API bool appQuoteString (char *dst, int dstSize, const char *src);
// return false when not enough dstSize or when no closing quote
CORE_API bool appUnquoteString (char *dst, int dstSize, const char *src);
