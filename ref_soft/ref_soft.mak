# Microsoft Developer Studio Generated NMAKE File, Based on ref_soft.dsp
!IF "$(CFG)" == ""
CFG=ref_soft - Win32 Release
!MESSAGE No configuration specified. Defaulting to ref_soft - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "ref_soft - Win32 Release" && "$(CFG)" != "ref_soft - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ref_soft.mak" CFG="ref_soft - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ref_soft - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ref_soft - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "ref_soft - Win32 Release"

OUTDIR=.\release
INTDIR=.\release

ALL : "..\release\ref_soft.dll"


CLEAN :
	-@erase "$(INTDIR)\q_shared2.obj"
	-@erase "$(INTDIR)\r_aclip.obj"
	-@erase "$(INTDIR)\r_alias.obj"
	-@erase "$(INTDIR)\r_bsp.obj"
	-@erase "$(INTDIR)\r_draw.obj"
	-@erase "$(INTDIR)\r_edge.obj"
	-@erase "$(INTDIR)\r_image.obj"
	-@erase "$(INTDIR)\r_light.obj"
	-@erase "$(INTDIR)\r_main.obj"
	-@erase "$(INTDIR)\r_misc.obj"
	-@erase "$(INTDIR)\r_model.obj"
	-@erase "$(INTDIR)\r_part.obj"
	-@erase "$(INTDIR)\r_poly.obj"
	-@erase "$(INTDIR)\r_polyse.obj"
	-@erase "$(INTDIR)\r_rast.obj"
	-@erase "$(INTDIR)\r_scan.obj"
	-@erase "$(INTDIR)\r_sprite.obj"
	-@erase "$(INTDIR)\r_surf.obj"
	-@erase "$(INTDIR)\rw_ddraw.obj"
	-@erase "$(INTDIR)\rw_dib.obj"
	-@erase "$(INTDIR)\rw_imp.obj"
	-@erase "$(OUTDIR)\ref_soft.exp"
	-@erase "$(OUTDIR)\ref_soft.lib"
	-@erase "..\release\ref_soft.dll"
	-@erase "..\release\ref_soft.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ref_soft.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winmm.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\ref_soft.pdb" /map:"..\release\ref_soft.map" /machine:I386 /def:".\ref_soft.def" /out:"..\release/ref_soft.dll" /implib:"$(OUTDIR)\ref_soft.lib" /filealign:512 
LINK32_OBJS= \
	"$(INTDIR)\q_shared2.obj" \
	"$(INTDIR)\r_aclip.obj" \
	"$(INTDIR)\r_alias.obj" \
	"$(INTDIR)\r_bsp.obj" \
	"$(INTDIR)\r_draw.obj" \
	"$(INTDIR)\r_edge.obj" \
	"$(INTDIR)\r_image.obj" \
	"$(INTDIR)\r_light.obj" \
	"$(INTDIR)\r_main.obj" \
	"$(INTDIR)\r_misc.obj" \
	"$(INTDIR)\r_model.obj" \
	"$(INTDIR)\r_part.obj" \
	"$(INTDIR)\r_poly.obj" \
	"$(INTDIR)\r_polyse.obj" \
	"$(INTDIR)\r_rast.obj" \
	"$(INTDIR)\r_scan.obj" \
	"$(INTDIR)\r_sprite.obj" \
	"$(INTDIR)\r_surf.obj" \
	"$(INTDIR)\rw_ddraw.obj" \
	"$(INTDIR)\rw_dib.obj" \
	"$(INTDIR)\rw_imp.obj" \
	".\release\r_aclipa.obj" \
	".\release\r_draw16.obj" \
	".\release\r_drawa.obj" \
	".\release\r_edgea.obj" \
	".\release\r_polysa.obj" \
	".\release\r_scana.obj" \
	".\release\r_spr8.obj" \
	".\release\r_surf8.obj" \
	".\release\r_varsa.obj"

