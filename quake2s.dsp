# Microsoft Developer Studio Project File - Name="quake2s" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=quake2s - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "quake2s.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "quake2s.mak" CFG="quake2s - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "quake2s - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "quake2s - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\release"
# PROP Intermediate_Dir ".\release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 winmm.lib wsock32.lib kernel32.lib user32.lib gdi32.lib dinput.lib /nologo /subsystem:windows /map /machine:I386 /out:".\release/quake2.exe" /heap:16740352,524288 /filealign:512
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\debug"
# PROP Intermediate_Dir ".\debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 winmm.lib wsock32.lib kernel32.lib user32.lib gdi32.lib /nologo /subsystem:windows /incremental:no /map /debug /machine:I386 /out:".\debug/quake2.exe"
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "quake2s - Win32 Release"
# Name "quake2s - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\win32\cd_win.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_cin.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_ents.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_fx.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_input.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_inv.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_main.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_newfx.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_parse.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_pred.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_scrn.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_tent.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_view.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\cmd.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\cmodel.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\common.c
# End Source File
# Begin Source File

SOURCE=.\win32\conproc.c
# End Source File
# Begin Source File

SOURCE=.\client\console.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\crc.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\cvar.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\files.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\images.c
# End Source File
# Begin Source File

SOURCE=.\win32\in_win.c
# End Source File
# Begin Source File

SOURCE=.\client\keys.c
# End Source File
# Begin Source File

SOURCE=.\game\m_flash.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\md4.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\memory.c
# End Source File
# Begin Source File

SOURCE=.\client\menu.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\model.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\net_chan.c
# End Source File
# Begin Source File

SOURCE=.\win32\net_wins.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\pmove.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\q_shared2.c
# End Source File
# Begin Source File

SOURCE=.\client\qmenu.c
# End Source File
# Begin Source File

SOURCE=.\client\snd_dma.c
# End Source File
# Begin Source File

SOURCE=.\client\snd_mem.c
# End Source File
# Begin Source File

SOURCE=.\client\snd_mix.c
# End Source File
# Begin Source File

SOURCE=.\win32\snd_win.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_ccmds.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_ents.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_game.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_init.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_main.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_send.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_user.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_world.c
# End Source File
# Begin Source File

SOURCE=.\win32\sys_win.c
# End Source File
# Begin Source File

SOURCE=.\win32\vid_dll.c
# End Source File
# Begin Source File

SOURCE=.\win32\vid_menu.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\qcommon\anorms.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\bspfile.h
# End Source File
# Begin Source File

SOURCE=.\client\cdaudio.h
# End Source File
# Begin Source File

SOURCE=.\client\client.h
# End Source File
# Begin Source File

SOURCE=.\win32\conproc.h
# End Source File
# Begin Source File

SOURCE=.\client\console.h
# End Source File
# Begin Source File

SOURCE=.\server\game.h
# End Source File
# Begin Source File

SOURCE=.\client\input.h
# End Source File
# Begin Source File

SOURCE=.\client\keys.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\q_shared2.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\qcommon.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\qfiles.h
# End Source File
# Begin Source File

SOURCE=.\client\qmenu.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\qmenu.h
# End Source File
# Begin Source File

SOURCE=.\client\ref.h
# End Source File
# Begin Source File

SOURCE=.\client\screen.h
# End Source File
# Begin Source File

SOURCE=.\server\server.h
# End Source File
# Begin Source File

SOURCE=.\client\snd_loc.h
# End Source File
# Begin Source File

SOURCE=.\client\sound.h
# End Source File
# Begin Source File

SOURCE=.\client\vid.h
# End Source File
# Begin Source File

SOURCE=.\win32\winquake.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\win32\q2.ico
# End Source File
# Begin Source File

SOURCE=.\win32\q2.rc
# End Source File
# End Group
# Begin Group "ref_gl"

# PROP Default_Filter ""
# Begin Group "Source Files (gl)"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=.\ref_gl\gl_backend.c
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_buffers.c
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_frontend.c
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_image.c
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_interface.c
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_light.c
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_lightmap.c
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_main.c
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_math.c
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_model.c
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_poly.c
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_shader.c
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_sky.c
# End Source File
# Begin Source File

SOURCE=.\win32\glw_imp.c
# End Source File
# Begin Source File

SOURCE=.\win32\qgl_win.c
# End Source File
# End Group
# Begin Group "Header Files (gl)"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\ref_gl\gl_backend.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_buffers.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_frontend.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_image.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_interface.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_light.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_lightmap.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_local.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_math.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_model.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_poly.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_shader.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_sky.h
# End Source File
# Begin Source File

SOURCE=.\win32\glw_win.h
# End Source File
# End Group
# End Group
# Begin Group "ref_soft"

# PROP Default_Filter ""
# Begin Group "Source Files (soft)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\win32\q_shwin.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_aclip.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_alias.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_bsp.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_draw.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_edge.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_image.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_light.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_main.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_misc.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_model.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_part.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_poly.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_polyse.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_rast.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_scan.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_sprite.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_surf.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\win32\rw_ddraw.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\win32\rw_dib.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\win32\rw_imp.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files (soft)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ref_soft\anorms.h
# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_local.h
# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_model.h

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\rand1k.h

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\win32\rw_win.h

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Assembler files"

# PROP Default_Filter "*.asm"
# Begin Source File

SOURCE=.\ref_soft\r_aclipa.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

