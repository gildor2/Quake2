# Microsoft Developer Studio Project File - Name="ref_gl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ref_gl - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ref_gl.mak".
!MESSAGE 
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

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ref_gl - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\ref_gl__"
# PROP BASE Intermediate_Dir ".\ref_gl__"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\release"
# PROP Intermediate_Dir ".\release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir "."
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /MD /W3 /GX /O1 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib /nologo /subsystem:windows /dll /pdb:none /map:"..\release\ref_oldgl.map" /machine:I386 /out:"..\release/ref_oldgl.dll" /filealign:512

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\ref_gl__"
# PROP BASE Intermediate_Dir ".\ref_gl__"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\debug"
# PROP Intermediate_Dir ".\debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir "."
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /MDd /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib /nologo /subsystem:windows /dll /incremental:no /map:"..\debug\ref_gl.map" /debug /machine:I386
# SUBTRACT LINK32 /profile

!ENDIF 

# Begin Target

# Name "ref_gl - Win32 Release"
# Name "ref_gl - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\gl_draw.cpp

!IF  "$(CFG)" == "ref_gl - Win32 Release"

# ADD CPP /D "DYNAMIC_REF"

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_image.cpp

!IF  "$(CFG)" == "ref_gl - Win32 Release"

# ADD CPP /D "DYNAMIC_REF"

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_light.cpp

!IF  "$(CFG)" == "ref_gl - Win32 Release"

# ADD CPP /D "DYNAMIC_REF"

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_mesh.cpp

!IF  "$(CFG)" == "ref_gl - Win32 Release"

# ADD CPP /D "DYNAMIC_REF"

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_model.cpp

!IF  "$(CFG)" == "ref_gl - Win32 Release"

# ADD CPP /D "DYNAMIC_REF"

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_rmain.cpp

!IF  "$(CFG)" == "ref_gl - Win32 Release"

# ADD CPP /D "DYNAMIC_REF"

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_rmisc.cpp

!IF  "$(CFG)" == "ref_gl - Win32 Release"

# ADD CPP /D "DYNAMIC_REF"

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_rsurf.cpp

!IF  "$(CFG)" == "ref_gl - Win32 Release"

# ADD CPP /D "DYNAMIC_REF"

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_textout.cpp

!IF  "$(CFG)" == "ref_gl - Win32 Release"

# ADD CPP /D "DYNAMIC_REF"

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_warp.cpp

!IF  "$(CFG)" == "ref_gl - Win32 Release"

# ADD CPP /D "DYNAMIC_REF"

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=glw_imp.cpp

!IF  "$(CFG)" == "ref_gl - Win32 Release"

# ADD CPP /D "DYNAMIC_REF"

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\qcommon\q_shared2.cpp

!IF  "$(CFG)" == "ref_gl - Win32 Release"

# ADD CPP /D "DYNAMIC_REF"

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\qgl_win.cpp

!IF  "$(CFG)" == "ref_gl - Win32 Release"

# ADD CPP /D "DYNAMIC_REF"

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\ref_vars.cpp

!IF  "$(CFG)" == "ref_gl - Win32 Release"

# ADD CPP /D "DYNAMIC_REF"

!ELSEIF  "$(CFG)" == "ref_gl - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\anorms.h
# End Source File
# Begin Source File

SOURCE=.\anormtab.h
# End Source File
# Begin Source File

SOURCE=.\gl_local.h
# End Source File
# Begin Source File

SOURCE=.\gl_model.h
# End Source File
# Begin Source File

SOURCE=..\win32\glw_win.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\q_shared2.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\qcommon.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\qfiles.h
# End Source File
# Begin Source File

SOURCE=.\qgl_decl.h
# End Source File
# Begin Source File

SOURCE=.\qgl_impl.h
# End Source File
# Begin Source File

SOURCE=..\client\ref.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl.h
# End Source File
# Begin Source File

SOURCE=..\win32\winquake.h
# End Source File
# End Group
# End Target
# End Project
