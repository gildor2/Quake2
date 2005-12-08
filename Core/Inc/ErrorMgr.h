/*-----------------------------------------------------------------------------
	Error management
-----------------------------------------------------------------------------*/

//?? rename: not "handler"
class CErrorHandler
{
public:
	bool	swError;			// true when error was thrown by appError() call; will not dump CPU context
	bool	wasError;			// used for error history formatting
	TString<256>  Message;		// error message
	TString<2048> History;		// call history
	void	Reset ();
	// fields for non-fatal error
	bool	nonFatalError;		// when false, app can try to recover from error (and swError will be true)
};

CORE_API NORETURN void appFatalError (const char *fmt, ...) PRINTF(1,2);
#define appError	appFatalError
CORE_API NORETURN void appNonFatalError (const char *fmt, ...) PRINTF(1,2);


CORE_API void appUnwindPrefix (const char *fmt);		// not vararg (will display function name for unguardf only)
CORE_API NORETURN void appUnwindThrow (const char *fmt, ...) PRINTF(1,2);

CORE_API const char *appGetSystemErrorMessage (unsigned code);
CORE_API void appDisplayError ();


// global variables
CORE_API extern CErrorHandler	GErr;
CORE_API extern bool			GIsFatalError;			// should not be set manually; set by private appBeginError()