"..\release\ref_soft.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

OUTDIR=.\..\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\..\debug
# End Custom Macros

ALL : "$(OUTDIR)\ref_soft.dll" "$(OUTDIR)\ref_soft.bsc"


CLEAN :
	-@erase "$(INTDIR)\q_shared2.obj"
	-@erase "$(INTDIR)\q_shared2.sbr"
	-@erase "$(INTDIR)\r_aclip.obj"
	-@erase "$(INTDIR)\r_aclip.sbr"
	-@erase "$(INTDIR)\r_alias.obj"
	-@erase "$(INTDIR)\r_alias.sbr"
	-@erase "$(INTDIR)\r_bsp.obj"
	-@erase "$(INTDIR)\r_bsp.sbr"
	-@erase "$(INTDIR)\r_draw.obj"
	-@erase "$(INTDIR)\r_draw.sbr"
	-@erase "$(INTDIR)\r_edge.obj"
	-@erase "$(INTDIR)\r_edge.sbr"
	-@erase "$(INTDIR)\r_image.obj"
	-@erase "$(INTDIR)\r_image.sbr"
	-@erase "$(INTDIR)\r_light.obj"
	-@erase "$(INTDIR)\r_light.sbr"
	-@erase "$(INTDIR)\r_main.obj"
	-@erase "$(INTDIR)\r_main.sbr"
	-@erase "$(INTDIR)\r_misc.obj"
	-@erase "$(INTDIR)\r_misc.sbr"
	-@erase "$(INTDIR)\r_model.obj"
	-@erase "$(INTDIR)\r_model.sbr"
	-@erase "$(INTDIR)\r_part.obj"
	-@erase "$(INTDIR)\r_part.sbr"
	-@erase "$(INTDIR)\r_poly.obj"
	-@erase "$(INTDIR)\r_poly.sbr"
	-@erase "$(INTDIR)\r_polyse.obj"
	-@erase "$(INTDIR)\r_polyse.sbr"
	-@erase "$(INTDIR)\r_rast.obj"
	-@erase "$(INTDIR)\r_rast.sbr"
	-@erase "$(INTDIR)\r_scan.obj"
	-@erase "$(INTDIR)\r_scan.sbr"
	-@erase "$(INTDIR)\r_sprite.obj"
	-@erase "$(INTDIR)\r_sprite.sbr"
	-@erase "$(INTDIR)\r_surf.obj"
	-@erase "$(INTDIR)\r_surf.sbr"
	-@erase "$(INTDIR)\rw_ddraw.obj"
	-@erase "$(INTDIR)\rw_ddraw.sbr"
	-@erase "$(INTDIR)\rw_dib.obj"
	-@erase "$(INTDIR)\rw_dib.sbr"
	-@erase "$(INTDIR)\rw_imp.obj"
	-@erase "$(INTDIR)\rw_imp.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\ref_soft.bsc"
	-@erase "$(OUTDIR)\ref_soft.dll"
	-@erase "$(OUTDIR)\ref_soft.exp"
	-@erase "$(OUTDIR)\ref_soft.lib"
	-@erase "$(OUTDIR)\ref_soft.map"
	-@erase "$(OUTDIR)\ref_soft.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ref_soft.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\q_shared2.sbr" \
	"$(INTDIR)\r_aclip.sbr" \
	"$(INTDIR)\r_alias.sbr" \
	"$(INTDIR)\r_bsp.sbr" \
	"$(INTDIR)\r_draw.sbr" \
	"$(INTDIR)\r_edge.sbr" \
	"$(INTDIR)\r_image.sbr" \
	"$(INTDIR)\r_light.sbr" \
	"$(INTDIR)\r_main.sbr" \
	"$(INTDIR)\r_misc.sbr" \
	"$(INTDIR)\r_model.sbr" \
	"$(INTDIR)\r_part.sbr" \
	"$(INTDIR)\r_poly.sbr" \
	"$(INTDIR)\r_polyse.sbr" \
	"$(INTDIR)\r_rast.sbr" \
	"$(INTDIR)\r_scan.sbr" \
	"$(INTDIR)\r_sprite.sbr" \
	"$(INTDIR)\r_surf.sbr" \
	"$(INTDIR)\rw_ddraw.sbr" \
	"$(INTDIR)\rw_dib.sbr" \
	"$(INTDIR)\rw_imp.sbr"

