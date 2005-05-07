// print
void Com_Printf (char *fmt, ...);
void Com_DPrintf (char *fmt, ...);
void Com_WPrintf (char *fmt, ...);
void Com_Error (int code, char *fmt, ...);
#ifdef _WIN32
int CreateWindow (int w, int h, bool fullscreen = false);
#endif
bool Shutdown (void);
void Test (int a, char* b);
void *Alloc (int size);
