# Microsoft Developer Studio Generated NMAKE File, Based on ref_gl.dsp
!IF "$(CFG)" == ""
CFG=ref_gl - Win32 Release
!MESSAGE No configuration specified. Defaulting to ref_gl - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "ref_gl - Win32 Release" && "$(CFG)" != "ref_gl - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ref_gl.mak" CFG="ref_gl - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ref_gl - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ref_gl - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "ref_gl - Win32 Release"

OUTDIR=.\release
INTDIR=.\release

!IF "$(RECURSE)" == "0" 

ALL : "..\release\ref_oldgl.dll"

!ELSE 

ALL : "quake2s - Win32 Release" "..\release\ref_oldgl.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"quake2s - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\gl_draw.obj"
	-@erase "$(INTDIR)\gl_image.obj"
	-@erase "$(INTDIR)\gl_light.obj"
	-@erase "$(INTDIR)\gl_mesh.obj"
	-@erase "$(INTDIR)\gl_model.obj"
	-@erase "$(INTDIR)\gl_rmain.obj"
	-@erase "$(INTDIR)\gl_rmisc.obj"
	-@erase "$(INTDIR)\gl_rsurf.obj"
	-@erase "$(INTDIR)\gl_textout.obj"
	-@erase "$(INTDIR)\gl_warp.obj"
	-@erase "$(INTDIR)\glw_imp.obj"
	-@erase "$(INTDIR)\q_shared2.obj"
	-@erase "$(INTDIR)\qgl_win.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\ref_oldgl.exp"
	-@erase "..\release\ref_oldgl.dll"
	-@erase "..\release\ref_oldgl.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ref_gl.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winmm.lib /nologo /subsystem:windows /dll /pdb:none /map:"..\release\ref_oldgl.map" /machine:I386 /out:"..\release/ref_oldgl.dll" /implib:"$(OUTDIR)\ref_oldgl.lib" /filealign:512 
LINK32_OBJS= \
	"$(INTDIR)\gl_draw.obj" \
	"$(INTDIR)\gl_image.obj" \
	"$(INTDIR)\gl_light.obj" \
	"$(INTDIR)\gl_mesh.obj" \
	"$(INTDIR)\gl_model.obj" \
	"$(INTDIR)\gl_rmain.obj" \
	"$(INTDIR)\gl_rmisc.obj" \
	"$(INTDIR)\gl_rsurf.obj" \
	"$(INTDIR)\gl_textout.obj" \
	"$(INTDIR)\gl_warp.obj" \
	"$(INTDIR)\glw_imp.obj" \
	"$(INTDIR)\q_shared2.obj" \
	"$(INTDIR)\qgl_win.obj"

"..\release\ref_oldgl.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

OUTDIR=.\..\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\..\debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ref_gl.dll" "$(OUTDIR)\ref_gl.bsc"

!ELSE 

ALL : "quake2s - Win32 Debug" "$(OUTDIR)\ref_gl.dll" "$(OUTDIR)\ref_gl.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"quake2s - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\gl_draw.obj"
	-@erase "$(INTDIR)\gl_draw.sbr"
	-@erase "$(INTDIR)\gl_image.obj"
	-@erase "$(INTDIR)\gl_image.sbr"
	-@erase "$(INTDIR)\gl_light.obj"
	-@erase "$(INTDIR)\gl_light.sbr"
	-@erase "$(INTDIR)\gl_mesh.obj"
	-@erase "$(INTDIR)\gl_mesh.sbr"
	-@erase "$(INTDIR)\gl_model.obj"
	-@erase "$(INTDIR)\gl_model.sbr"
	-@erase "$(INTDIR)\gl_rmain.obj"
	-@erase "$(INTDIR)\gl_rmain.sbr"
	-@erase "$(INTDIR)\gl_rmisc.obj"
	-@erase "$(INTDIR)\gl_rmisc.sbr"
	-@erase "$(INTDIR)\gl_rsurf.obj"
	-@erase "$(INTDIR)\gl_rsurf.sbr"
	-@erase "$(INTDIR)\gl_textout.obj"
	-@erase "$(INTDIR)\gl_textout.sbr"
	-@erase "$(INTDIR)\gl_warp.obj"
	-@erase "$(INTDIR)\gl_warp.sbr"
	-@erase "$(INTDIR)\glw_imp.obj"
	-@erase "$(INTDIR)\glw_imp.sbr"
	-@erase "$(INTDIR)\q_shared2.obj"
	-@erase "$(INTDIR)\q_shared2.sbr"
	-@erase "$(INTDIR)\qgl_win.obj"
	-@erase "$(INTDIR)\qgl_win.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\ref_gl.bsc"
	-@erase "$(OUTDIR)\ref_gl.dll"
	-@erase "$(OUTDIR)\ref_gl.exp"
	-@erase "$(OUTDIR)\ref_gl.map"
	-@erase "$(OUTDIR)\ref_gl.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_gl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ref_gl.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\gl_draw.sbr" \
	"$(INTDIR)\gl_image.sbr" \
	"$(INTDIR)\gl_light.sbr" \
	"$(INTDIR)\gl_mesh.sbr" \
	"$(INTDIR)\gl_model.sbr" \
	"$(INTDIR)\gl_rmain.sbr" \
	"$(INTDIR)\gl_rmisc.sbr" \
	"$(INTDIR)\gl_rsurf.sbr" \
	"$(INTDIR)\gl_textout.sbr" \
	"$(INTDIR)\gl_warp.sbr" \
	"$(INTDIR)\glw_imp.sbr" \
	"$(INTDIR)\q_shared2.sbr" \
	"$(INTDIR)\qgl_win.sbr"