"$(OUTDIR)\ref_soft.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winmm.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\ref_soft.pdb" /map:"..\debug/ref_soft.map" /debug /machine:I386 /nodefaultlib:"libc" /def:".\ref_soft.def" /out:"$(OUTDIR)\ref_soft.dll" /implib:"$(OUTDIR)\ref_soft.lib" 
DEF_FILE= \
	".\ref_soft.def"
LINK32_OBJS= \
	"$(INTDIR)\q_shared2.obj" \
	"$(INTDIR)\r_aclip.obj" \
	"$(INTDIR)\r_alias.obj" \
	"$(INTDIR)\r_bsp.obj" \
	"$(INTDIR)\r_draw.obj" \
	"$(INTDIR)\r_edge.obj" \
	"$(INTDIR)\r_image.obj" \
	"$(INTDIR)\r_light.obj" \
	"$(INTDIR)\r_main.obj" \
	"$(INTDIR)\r_misc.obj" \
	"$(INTDIR)\r_model.obj" \
	"$(INTDIR)\r_part.obj" \
	"$(INTDIR)\r_poly.obj" \
	"$(INTDIR)\r_polyse.obj" \
	"$(INTDIR)\r_rast.obj" \
	"$(INTDIR)\r_scan.obj" \
	"$(INTDIR)\r_sprite.obj" \
	"$(INTDIR)\r_surf.obj" \
	"$(INTDIR)\rw_ddraw.obj" \
	"$(INTDIR)\rw_dib.obj" \
	"$(INTDIR)\rw_imp.obj" \
	"..\debug\r_aclipa.obj" \
	"..\debug\r_draw16.obj" \
	"..\debug\r_drawa.obj" \
	"..\debug\r_edgea.obj" \
	"..\debug\r_polysa.obj" \
	"..\debug\r_scana.obj" \
	"..\debug\r_spr8.obj" \
	"..\debug\r_surf8.obj" \
	"..\debug\r_varsa.obj"

"$(OUTDIR)\ref_soft.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("ref_soft.dep")
!INCLUDE "ref_soft.dep"
!ELSE 
!MESSAGE Warning: cannot find "ref_soft.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "ref_soft - Win32 Release" || "$(CFG)" == "ref_soft - Win32 Debug"
SOURCE=..\qcommon\q_shared2.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\q_shared2.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\q_shared2.obj"	"$(INTDIR)\q_shared2.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_aclip.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_aclip.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_aclip.obj"	"$(INTDIR)\r_aclip.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_aclipa.asm

!IF  "$(CFG)" == "ref_soft - Win32 Release"

OutDir=.\release
InputPath=.\r_aclipa.asm
InputName=r_aclipa

".\release\r_aclipa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

OutDir=.\..\debug
InputPath=.\r_aclipa.asm
InputName=r_aclipa

"..\debug\r_aclipa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\r_alias.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_alias.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_alias.obj"	"$(INTDIR)\r_alias.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_bsp.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_bsp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_bsp.obj"	"$(INTDIR)\r_bsp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_draw.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_draw.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_draw.obj"	"$(INTDIR)\r_draw.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_draw16.asm

!IF  "$(CFG)" == "ref_soft - Win32 Release"

OutDir=.\release
InputPath=.\r_draw16.asm
InputName=r_draw16

".\release\r_draw16.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

OutDir=.\..\debug
InputPath=.\r_draw16.asm
InputName=r_draw16

"..\debug\r_draw16.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\r_drawa.asm