# Begin Custom Build
OutDir=.\debug
InputPath=.\ref_soft\r_aclipa.asm
InputName=r_aclipa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_draw16.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

# Begin Custom Build
OutDir=.\debug
InputPath=.\ref_soft\r_draw16.asm
InputName=r_draw16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_drawa.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

# Begin Custom Build
OutDir=.\debug
InputPath=.\ref_soft\r_drawa.asm
InputName=r_drawa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_edgea.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

# Begin Custom Build
OutDir=.\debug
InputPath=.\ref_soft\r_edgea.asm
InputName=r_edgea

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_polysa.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

# Begin Custom Build
OutDir=.\debug
InputPath=.\ref_soft\r_polysa.asm
InputName=r_polysa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_scana.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

# Begin Custom Build
OutDir=.\debug
InputPath=.\ref_soft\r_scana.asm
InputName=r_scana

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_spr8.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

# Begin Custom Build
OutDir=.\debug
InputPath=.\ref_soft\r_spr8.asm
InputName=r_spr8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_surf8.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

# Begin Custom Build
OutDir=.\debug
InputPath=.\ref_soft\r_surf8.asm
InputName=r_surf8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_varsa.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

# PROP Intermediate_Dir ".\release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

# Begin Custom Build
OutDir=.\debug
InputPath=.\ref_soft\r_varsa.asm
InputName=r_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Group
# Begin Group "Zip files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\zip\adler32.c
# End Source File
# Begin Source File

SOURCE=.\zip\crc32.c
# End Source File
# Begin Source File

SOURCE=.\zip\infblock.c
# End Source File
# Begin Source File

SOURCE=.\zip\infblock.h
# End Source File
# Begin Source File

SOURCE=.\zip\infcodes.c
# End Source File
# Begin Source File

SOURCE=.\zip\infcodes.h
# End Source File
# Begin Source File

SOURCE=.\zip\inffast.c
# End Source File
# Begin Source File

SOURCE=.\zip\inffast.h
# End Source File
# Begin Source File

SOURCE=.\zip\inffixed.h
# End Source File
# Begin Source File

SOURCE=.\zip\inflate.c
# End Source File
# Begin Source File

SOURCE=.\zip\inftrees.c
# End Source File
# Begin Source File

SOURCE=.\zip\inftrees.h
# End Source File
# Begin Source File

SOURCE=.\zip\infutil.c
# End Source File
# Begin Source File

SOURCE=.\zip\infutil.h
# End Source File
# Begin Source File

SOURCE=.\zip\zconf.h
# End Source File
# Begin Source File

SOURCE=.\zip\zip.c
# End Source File
# Begin Source File

SOURCE=.\zip\zip.h
# End Source File
# Begin Source File

SOURCE=.\zip\zlib.h
# End Source File
# Begin Source File

SOURCE=.\zip\zutil.c
# End Source File
# Begin Source File

SOURCE=.\zip\zutil.h
# End Source File
# End Group
# Begin Group "JPEG files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\jpeg\jcapimin.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jcapistd.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jccoefct.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jccolor.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jcdctmgr.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jcdiffct.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jchuff.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jchuff.h
# End Source File
# Begin Source File

SOURCE=.\jpeg\jcinit.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jclhuff.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jclossls.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jclossy.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jcmainct.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jcmarker.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jcmaster.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jcodec.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jcomapi.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jconfig.h
# End Source File
# Begin Source File

SOURCE=.\jpeg\jcparam.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jcphuff.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jcpred.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jcprepct.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jcsample.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jcscale.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jcshuff.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdapimin.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdapistd.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdatadst.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdcoefct.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdcolor.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdct.h
# End Source File
# Begin Source File

SOURCE=.\jpeg\jddctmgr.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jddiffct.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdhuff.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdhuff.h
# End Source File
# Begin Source File

SOURCE=.\jpeg\jdinput.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdlhuff.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdlossls.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdlossy.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdmainct.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdmarker.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdmaster.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdmerge.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdphuff.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdpostct.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdpred.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdsample.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdscale.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jdshuff.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jerror.h
# End Source File
# Begin Source File

SOURCE=.\jpeg\jfdctflt.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jfdctfst.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jfdctint.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jidctflt.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jidctfst.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jidctint.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jidctred.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jinclude.h
# End Source File
# Begin Source File

SOURCE=.\jpeg\jlossls.h
# End Source File
# Begin Source File

SOURCE=.\jpeg\jlossy.h
# End Source File
# Begin Source File

SOURCE=.\jpeg\jmemansi.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jmemmgr.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jmemsys.h
# End Source File
# Begin Source File

SOURCE=.\jpeg\jmorecfg.h
# End Source File
# Begin Source File

SOURCE=.\jpeg\jpegint.h
# End Source File
# Begin Source File

SOURCE=.\jpeg\jpeglib.h
# End Source File
# Begin Source File

SOURCE=.\jpeg\jquant1.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jquant2.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jutils.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

# ADD CPP /W1

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg\jversion.h
# End Source File
# End Group
# Begin Group "Misc Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\resource\MAKE\archive.gz

!IF  "$(CFG)" == "quake2s - Win32 Release"

# Begin Custom Build
InputDir=.\resource\MAKE
IntDir=.\release
InputPath=.\resource\MAKE\archive.gz

"$(IntDir)\resources.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f win32 -o $(IntDir)\resources.obj -Darc=\"$(InputPath)\" $(InputDir)\make.asm

# End Custom Build

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