"$(OUTDIR)\ref_gl.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winmm.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\ref_gl.pdb" /map:"..\debug\ref_gl.map" /debug /machine:I386 /out:"$(OUTDIR)\ref_gl.dll" /implib:"$(OUTDIR)\ref_gl.lib" 
LINK32_OBJS= \
	"$(INTDIR)\gl_draw.obj" \
	"$(INTDIR)\gl_image.obj" \
	"$(INTDIR)\gl_light.obj" \
	"$(INTDIR)\gl_mesh.obj" \
	"$(INTDIR)\gl_model.obj" \
	"$(INTDIR)\gl_rmain.obj" \
	"$(INTDIR)\gl_rmisc.obj" \
	"$(INTDIR)\gl_rsurf.obj" \
	"$(INTDIR)\gl_textout.obj" \
	"$(INTDIR)\gl_warp.obj" \
	"$(INTDIR)\glw_imp.obj" \
	"$(INTDIR)\q_shared2.obj" \
	"$(INTDIR)\qgl_win.obj"

"$(OUTDIR)\ref_gl.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("ref_gl.dep")
!INCLUDE "ref_gl.dep"
!ELSE 
!MESSAGE Warning: cannot find "ref_gl.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "ref_gl - Win32 Release" || "$(CFG)" == "ref_gl - Win32 Debug"
SOURCE=.\gl_draw.c

!IF  "$(CFG)" == "ref_gl - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "DYNAMIC_REF" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_draw.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_gl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_draw.obj"	"$(INTDIR)\gl_draw.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\gl_image.c

!IF  "$(CFG)" == "ref_gl - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "DYNAMIC_REF" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_image.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_gl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_image.obj"	"$(INTDIR)\gl_image.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\gl_light.c

!IF  "$(CFG)" == "ref_gl - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "DYNAMIC_REF" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_light.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_gl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_light.obj"	"$(INTDIR)\gl_light.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\gl_mesh.c

!IF  "$(CFG)" == "ref_gl - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "DYNAMIC_REF" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_mesh.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_gl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_mesh.obj"	"$(INTDIR)\gl_mesh.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\gl_model.c

!IF  "$(CFG)" == "ref_gl - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "DYNAMIC_REF" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_model.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_gl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_model.obj"	"$(INTDIR)\gl_model.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\gl_rmain.c

!IF  "$(CFG)" == "ref_gl - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "DYNAMIC_REF" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_rmain.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_gl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_rmain.obj"	"$(INTDIR)\gl_rmain.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\gl_rmisc.c

!IF  "$(CFG)" == "ref_gl - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "DYNAMIC_REF" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_rmisc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_gl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_rmisc.obj"	"$(INTDIR)\gl_rmisc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\gl_rsurf.c

!IF  "$(CFG)" == "ref_gl - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "DYNAMIC_REF" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_rsurf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_gl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_rsurf.obj"	"$(INTDIR)\gl_rsurf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\gl_textout.c

!IF  "$(CFG)" == "ref_gl - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "DYNAMIC_REF" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_textout.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_gl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_textout.obj"	"$(INTDIR)\gl_textout.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\gl_warp.c

!IF  "$(CFG)" == "ref_gl - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "DYNAMIC_REF" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_warp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_gl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gl_warp.obj"	"$(INTDIR)\gl_warp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=glw_imp.c

!IF  "$(CFG)" == "ref_gl - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "DYNAMIC_REF" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\glw_imp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_gl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\glw_imp.obj"	"$(INTDIR)\glw_imp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\qcommon\q_shared2.c

!IF  "$(CFG)" == "ref_gl - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "DYNAMIC_REF" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\q_shared2.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_gl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\q_shared2.obj"	"$(INTDIR)\q_shared2.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\qgl_win.c

!IF  "$(CFG)" == "ref_gl - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "DYNAMIC_REF" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\qgl_win.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_gl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\qgl_win.obj"	"$(INTDIR)\qgl_win.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

!IF  "$(CFG)" == "ref_gl - Win32 Release"

"quake2s - Win32 Release" : 
   cd "\7"
   $(MAKE) /$(MAKEFLAGS) /F .\quake2s.mak CFG="quake2s - Win32 Release" 
   cd ".\ref_gl.old"

"quake2s - Win32 ReleaseCLEAN" : 
   cd "\7"
   $(MAKE) /$(MAKEFLAGS) /F .\quake2s.mak CFG="quake2s - Win32 Release" RECURSE=1 CLEAN 
   cd ".\ref_gl.old"

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

"quake2s - Win32 Debug" : 
   cd "\7"
   $(MAKE) /$(MAKEFLAGS) /F .\quake2s.mak CFG="quake2s - Win32 Debug" 
   cd ".\ref_gl.old"

"quake2s - Win32 DebugCLEAN" : 
   cd "\7"
   $(MAKE) /$(MAKEFLAGS) /F .\quake2s.mak CFG="quake2s - Win32 Debug" RECURSE=1 CLEAN 
   cd ".\ref_gl.old"

!ENDIF 


!ENDIF 