!IF  "$(CFG)" == "ref_soft - Win32 Release"

OutDir=.\release
InputPath=.\r_drawa.asm
InputName=r_drawa

".\release\r_drawa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

OutDir=.\..\debug
InputPath=.\r_drawa.asm
InputName=r_drawa

"..\debug\r_drawa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\r_edge.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_edge.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_edge.obj"	"$(INTDIR)\r_edge.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_edgea.asm

!IF  "$(CFG)" == "ref_soft - Win32 Release"

OutDir=.\release
InputPath=.\r_edgea.asm
InputName=r_edgea

".\release\r_edgea.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

OutDir=.\..\debug
InputPath=.\r_edgea.asm
InputName=r_edgea

"..\debug\r_edgea.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\r_image.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_image.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_image.obj"	"$(INTDIR)\r_image.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_light.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_light.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_light.obj"	"$(INTDIR)\r_light.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_main.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_main.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_main.obj"	"$(INTDIR)\r_main.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_misc.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_misc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_misc.obj"	"$(INTDIR)\r_misc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_model.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_model.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_model.obj"	"$(INTDIR)\r_model.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_part.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_part.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_part.obj"	"$(INTDIR)\r_part.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_poly.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_poly.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_poly.obj"	"$(INTDIR)\r_poly.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_polysa.asm

!IF  "$(CFG)" == "ref_soft - Win32 Release"

OutDir=.\release
InputPath=.\r_polysa.asm
InputName=r_polysa

".\release\r_polysa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

OutDir=.\..\debug
InputPath=.\r_polysa.asm
InputName=r_polysa

"..\debug\r_polysa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\r_polyse.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_polyse.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_polyse.obj"	"$(INTDIR)\r_polyse.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_rast.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_rast.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_rast.obj"	"$(INTDIR)\r_rast.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_scan.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_scan.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_scan.obj"	"$(INTDIR)\r_scan.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_scana.asm

!IF  "$(CFG)" == "ref_soft - Win32 Release"

OutDir=.\release
InputPath=.\r_scana.asm
InputName=r_scana

".\release\r_scana.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

OutDir=.\..\debug
InputPath=.\r_scana.asm
InputName=r_scana

"..\debug\r_scana.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\r_spr8.asm

!IF  "$(CFG)" == "ref_soft - Win32 Release"

OutDir=.\release
InputPath=.\r_spr8.asm
InputName=r_spr8

".\release\r_spr8.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

OutDir=.\..\debug
InputPath=.\r_spr8.asm
InputName=r_spr8

"..\debug\r_spr8.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\r_sprite.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_sprite.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_sprite.obj"	"$(INTDIR)\r_sprite.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_surf.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\r_surf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_surf.obj"	"$(INTDIR)\r_surf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\r_surf8.asm

!IF  "$(CFG)" == "ref_soft - Win32 Release"

OutDir=.\release
InputPath=.\r_surf8.asm
InputName=r_surf8

".\release\r_surf8.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

OutDir=.\..\debug
InputPath=.\r_surf8.asm
InputName=r_surf8

"..\debug\r_surf8.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\r_varsa.asm

!IF  "$(CFG)" == "ref_soft - Win32 Release"

OutDir=.\release
InputPath=.\r_varsa.asm
InputName=r_varsa

".\release\r_varsa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

OutDir=.\..\debug
InputPath=.\r_varsa.asm
InputName=r_varsa

"..\debug\r_varsa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=..\win32\rw_ddraw.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\rw_ddraw.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\rw_ddraw.obj"	"$(INTDIR)\rw_ddraw.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\win32\rw_dib.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\rw_dib.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\rw_dib.obj"	"$(INTDIR)\rw_dib.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\win32\rw_imp.c

!IF  "$(CFG)" == "ref_soft - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DYNAMIC_REF" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\rw_imp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ref_soft - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ref_soft.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\rw_imp.obj"	"$(INTDIR)\rw_imp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

