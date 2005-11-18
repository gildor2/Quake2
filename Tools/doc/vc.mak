#----- generic VC6 makefile -----

# what to build
ALL : "../release/build.exe" (exe, dll, lib)

OUTDIR=.\Release
INTDIR=.\Release

#!! don't know:
!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE
NULL=nul
!ENDIF


# std defines:
#   NDEBUG - !debug version; _DEBUG - debug version; WIN32, _WINDOWS - always; _LIB - when making library (.lib)
# add std libraries

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "./Inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D CORE_API=DLL_EXPORT /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c
# /nologo
# /MD - use msvcrt.dll
# /W3 - warning level
# /GX - enable exception handling
# /O1 - minimize size, /O2 - maximize speed
# /Ob0 - no inlines (default), /Ob1 - expand inlines only (default for /O1, /O2), /Ob2 - expand inlines and other suitable functions
# /Fo"dir for obj"
# /Fd"dir for pdb"
# /c  - do not link (compile only)
# /I "dir" - include dir
# /D define /D define=value
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib /nologo /base:"0x8000000" /dll /pdb:none /map:"../Release/Core.map" /machine:I386 /out:"../Release/Core.dll" /implib:"../Release/Core.lib" /filealign:512
# /nologo
# /base:0xXXXXXX - base address for dll (or exe?)
# /pdb:none or /pdb:filename.pdb
# /map:filename
# /machine:I386
# /out:filename.dll/exe/lib
# /filealign:512 - for exe/dll, reduce file size
# /subsystem:console - for console exe
# /incremental:no
# /dll or /lib - for dll or lib
# /implib:filename.lib - import library for dll
# for EXE:
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib ../Release/Core.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\TestApp.pdb" /map:"../Release/TestApp.map" /machine:I386 /out:"../Release/TestApp.exe" /filealign:512
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"lib.lib"  #!! name should not be here

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

# will create outdir
"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"


LINK32_OBJS= \
	"$(INTDIR)\Commands.obj" ...
# or:
LINK32_OBJS= \
	"$(INTDIR)\TestApp.obj" \
	"..\Release\Core.lib"


# main target
# DLL:
"..\Release\Core.dll" : "$(OUTDIR)" $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# LIB:
".\lib.lib" : "$(OUTDIR)" $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(LIB32_OBJS)
<<

# EXE:
"..\Debug\TestApp.exe" : "$(OUTDIR)" $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<



# dependencies here ...........
.\TestApp.cpp : \
	"..\Core\Inc\Build.h"\
	"..\Core\Inc\Commands.h"\
	"..\Core\Inc\Core.h"


# building separate files

SOURCE=.\Src\Memory.cpp

"$(INTDIR)\Memory.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)
