# Microsoft Developer Studio Generated NMAKE File, Based on quake2.dsp
!IF "$(CFG)" == ""
CFG=quake2 - Win32 Release
!MESSAGE No configuration specified. Defaulting to quake2 - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "quake2 - Win32 Release" && "$(CFG)" != "quake2 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "quake2.mak" CFG="quake2 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "quake2 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "quake2 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "quake2 - Win32 Release"

OUTDIR=.\release
INTDIR=.\release
# Begin Custom Macros
OutDir=.\.\release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\quake2.exe"

!ELSE 

ALL : "ref_soft - Win32 Release" "ref_gl - Win32 Release"\
 "$(OUTDIR)\quake2.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"ref_gl - Win32 ReleaseCLEAN" "ref_soft - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\adler32.obj"
	-@erase "$(INTDIR)\cd_win.obj"
	-@erase "$(INTDIR)\cl_cin.obj"
	-@erase "$(INTDIR)\cl_ents.obj"
	-@erase "$(INTDIR)\cl_fx.obj"
	-@erase "$(INTDIR)\cl_input.obj"
	-@erase "$(INTDIR)\cl_inv.obj"
	-@erase "$(INTDIR)\cl_main.obj"
	-@erase "$(INTDIR)\cl_newfx.obj"
	-@erase "$(INTDIR)\cl_parse.obj"
	-@erase "$(INTDIR)\cl_pred.obj"
	-@erase "$(INTDIR)\cl_scrn.obj"
	-@erase "$(INTDIR)\cl_tent.obj"
	-@erase "$(INTDIR)\cl_view.obj"
	-@erase "$(INTDIR)\cmd.obj"
	-@erase "$(INTDIR)\cmodel.obj"
	-@erase "$(INTDIR)\common.obj"
	-@erase "$(INTDIR)\conproc.obj"
	-@erase "$(INTDIR)\console.obj"
	-@erase "$(INTDIR)\crc.obj"
	-@erase "$(INTDIR)\crc32.obj"
	-@erase "$(INTDIR)\cvar.obj"
	-@erase "$(INTDIR)\files.obj"
	-@erase "$(INTDIR)\images.obj"
	-@erase "$(INTDIR)\in_win.obj"
	-@erase "$(INTDIR)\infblock.obj"
	-@erase "$(INTDIR)\infcodes.obj"
	-@erase "$(INTDIR)\inffast.obj"
	-@erase "$(INTDIR)\inflate.obj"
	-@erase "$(INTDIR)\inftrees.obj"
	-@erase "$(INTDIR)\infutil.obj"
	-@erase "$(INTDIR)\jccoefct.obj"
	-@erase "$(INTDIR)\jcdctmgr.obj"
	-@erase "$(INTDIR)\jcdiffct.obj"
	-@erase "$(INTDIR)\jchuff.obj"
	-@erase "$(INTDIR)\jclhuff.obj"
	-@erase "$(INTDIR)\jclossls.obj"
	-@erase "$(INTDIR)\jclossy.obj"
	-@erase "$(INTDIR)\jcodec.obj"
	-@erase "$(INTDIR)\jcomapi.obj"
	-@erase "$(INTDIR)\jcphuff.obj"
	-@erase "$(INTDIR)\jcpred.obj"
	-@erase "$(INTDIR)\jcscale.obj"
	-@erase "$(INTDIR)\jcshuff.obj"
	-@erase "$(INTDIR)\jdapimin.obj"
	-@erase "$(INTDIR)\jdapistd.obj"
	-@erase "$(INTDIR)\jdcoefct.obj"
	-@erase "$(INTDIR)\jdcolor.obj"
	-@erase "$(INTDIR)\jddctmgr.obj"
	-@erase "$(INTDIR)\jddiffct.obj"
	-@erase "$(INTDIR)\jdhuff.obj"
	-@erase "$(INTDIR)\jdinput.obj"
	-@erase "$(INTDIR)\jdlhuff.obj"
	-@erase "$(INTDIR)\jdlossls.obj"
	-@erase "$(INTDIR)\jdlossy.obj"
	-@erase "$(INTDIR)\jdmainct.obj"
	-@erase "$(INTDIR)\jdmarker.obj"
	-@erase "$(INTDIR)\jdmaster.obj"
	-@erase "$(INTDIR)\jdmerge.obj"
	-@erase "$(INTDIR)\jdphuff.obj"
	-@erase "$(INTDIR)\jdpostct.obj"
	-@erase "$(INTDIR)\jdpred.obj"
	-@erase "$(INTDIR)\jdsample.obj"
	-@erase "$(INTDIR)\jdscale.obj"
	-@erase "$(INTDIR)\jdshuff.obj"
	-@erase "$(INTDIR)\jerror.obj"
	-@erase "$(INTDIR)\jfdctflt.obj"
	-@erase "$(INTDIR)\jfdctfst.obj"
	-@erase "$(INTDIR)\jfdctint.obj"
	-@erase "$(INTDIR)\jidctflt.obj"
	-@erase "$(INTDIR)\jidctfst.obj"
	-@erase "$(INTDIR)\jidctint.obj"
	-@erase "$(INTDIR)\jidctred.obj"
	-@erase "$(INTDIR)\jmemansi.obj"
	-@erase "$(INTDIR)\jmemmgr.obj"
	-@erase "$(INTDIR)\jquant1.obj"
	-@erase "$(INTDIR)\jquant2.obj"
	-@erase "$(INTDIR)\jutils.obj"
	-@erase "$(INTDIR)\keys.obj"
	-@erase "$(INTDIR)\m_flash.obj"
	-@erase "$(INTDIR)\md4.obj"
	-@erase "$(INTDIR)\menu.obj"
	-@erase "$(INTDIR)\model.obj"
	-@erase "$(INTDIR)\net_chan.obj"
	-@erase "$(INTDIR)\net_wins.obj"
	-@erase "$(INTDIR)\pmove.obj"
	-@erase "$(INTDIR)\q2.res"
	-@erase "$(INTDIR)\q_shared2.obj"
	-@erase "$(INTDIR)\q_shwin.obj"
	-@erase "$(INTDIR)\qmenu.obj"
	-@erase "$(INTDIR)\snd_dma.obj"
	-@erase "$(INTDIR)\snd_mem.obj"
	-@erase "$(INTDIR)\snd_mix.obj"
	-@erase "$(INTDIR)\snd_win.obj"
	-@erase "$(INTDIR)\sv_ccmds.obj"
	-@erase "$(INTDIR)\sv_ents.obj"
	-@erase "$(INTDIR)\sv_game.obj"
	-@erase "$(INTDIR)\sv_init.obj"
	-@erase "$(INTDIR)\sv_main.obj"
	-@erase "$(INTDIR)\sv_send.obj"
	-@erase "$(INTDIR)\sv_user.obj"
	-@erase "$(INTDIR)\sv_world.obj"
	-@erase "$(INTDIR)\sys_win.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vid_dll.obj"
	-@erase "$(INTDIR)\vid_menu.obj"
	-@erase "$(INTDIR)\x86.obj"
	-@erase "$(INTDIR)\zip.obj"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase "$(OUTDIR)\quake2.exe"
	-@erase "$(OUTDIR)\quake2.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /G5 /MD /W3 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\release/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\q2.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\quake2.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=winmm.lib wsock32.lib kernel32.lib user32.lib gdi32.lib /nologo\
 /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\quake2.pdb"\
 /map:"$(INTDIR)\quake2.map" /machine:I386 /out:"$(OUTDIR)\quake2.exe"\
 /heap:16740352,524288 
LINK32_OBJS= \
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\cd_win.obj" \
	"$(INTDIR)\cl_cin.obj" \
	"$(INTDIR)\cl_ents.obj" \
	"$(INTDIR)\cl_fx.obj" \
	"$(INTDIR)\cl_input.obj" \
	"$(INTDIR)\cl_inv.obj" \
	"$(INTDIR)\cl_main.obj" \
	"$(INTDIR)\cl_newfx.obj" \
	"$(INTDIR)\cl_parse.obj" \
	"$(INTDIR)\cl_pred.obj" \
	"$(INTDIR)\cl_scrn.obj" \
	"$(INTDIR)\cl_tent.obj" \
	"$(INTDIR)\cl_view.obj" \
	"$(INTDIR)\cmd.obj" \
	"$(INTDIR)\cmodel.obj" \
	"$(INTDIR)\common.obj" \
	"$(INTDIR)\conproc.obj" \
	"$(INTDIR)\console.obj" \
	"$(INTDIR)\crc.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\cvar.obj" \
	"$(INTDIR)\files.obj" \
	"$(INTDIR)\images.obj" \
	"$(INTDIR)\in_win.obj" \
	"$(INTDIR)\infblock.obj" \
	"$(INTDIR)\infcodes.obj" \
	"$(INTDIR)\inffast.obj" \
	"$(INTDIR)\inflate.obj" \
	"$(INTDIR)\inftrees.obj" \
	"$(INTDIR)\infutil.obj" \
	"$(INTDIR)\jccoefct.obj" \
	"$(INTDIR)\jcdctmgr.obj" \
	"$(INTDIR)\jcdiffct.obj" \
	"$(INTDIR)\jchuff.obj" \
	"$(INTDIR)\jclhuff.obj" \
	"$(INTDIR)\jclossls.obj" \
	"$(INTDIR)\jclossy.obj" \
	"$(INTDIR)\jcodec.obj" \
	"$(INTDIR)\jcomapi.obj" \
	"$(INTDIR)\jcphuff.obj" \
	"$(INTDIR)\jcpred.obj" \
	"$(INTDIR)\jcscale.obj" \
	"$(INTDIR)\jcshuff.obj" \
	"$(INTDIR)\jdapimin.obj" \
	"$(INTDIR)\jdapistd.obj" \
	"$(INTDIR)\jdcoefct.obj" \
	"$(INTDIR)\jdcolor.obj" \
	"$(INTDIR)\jddctmgr.obj" \
	"$(INTDIR)\jddiffct.obj" \
	"$(INTDIR)\jdhuff.obj" \
	"$(INTDIR)\jdinput.obj" \
	"$(INTDIR)\jdlhuff.obj" \
	"$(INTDIR)\jdlossls.obj" \
	"$(INTDIR)\jdlossy.obj" \
	"$(INTDIR)\jdmainct.obj" \
	"$(INTDIR)\jdmarker.obj" \
	"$(INTDIR)\jdmaster.obj" \
	"$(INTDIR)\jdmerge.obj" \
	"$(INTDIR)\jdphuff.obj" \
	"$(INTDIR)\jdpostct.obj" \
	"$(INTDIR)\jdpred.obj" \
	"$(INTDIR)\jdsample.obj" \
	"$(INTDIR)\jdscale.obj" \
	"$(INTDIR)\jdshuff.obj" \
	"$(INTDIR)\jerror.obj" \
	"$(INTDIR)\jfdctflt.obj" \
	"$(INTDIR)\jfdctfst.obj" \
	"$(INTDIR)\jfdctint.obj" \
	"$(INTDIR)\jidctflt.obj" \
	"$(INTDIR)\jidctfst.obj" \
	"$(INTDIR)\jidctint.obj" \
	"$(INTDIR)\jidctred.obj" \
	"$(INTDIR)\jmemansi.obj" \
	"$(INTDIR)\jmemmgr.obj" \
	"$(INTDIR)\jquant1.obj" \
	"$(INTDIR)\jquant2.obj" \
	"$(INTDIR)\jutils.obj" \
	"$(INTDIR)\keys.obj" \
	"$(INTDIR)\m_flash.obj" \
	"$(INTDIR)\md4.obj" \
	"$(INTDIR)\menu.obj" \
	"$(INTDIR)\model.obj" \
	"$(INTDIR)\net_chan.obj" \
	"$(INTDIR)\net_wins.obj" \
	"$(INTDIR)\pmove.obj" \
	"$(INTDIR)\q2.res" \
	"$(INTDIR)\q_shared2.obj" \
	"$(INTDIR)\q_shwin.obj" \
	"$(INTDIR)\qmenu.obj" \
	"$(INTDIR)\snd_dma.obj" \
	"$(INTDIR)\snd_mem.obj" \
	"$(INTDIR)\snd_mix.obj" \
	"$(INTDIR)\snd_win.obj" \
	"$(INTDIR)\sv_ccmds.obj" \
	"$(INTDIR)\sv_ents.obj" \
	"$(INTDIR)\sv_game.obj" \
	"$(INTDIR)\sv_init.obj" \
	"$(INTDIR)\sv_main.obj" \
	"$(INTDIR)\sv_send.obj" \
	"$(INTDIR)\sv_user.obj" \
	"$(INTDIR)\sv_world.obj" \
	"$(INTDIR)\sys_win.obj" \
	"$(INTDIR)\vid_dll.obj" \
	"$(INTDIR)\vid_menu.obj" \
	"$(INTDIR)\x86.obj" \
	"$(INTDIR)\zip.obj" \
	"$(INTDIR)\zutil.obj" \
	".\ref_gl\release\ref_gl.lib" \
	".\ref_soft\release\ref_soft.lib"

"$(OUTDIR)\quake2.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

OUTDIR=.\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\.\debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\quake2.exe" "$(OUTDIR)\quake2.bsc"

!ELSE 

ALL : "ref_soft - Win32 Debug" "ref_gl - Win32 Debug" "$(OUTDIR)\quake2.exe"\
 "$(OUTDIR)\quake2.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"ref_gl - Win32 DebugCLEAN" "ref_soft - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\adler32.obj"
	-@erase "$(INTDIR)\adler32.sbr"
	-@erase "$(INTDIR)\cd_win.obj"
	-@erase "$(INTDIR)\cd_win.sbr"
	-@erase "$(INTDIR)\cl_cin.obj"
	-@erase "$(INTDIR)\cl_cin.sbr"
	-@erase "$(INTDIR)\cl_ents.obj"
	-@erase "$(INTDIR)\cl_ents.sbr"
	-@erase "$(INTDIR)\cl_fx.obj"
	-@erase "$(INTDIR)\cl_fx.sbr"
	-@erase "$(INTDIR)\cl_input.obj"
	-@erase "$(INTDIR)\cl_input.sbr"
	-@erase "$(INTDIR)\cl_inv.obj"
	-@erase "$(INTDIR)\cl_inv.sbr"
	-@erase "$(INTDIR)\cl_main.obj"
	-@erase "$(INTDIR)\cl_main.sbr"
	-@erase "$(INTDIR)\cl_newfx.obj"
	-@erase "$(INTDIR)\cl_newfx.sbr"
	-@erase "$(INTDIR)\cl_parse.obj"
	-@erase "$(INTDIR)\cl_parse.sbr"
	-@erase "$(INTDIR)\cl_pred.obj"
	-@erase "$(INTDIR)\cl_pred.sbr"
	-@erase "$(INTDIR)\cl_scrn.obj"
	-@erase "$(INTDIR)\cl_scrn.sbr"
	-@erase "$(INTDIR)\cl_tent.obj"
	-@erase "$(INTDIR)\cl_tent.sbr"
	-@erase "$(INTDIR)\cl_view.obj"
	-@erase "$(INTDIR)\cl_view.sbr"
	-@erase "$(INTDIR)\cmd.obj"
	-@erase "$(INTDIR)\cmd.sbr"
	-@erase "$(INTDIR)\cmodel.obj"
	-@erase "$(INTDIR)\cmodel.sbr"
	-@erase "$(INTDIR)\common.obj"
	-@erase "$(INTDIR)\common.sbr"
	-@erase "$(INTDIR)\conproc.obj"
	-@erase "$(INTDIR)\conproc.sbr"
	-@erase "$(INTDIR)\console.obj"
	-@erase "$(INTDIR)\console.sbr"
	-@erase "$(INTDIR)\crc.obj"
	-@erase "$(INTDIR)\crc.sbr"
	-@erase "$(INTDIR)\crc32.obj"
	-@erase "$(INTDIR)\crc32.sbr"
	-@erase "$(INTDIR)\cvar.obj"
	-@erase "$(INTDIR)\cvar.sbr"
	-@erase "$(INTDIR)\files.obj"
	-@erase "$(INTDIR)\files.sbr"
	-@erase "$(INTDIR)\images.obj"
	-@erase "$(INTDIR)\images.sbr"
	-@erase "$(INTDIR)\in_win.obj"
	-@erase "$(INTDIR)\in_win.sbr"
	-@erase "$(INTDIR)\infblock.obj"
	-@erase "$(INTDIR)\infblock.sbr"
	-@erase "$(INTDIR)\infcodes.obj"
	-@erase "$(INTDIR)\infcodes.sbr"
	-@erase "$(INTDIR)\inffast.obj"
	-@erase "$(INTDIR)\inffast.sbr"
	-@erase "$(INTDIR)\inflate.obj"
	-@erase "$(INTDIR)\inflate.sbr"
	-@erase "$(INTDIR)\inftrees.obj"
	-@erase "$(INTDIR)\inftrees.sbr"
	-@erase "$(INTDIR)\infutil.obj"
	-@erase "$(INTDIR)\infutil.sbr"
	-@erase "$(INTDIR)\jccoefct.obj"
	-@erase "$(INTDIR)\jccoefct.sbr"
	-@erase "$(INTDIR)\jcdctmgr.obj"
	-@erase "$(INTDIR)\jcdctmgr.sbr"
	-@erase "$(INTDIR)\jcdiffct.obj"
	-@erase "$(INTDIR)\jcdiffct.sbr"
	-@erase "$(INTDIR)\jchuff.obj"
	-@erase "$(INTDIR)\jchuff.sbr"
	-@erase "$(INTDIR)\jclhuff.obj"
	-@erase "$(INTDIR)\jclhuff.sbr"
	-@erase "$(INTDIR)\jclossls.obj"
	-@erase "$(INTDIR)\jclossls.sbr"
	-@erase "$(INTDIR)\jclossy.obj"
	-@erase "$(INTDIR)\jclossy.sbr"
	-@erase "$(INTDIR)\jcodec.obj"
	-@erase "$(INTDIR)\jcodec.sbr"
	-@erase "$(INTDIR)\jcomapi.obj"
	-@erase "$(INTDIR)\jcomapi.sbr"
	-@erase "$(INTDIR)\jcphuff.obj"
	-@erase "$(INTDIR)\jcphuff.sbr"
	-@erase "$(INTDIR)\jcpred.obj"
	-@erase "$(INTDIR)\jcpred.sbr"
	-@erase "$(INTDIR)\jcscale.obj"
	-@erase "$(INTDIR)\jcscale.sbr"
	-@erase "$(INTDIR)\jcshuff.obj"
	-@erase "$(INTDIR)\jcshuff.sbr"
	-@erase "$(INTDIR)\jdapimin.obj"
	-@erase "$(INTDIR)\jdapimin.sbr"
	-@erase "$(INTDIR)\jdapistd.obj"
	-@erase "$(INTDIR)\jdapistd.sbr"
	-@erase "$(INTDIR)\jdcoefct.obj"
	-@erase "$(INTDIR)\jdcoefct.sbr"
	-@erase "$(INTDIR)\jdcolor.obj"
	-@erase "$(INTDIR)\jdcolor.sbr"
	-@erase "$(INTDIR)\jddctmgr.obj"
	-@erase "$(INTDIR)\jddctmgr.sbr"
	-@erase "$(INTDIR)\jddiffct.obj"
	-@erase "$(INTDIR)\jddiffct.sbr"
	-@erase "$(INTDIR)\jdhuff.obj"
	-@erase "$(INTDIR)\jdhuff.sbr"
	-@erase "$(INTDIR)\jdinput.obj"
	-@erase "$(INTDIR)\jdinput.sbr"
	-@erase "$(INTDIR)\jdlhuff.obj"
	-@erase "$(INTDIR)\jdlhuff.sbr"
	-@erase "$(INTDIR)\jdlossls.obj"
	-@erase "$(INTDIR)\jdlossls.sbr"
	-@erase "$(INTDIR)\jdlossy.obj"
	-@erase "$(INTDIR)\jdlossy.sbr"
	-@erase "$(INTDIR)\jdmainct.obj"
	-@erase "$(INTDIR)\jdmainct.sbr"
	-@erase "$(INTDIR)\jdmarker.obj"
	-@erase "$(INTDIR)\jdmarker.sbr"
	-@erase "$(INTDIR)\jdmaster.obj"
	-@erase "$(INTDIR)\jdmaster.sbr"
	-@erase "$(INTDIR)\jdmerge.obj"
	-@erase "$(INTDIR)\jdmerge.sbr"
	-@erase "$(INTDIR)\jdphuff.obj"
	-@erase "$(INTDIR)\jdphuff.sbr"
	-@erase "$(INTDIR)\jdpostct.obj"
	-@erase "$(INTDIR)\jdpostct.sbr"
	-@erase "$(INTDIR)\jdpred.obj"
	-@erase "$(INTDIR)\jdpred.sbr"
	-@erase "$(INTDIR)\jdsample.obj"
	-@erase "$(INTDIR)\jdsample.sbr"
	-@erase "$(INTDIR)\jdscale.obj"
	-@erase "$(INTDIR)\jdscale.sbr"
	-@erase "$(INTDIR)\jdshuff.obj"
	-@erase "$(INTDIR)\jdshuff.sbr"
	-@erase "$(INTDIR)\jerror.obj"
	-@erase "$(INTDIR)\jerror.sbr"
	-@erase "$(INTDIR)\jfdctflt.obj"
	-@erase "$(INTDIR)\jfdctflt.sbr"
	-@erase "$(INTDIR)\jfdctfst.obj"
	-@erase "$(INTDIR)\jfdctfst.sbr"
	-@erase "$(INTDIR)\jfdctint.obj"
	-@erase "$(INTDIR)\jfdctint.sbr"
	-@erase "$(INTDIR)\jidctflt.obj"
	-@erase "$(INTDIR)\jidctflt.sbr"
	-@erase "$(INTDIR)\jidctfst.obj"
	-@erase "$(INTDIR)\jidctfst.sbr"
	-@erase "$(INTDIR)\jidctint.obj"
	-@erase "$(INTDIR)\jidctint.sbr"
	-@erase "$(INTDIR)\jidctred.obj"
	-@erase "$(INTDIR)\jidctred.sbr"
	-@erase "$(INTDIR)\jmemansi.obj"
	-@erase "$(INTDIR)\jmemansi.sbr"
	-@erase "$(INTDIR)\jmemmgr.obj"
	-@erase "$(INTDIR)\jmemmgr.sbr"
	-@erase "$(INTDIR)\jquant1.obj"
	-@erase "$(INTDIR)\jquant1.sbr"
	-@erase "$(INTDIR)\jquant2.obj"
	-@erase "$(INTDIR)\jquant2.sbr"
	-@erase "$(INTDIR)\jutils.obj"
	-@erase "$(INTDIR)\jutils.sbr"
	-@erase "$(INTDIR)\keys.obj"
	-@erase "$(INTDIR)\keys.sbr"
	-@erase "$(INTDIR)\m_flash.obj"
	-@erase "$(INTDIR)\m_flash.sbr"
	-@erase "$(INTDIR)\md4.obj"
	-@erase "$(INTDIR)\md4.sbr"
	-@erase "$(INTDIR)\menu.obj"
	-@erase "$(INTDIR)\menu.sbr"
	-@erase "$(INTDIR)\model.obj"
	-@erase "$(INTDIR)\model.sbr"
	-@erase "$(INTDIR)\net_chan.obj"
	-@erase "$(INTDIR)\net_chan.sbr"
	-@erase "$(INTDIR)\net_wins.obj"
	-@erase "$(INTDIR)\net_wins.sbr"
	-@erase "$(INTDIR)\pmove.obj"
	-@erase "$(INTDIR)\pmove.sbr"
	-@erase "$(INTDIR)\q2.res"
	-@erase "$(INTDIR)\q_shared2.obj"
	-@erase "$(INTDIR)\q_shared2.sbr"
	-@erase "$(INTDIR)\q_shwin.obj"
	-@erase "$(INTDIR)\q_shwin.sbr"
	-@erase "$(INTDIR)\qmenu.obj"
	-@erase "$(INTDIR)\qmenu.sbr"
	-@erase "$(INTDIR)\snd_dma.obj"
	-@erase "$(INTDIR)\snd_dma.sbr"
	-@erase "$(INTDIR)\snd_mem.obj"
	-@erase "$(INTDIR)\snd_mem.sbr"
	-@erase "$(INTDIR)\snd_mix.obj"
	-@erase "$(INTDIR)\snd_mix.sbr"
	-@erase "$(INTDIR)\snd_win.obj"
	-@erase "$(INTDIR)\snd_win.sbr"
	-@erase "$(INTDIR)\sv_ccmds.obj"
	-@erase "$(INTDIR)\sv_ccmds.sbr"
	-@erase "$(INTDIR)\sv_ents.obj"
	-@erase "$(INTDIR)\sv_ents.sbr"
	-@erase "$(INTDIR)\sv_game.obj"
	-@erase "$(INTDIR)\sv_game.sbr"
	-@erase "$(INTDIR)\sv_init.obj"
	-@erase "$(INTDIR)\sv_init.sbr"
	-@erase "$(INTDIR)\sv_main.obj"
	-@erase "$(INTDIR)\sv_main.sbr"
	-@erase "$(INTDIR)\sv_send.obj"
	-@erase "$(INTDIR)\sv_send.sbr"
	-@erase "$(INTDIR)\sv_user.obj"
	-@erase "$(INTDIR)\sv_user.sbr"
	-@erase "$(INTDIR)\sv_world.obj"
	-@erase "$(INTDIR)\sv_world.sbr"
	-@erase "$(INTDIR)\sys_win.obj"
	-@erase "$(INTDIR)\sys_win.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vid_dll.obj"
	-@erase "$(INTDIR)\vid_dll.sbr"
	-@erase "$(INTDIR)\vid_menu.obj"
	-@erase "$(INTDIR)\vid_menu.sbr"
	-@erase "$(INTDIR)\x86.obj"
	-@erase "$(INTDIR)\x86.sbr"
	-@erase "$(INTDIR)\zip.obj"
	-@erase "$(INTDIR)\zip.sbr"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase "$(INTDIR)\zutil.sbr"
	-@erase "$(OUTDIR)\quake2.bsc"
	-@erase "$(OUTDIR)\quake2.exe"
	-@erase "$(OUTDIR)\quake2.map"
	-@erase "$(OUTDIR)\quake2.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 
CPP_OBJS=.\debug/
CPP_SBRS=.\debug/

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\q2.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\quake2.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\adler32.sbr" \
	"$(INTDIR)\cd_win.sbr" \
	"$(INTDIR)\cl_cin.sbr" \
	"$(INTDIR)\cl_ents.sbr" \
	"$(INTDIR)\cl_fx.sbr" \
	"$(INTDIR)\cl_input.sbr" \
	"$(INTDIR)\cl_inv.sbr" \
	"$(INTDIR)\cl_main.sbr" \
	"$(INTDIR)\cl_newfx.sbr" \
	"$(INTDIR)\cl_parse.sbr" \
	"$(INTDIR)\cl_pred.sbr" \
	"$(INTDIR)\cl_scrn.sbr" \
	"$(INTDIR)\cl_tent.sbr" \
	"$(INTDIR)\cl_view.sbr" \
	"$(INTDIR)\cmd.sbr" \
	"$(INTDIR)\cmodel.sbr" \
	"$(INTDIR)\common.sbr" \
	"$(INTDIR)\conproc.sbr" \
	"$(INTDIR)\console.sbr" \
	"$(INTDIR)\crc.sbr" \
	"$(INTDIR)\crc32.sbr" \
	"$(INTDIR)\cvar.sbr" \
	"$(INTDIR)\files.sbr" \
	"$(INTDIR)\images.sbr" \
	"$(INTDIR)\in_win.sbr" \
	"$(INTDIR)\infblock.sbr" \
	"$(INTDIR)\infcodes.sbr" \
	"$(INTDIR)\inffast.sbr" \
	"$(INTDIR)\inflate.sbr" \
	"$(INTDIR)\inftrees.sbr" \
	"$(INTDIR)\infutil.sbr" \
	"$(INTDIR)\jccoefct.sbr" \
	"$(INTDIR)\jcdctmgr.sbr" \
	"$(INTDIR)\jcdiffct.sbr" \
	"$(INTDIR)\jchuff.sbr" \
	"$(INTDIR)\jclhuff.sbr" \
	"$(INTDIR)\jclossls.sbr" \
	"$(INTDIR)\jclossy.sbr" \
	"$(INTDIR)\jcodec.sbr" \
	"$(INTDIR)\jcomapi.sbr" \
	"$(INTDIR)\jcphuff.sbr" \
	"$(INTDIR)\jcpred.sbr" \
	"$(INTDIR)\jcscale.sbr" \
	"$(INTDIR)\jcshuff.sbr" \
	"$(INTDIR)\jdapimin.sbr" \
	"$(INTDIR)\jdapistd.sbr" \
	"$(INTDIR)\jdcoefct.sbr" \
	"$(INTDIR)\jdcolor.sbr" \
	"$(INTDIR)\jddctmgr.sbr" \
	"$(INTDIR)\jddiffct.sbr" \
	"$(INTDIR)\jdhuff.sbr" \
	"$(INTDIR)\jdinput.sbr" \
	"$(INTDIR)\jdlhuff.sbr" \
	"$(INTDIR)\jdlossls.sbr" \
	"$(INTDIR)\jdlossy.sbr" \
	"$(INTDIR)\jdmainct.sbr" \
	"$(INTDIR)\jdmarker.sbr" \
	"$(INTDIR)\jdmaster.sbr" \
	"$(INTDIR)\jdmerge.sbr" \
	"$(INTDIR)\jdphuff.sbr" \
	"$(INTDIR)\jdpostct.sbr" \
	"$(INTDIR)\jdpred.sbr" \
	"$(INTDIR)\jdsample.sbr" \
	"$(INTDIR)\jdscale.sbr" \
	"$(INTDIR)\jdshuff.sbr" \
	"$(INTDIR)\jerror.sbr" \
	"$(INTDIR)\jfdctflt.sbr" \
	"$(INTDIR)\jfdctfst.sbr" \
	"$(INTDIR)\jfdctint.sbr" \
	"$(INTDIR)\jidctflt.sbr" \
	"$(INTDIR)\jidctfst.sbr" \
	"$(INTDIR)\jidctint.sbr" \
	"$(INTDIR)\jidctred.sbr" \
	"$(INTDIR)\jmemansi.sbr" \
	"$(INTDIR)\jmemmgr.sbr" \
	"$(INTDIR)\jquant1.sbr" \
	"$(INTDIR)\jquant2.sbr" \
	"$(INTDIR)\jutils.sbr" \
	"$(INTDIR)\keys.sbr" \
	"$(INTDIR)\m_flash.sbr" \
	"$(INTDIR)\md4.sbr" \
	"$(INTDIR)\menu.sbr" \
	"$(INTDIR)\model.sbr" \
	"$(INTDIR)\net_chan.sbr" \
	"$(INTDIR)\net_wins.sbr" \
	"$(INTDIR)\pmove.sbr" \
	"$(INTDIR)\q_shared2.sbr" \
	"$(INTDIR)\q_shwin.sbr" \
	"$(INTDIR)\qmenu.sbr" \
	"$(INTDIR)\snd_dma.sbr" \
	"$(INTDIR)\snd_mem.sbr" \
	"$(INTDIR)\snd_mix.sbr" \
	"$(INTDIR)\snd_win.sbr" \
	"$(INTDIR)\sv_ccmds.sbr" \
	"$(INTDIR)\sv_ents.sbr" \
	"$(INTDIR)\sv_game.sbr" \
	"$(INTDIR)\sv_init.sbr" \
	"$(INTDIR)\sv_main.sbr" \
	"$(INTDIR)\sv_send.sbr" \
	"$(INTDIR)\sv_user.sbr" \
	"$(INTDIR)\sv_world.sbr" \
	"$(INTDIR)\sys_win.sbr" \
	"$(INTDIR)\vid_dll.sbr" \
	"$(INTDIR)\vid_menu.sbr" \
	"$(INTDIR)\x86.sbr" \
	"$(INTDIR)\zip.sbr" \
	"$(INTDIR)\zutil.sbr"

"$(OUTDIR)\quake2.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=winmm.lib wsock32.lib kernel32.lib user32.lib gdi32.lib /nologo\
 /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\quake2.pdb"\
 /map:"$(INTDIR)\quake2.map" /debug /machine:I386 /out:"$(OUTDIR)\quake2.exe" 
LINK32_OBJS= \
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\cd_win.obj" \
	"$(INTDIR)\cl_cin.obj" \
	"$(INTDIR)\cl_ents.obj" \
	"$(INTDIR)\cl_fx.obj" \
	"$(INTDIR)\cl_input.obj" \
	"$(INTDIR)\cl_inv.obj" \
	"$(INTDIR)\cl_main.obj" \
	"$(INTDIR)\cl_newfx.obj" \
	"$(INTDIR)\cl_parse.obj" \
	"$(INTDIR)\cl_pred.obj" \
	"$(INTDIR)\cl_scrn.obj" \
	"$(INTDIR)\cl_tent.obj" \
	"$(INTDIR)\cl_view.obj" \
	"$(INTDIR)\cmd.obj" \
	"$(INTDIR)\cmodel.obj" \
	"$(INTDIR)\common.obj" \
	"$(INTDIR)\conproc.obj" \
	"$(INTDIR)\console.obj" \
	"$(INTDIR)\crc.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\cvar.obj" \
	"$(INTDIR)\files.obj" \
	"$(INTDIR)\images.obj" \
	"$(INTDIR)\in_win.obj" \
	"$(INTDIR)\infblock.obj" \
	"$(INTDIR)\infcodes.obj" \
	"$(INTDIR)\inffast.obj" \
	"$(INTDIR)\inflate.obj" \
	"$(INTDIR)\inftrees.obj" \
	"$(INTDIR)\infutil.obj" \
	"$(INTDIR)\jccoefct.obj" \
	"$(INTDIR)\jcdctmgr.obj" \
	"$(INTDIR)\jcdiffct.obj" \
	"$(INTDIR)\jchuff.obj" \
	"$(INTDIR)\jclhuff.obj" \
	"$(INTDIR)\jclossls.obj" \
	"$(INTDIR)\jclossy.obj" \
	"$(INTDIR)\jcodec.obj" \
	"$(INTDIR)\jcomapi.obj" \
	"$(INTDIR)\jcphuff.obj" \
	"$(INTDIR)\jcpred.obj" \
	"$(INTDIR)\jcscale.obj" \
	"$(INTDIR)\jcshuff.obj" \
	"$(INTDIR)\jdapimin.obj" \
	"$(INTDIR)\jdapistd.obj" \
	"$(INTDIR)\jdcoefct.obj" \
	"$(INTDIR)\jdcolor.obj" \
	"$(INTDIR)\jddctmgr.obj" \
	"$(INTDIR)\jddiffct.obj" \
	"$(INTDIR)\jdhuff.obj" \
	"$(INTDIR)\jdinput.obj" \
	"$(INTDIR)\jdlhuff.obj" \
	"$(INTDIR)\jdlossls.obj" \
	"$(INTDIR)\jdlossy.obj" \
	"$(INTDIR)\jdmainct.obj" \
	"$(INTDIR)\jdmarker.obj" \
	"$(INTDIR)\jdmaster.obj" \
	"$(INTDIR)\jdmerge.obj" \
	"$(INTDIR)\jdphuff.obj" \
	"$(INTDIR)\jdpostct.obj" \
	"$(INTDIR)\jdpred.obj" \
	"$(INTDIR)\jdsample.obj" \
	"$(INTDIR)\jdscale.obj" \
	"$(INTDIR)\jdshuff.obj" \
	"$(INTDIR)\jerror.obj" \
	"$(INTDIR)\jfdctflt.obj" \
	"$(INTDIR)\jfdctfst.obj" \
	"$(INTDIR)\jfdctint.obj" \
	"$(INTDIR)\jidctflt.obj" \
	"$(INTDIR)\jidctfst.obj" \
	"$(INTDIR)\jidctint.obj" \
	"$(INTDIR)\jidctred.obj" \
	"$(INTDIR)\jmemansi.obj" \
	"$(INTDIR)\jmemmgr.obj" \
	"$(INTDIR)\jquant1.obj" \
	"$(INTDIR)\jquant2.obj" \
	"$(INTDIR)\jutils.obj" \
	"$(INTDIR)\keys.obj" \
	"$(INTDIR)\m_flash.obj" \
	"$(INTDIR)\md4.obj" \
	"$(INTDIR)\menu.obj" \
	"$(INTDIR)\model.obj" \
	"$(INTDIR)\net_chan.obj" \
	"$(INTDIR)\net_wins.obj" \
	"$(INTDIR)\pmove.obj" \
	"$(INTDIR)\q2.res" \
	"$(INTDIR)\q_shared2.obj" \
	"$(INTDIR)\q_shwin.obj" \
	"$(INTDIR)\qmenu.obj" \
	"$(INTDIR)\snd_dma.obj" \
	"$(INTDIR)\snd_mem.obj" \
	"$(INTDIR)\snd_mix.obj" \
	"$(INTDIR)\snd_win.obj" \
	"$(INTDIR)\sv_ccmds.obj" \
	"$(INTDIR)\sv_ents.obj" \
	"$(INTDIR)\sv_game.obj" \
	"$(INTDIR)\sv_init.obj" \
	"$(INTDIR)\sv_main.obj" \
	"$(INTDIR)\sv_send.obj" \
	"$(INTDIR)\sv_user.obj" \
	"$(INTDIR)\sv_world.obj" \
	"$(INTDIR)\sys_win.obj" \
	"$(INTDIR)\vid_dll.obj" \
	"$(INTDIR)\vid_menu.obj" \
	"$(INTDIR)\x86.obj" \
	"$(INTDIR)\zip.obj" \
	"$(INTDIR)\zutil.obj" \
	"$(OUTDIR)\ref_gl.lib" \
	"$(OUTDIR)\ref_soft.lib"

"$(OUTDIR)\quake2.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "quake2 - Win32 Release" || "$(CFG)" == "quake2 - Win32 Debug"
SOURCE=.\win32\cd_win.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CD_WI=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cd_win.obj" : $(SOURCE) $(DEP_CPP_CD_WI) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CD_WI=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cd_win.obj"	"$(INTDIR)\cd_win.sbr" : $(SOURCE) $(DEP_CPP_CD_WI)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_cin.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CL_CI=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_cin.obj" : $(SOURCE) $(DEP_CPP_CL_CI) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CL_CI=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_cin.obj"	"$(INTDIR)\cl_cin.sbr" : $(SOURCE) $(DEP_CPP_CL_CI)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_ents.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CL_EN=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_ents.obj" : $(SOURCE) $(DEP_CPP_CL_EN) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CL_EN=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_ents.obj"	"$(INTDIR)\cl_ents.sbr" : $(SOURCE) $(DEP_CPP_CL_EN)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_fx.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CL_FX=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_fx.obj" : $(SOURCE) $(DEP_CPP_CL_FX) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CL_FX=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_fx.obj"	"$(INTDIR)\cl_fx.sbr" : $(SOURCE) $(DEP_CPP_CL_FX)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_input.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CL_IN=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_input.obj" : $(SOURCE) $(DEP_CPP_CL_IN) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CL_IN=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_input.obj"	"$(INTDIR)\cl_input.sbr" : $(SOURCE) $(DEP_CPP_CL_IN)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_inv.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CL_INV=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_inv.obj" : $(SOURCE) $(DEP_CPP_CL_INV) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CL_INV=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_inv.obj"	"$(INTDIR)\cl_inv.sbr" : $(SOURCE) $(DEP_CPP_CL_INV)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_main.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CL_MA=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_main.obj" : $(SOURCE) $(DEP_CPP_CL_MA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CL_MA=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_main.obj"	"$(INTDIR)\cl_main.sbr" : $(SOURCE) $(DEP_CPP_CL_MA)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_newfx.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CL_NE=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_newfx.obj" : $(SOURCE) $(DEP_CPP_CL_NE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CL_NE=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_newfx.obj"	"$(INTDIR)\cl_newfx.sbr" : $(SOURCE) $(DEP_CPP_CL_NE)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_parse.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CL_PA=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_parse.obj" : $(SOURCE) $(DEP_CPP_CL_PA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CL_PA=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_parse.obj"	"$(INTDIR)\cl_parse.sbr" : $(SOURCE) $(DEP_CPP_CL_PA)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_pred.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CL_PR=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_pred.obj" : $(SOURCE) $(DEP_CPP_CL_PR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CL_PR=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_pred.obj"	"$(INTDIR)\cl_pred.sbr" : $(SOURCE) $(DEP_CPP_CL_PR)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_scrn.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CL_SC=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_scrn.obj" : $(SOURCE) $(DEP_CPP_CL_SC) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CL_SC=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_scrn.obj"	"$(INTDIR)\cl_scrn.sbr" : $(SOURCE) $(DEP_CPP_CL_SC)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_tent.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CL_TE=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_tent.obj" : $(SOURCE) $(DEP_CPP_CL_TE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CL_TE=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_tent.obj"	"$(INTDIR)\cl_tent.sbr" : $(SOURCE) $(DEP_CPP_CL_TE)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_view.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CL_VI=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_view.obj" : $(SOURCE) $(DEP_CPP_CL_VI) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CL_VI=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cl_view.obj"	"$(INTDIR)\cl_view.sbr" : $(SOURCE) $(DEP_CPP_CL_VI)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\cmd.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CMD_C=\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cmd.obj" : $(SOURCE) $(DEP_CPP_CMD_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CMD_C=\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cmd.obj"	"$(INTDIR)\cmd.sbr" : $(SOURCE) $(DEP_CPP_CMD_C)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\cmodel.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CMODE=\
	".\client\ref.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cmodel.obj" : $(SOURCE) $(DEP_CPP_CMODE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CMODE=\
	".\client\ref.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cmodel.obj"	"$(INTDIR)\cmodel.sbr" : $(SOURCE) $(DEP_CPP_CMODE)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\common.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_COMMO=\
	".\client\anorms.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\common.obj" : $(SOURCE) $(DEP_CPP_COMMO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_COMMO=\
	".\client\anorms.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\common.obj"	"$(INTDIR)\common.sbr" : $(SOURCE) $(DEP_CPP_COMMO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\conproc.c
DEP_CPP_CONPR=\
	".\win32\conproc.h"\
	

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\conproc.obj" : $(SOURCE) $(DEP_CPP_CONPR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\conproc.obj"	"$(INTDIR)\conproc.sbr" : $(SOURCE) $(DEP_CPP_CONPR)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\console.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CONSO=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\console.obj" : $(SOURCE) $(DEP_CPP_CONSO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CONSO=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\console.obj"	"$(INTDIR)\console.sbr" : $(SOURCE) $(DEP_CPP_CONSO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\crc.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CRC_C=\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\crc.obj" : $(SOURCE) $(DEP_CPP_CRC_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CRC_C=\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\crc.obj"	"$(INTDIR)\crc.sbr" : $(SOURCE) $(DEP_CPP_CRC_C)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\cvar.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CVAR_=\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cvar.obj" : $(SOURCE) $(DEP_CPP_CVAR_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CVAR_=\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\cvar.obj"	"$(INTDIR)\cvar.sbr" : $(SOURCE) $(DEP_CPP_CVAR_)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\files.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_FILES=\
	".\client\console.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\zip\zconf.h"\
	".\zip\zip.h"\
	".\zip\zlib.h"\
	

"$(INTDIR)\files.obj" : $(SOURCE) $(DEP_CPP_FILES) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_FILES=\
	".\client\console.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\zip\zconf.h"\
	".\zip\zip.h"\
	".\zip\zlib.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\files.obj"	"$(INTDIR)\files.sbr" : $(SOURCE) $(DEP_CPP_FILES)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\images.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_IMAGE=\
	".\jpeg\jconfig.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpeglib.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\images.obj" : $(SOURCE) $(DEP_CPP_IMAGE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_IMAGE=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\images.obj"	"$(INTDIR)\images.sbr" : $(SOURCE) $(DEP_CPP_IMAGE)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\in_win.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_IN_WI=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\win32\winquake.h"\
	

"$(INTDIR)\in_win.obj" : $(SOURCE) $(DEP_CPP_IN_WI) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_IN_WI=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\win32\winquake.h"\
	

"$(INTDIR)\in_win.obj"	"$(INTDIR)\in_win.sbr" : $(SOURCE) $(DEP_CPP_IN_WI)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\keys.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_KEYS_=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\keys.obj" : $(SOURCE) $(DEP_CPP_KEYS_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_KEYS_=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\keys.obj"	"$(INTDIR)\keys.sbr" : $(SOURCE) $(DEP_CPP_KEYS_)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\game\m_flash.c
DEP_CPP_M_FLA=\
	".\game\q_shared.h"\
	

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\m_flash.obj" : $(SOURCE) $(DEP_CPP_M_FLA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\m_flash.obj"	"$(INTDIR)\m_flash.sbr" : $(SOURCE) $(DEP_CPP_M_FLA)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\md4.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\md4.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\md4.obj"	"$(INTDIR)\md4.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\menu.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_MENU_=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\qmenu.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\menu.obj" : $(SOURCE) $(DEP_CPP_MENU_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_MENU_=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\qmenu.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\menu.obj"	"$(INTDIR)\menu.sbr" : $(SOURCE) $(DEP_CPP_MENU_)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\model.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_MODEL=\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\model.obj" : $(SOURCE) $(DEP_CPP_MODEL) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_MODEL=\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\model.obj"	"$(INTDIR)\model.sbr" : $(SOURCE) $(DEP_CPP_MODEL)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\net_chan.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_NET_C=\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\net_chan.obj" : $(SOURCE) $(DEP_CPP_NET_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_NET_C=\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\net_chan.obj"	"$(INTDIR)\net_chan.sbr" : $(SOURCE) $(DEP_CPP_NET_C)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\net_wins.c
DEP_CPP_NET_W=\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\net_wins.obj" : $(SOURCE) $(DEP_CPP_NET_W) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\net_wins.obj"	"$(INTDIR)\net_wins.sbr" : $(SOURCE) $(DEP_CPP_NET_W)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\pmove.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_PMOVE=\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\pmove.obj" : $(SOURCE) $(DEP_CPP_PMOVE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_PMOVE=\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\pmove.obj"	"$(INTDIR)\pmove.sbr" : $(SOURCE) $(DEP_CPP_PMOVE)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\q_shared2.c
DEP_CPP_Q_SHA=\
	".\qcommon\q_shared2.h"\
	

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\q_shared2.obj" : $(SOURCE) $(DEP_CPP_Q_SHA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\q_shared2.obj"	"$(INTDIR)\q_shared2.sbr" : $(SOURCE)\
 $(DEP_CPP_Q_SHA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\q_shwin.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_Q_SHW=\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\win32\winquake.h"\
	

"$(INTDIR)\q_shwin.obj" : $(SOURCE) $(DEP_CPP_Q_SHW) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_Q_SHW=\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\win32\winquake.h"\
	

"$(INTDIR)\q_shwin.obj"	"$(INTDIR)\q_shwin.sbr" : $(SOURCE) $(DEP_CPP_Q_SHW)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\qmenu.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_QMENU=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\qmenu.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\qmenu.obj" : $(SOURCE) $(DEP_CPP_QMENU) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_QMENU=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\qmenu.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\qmenu.obj"	"$(INTDIR)\qmenu.sbr" : $(SOURCE) $(DEP_CPP_QMENU)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\snd_dma.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_SND_D=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\snd_loc.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\snd_dma.obj" : $(SOURCE) $(DEP_CPP_SND_D) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_SND_D=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\snd_loc.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\snd_dma.obj"	"$(INTDIR)\snd_dma.sbr" : $(SOURCE) $(DEP_CPP_SND_D)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\snd_mem.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_SND_M=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\snd_loc.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\snd_mem.obj" : $(SOURCE) $(DEP_CPP_SND_M) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_SND_M=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\snd_loc.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\snd_mem.obj"	"$(INTDIR)\snd_mem.sbr" : $(SOURCE) $(DEP_CPP_SND_M)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\snd_mix.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_SND_MI=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\snd_loc.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\snd_mix.obj" : $(SOURCE) $(DEP_CPP_SND_MI) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_SND_MI=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\snd_loc.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\snd_mix.obj"	"$(INTDIR)\snd_mix.sbr" : $(SOURCE) $(DEP_CPP_SND_MI)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\snd_win.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_SND_W=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\snd_loc.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\win32\winquake.h"\
	

"$(INTDIR)\snd_win.obj" : $(SOURCE) $(DEP_CPP_SND_W) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_SND_W=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\snd_loc.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\win32\winquake.h"\
	

"$(INTDIR)\snd_win.obj"	"$(INTDIR)\snd_win.sbr" : $(SOURCE) $(DEP_CPP_SND_W)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_ccmds.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_SV_CC=\
	".\game\game.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\server\server.h"\
	

"$(INTDIR)\sv_ccmds.obj" : $(SOURCE) $(DEP_CPP_SV_CC) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_SV_CC=\
	".\game\game.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\server\server.h"\
	

"$(INTDIR)\sv_ccmds.obj"	"$(INTDIR)\sv_ccmds.sbr" : $(SOURCE) $(DEP_CPP_SV_CC)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_ents.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_SV_EN=\
	".\game\game.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\server\server.h"\
	

"$(INTDIR)\sv_ents.obj" : $(SOURCE) $(DEP_CPP_SV_EN) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_SV_EN=\
	".\game\game.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\server\server.h"\
	

"$(INTDIR)\sv_ents.obj"	"$(INTDIR)\sv_ents.sbr" : $(SOURCE) $(DEP_CPP_SV_EN)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_game.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_SV_GA=\
	".\game\game.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\server\server.h"\
	

"$(INTDIR)\sv_game.obj" : $(SOURCE) $(DEP_CPP_SV_GA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_SV_GA=\
	".\game\game.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\server\server.h"\
	

"$(INTDIR)\sv_game.obj"	"$(INTDIR)\sv_game.sbr" : $(SOURCE) $(DEP_CPP_SV_GA)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_init.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_SV_IN=\
	".\game\game.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\server\server.h"\
	

"$(INTDIR)\sv_init.obj" : $(SOURCE) $(DEP_CPP_SV_IN) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_SV_IN=\
	".\game\game.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\server\server.h"\
	

"$(INTDIR)\sv_init.obj"	"$(INTDIR)\sv_init.sbr" : $(SOURCE) $(DEP_CPP_SV_IN)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_main.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_SV_MA=\
	".\game\game.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\server\server.h"\
	

"$(INTDIR)\sv_main.obj" : $(SOURCE) $(DEP_CPP_SV_MA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_SV_MA=\
	".\game\game.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\server\server.h"\
	

"$(INTDIR)\sv_main.obj"	"$(INTDIR)\sv_main.sbr" : $(SOURCE) $(DEP_CPP_SV_MA)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_send.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_SV_SE=\
	".\game\game.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\server\server.h"\
	

"$(INTDIR)\sv_send.obj" : $(SOURCE) $(DEP_CPP_SV_SE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_SV_SE=\
	".\game\game.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\server\server.h"\
	

"$(INTDIR)\sv_send.obj"	"$(INTDIR)\sv_send.sbr" : $(SOURCE) $(DEP_CPP_SV_SE)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_user.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_SV_US=\
	".\game\game.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\server\server.h"\
	

"$(INTDIR)\sv_user.obj" : $(SOURCE) $(DEP_CPP_SV_US) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_SV_US=\
	".\game\game.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\server\server.h"\
	

"$(INTDIR)\sv_user.obj"	"$(INTDIR)\sv_user.sbr" : $(SOURCE) $(DEP_CPP_SV_US)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_world.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_SV_WO=\
	".\game\game.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\server\server.h"\
	

"$(INTDIR)\sv_world.obj" : $(SOURCE) $(DEP_CPP_SV_WO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_SV_WO=\
	".\game\game.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\server\server.h"\
	

"$(INTDIR)\sv_world.obj"	"$(INTDIR)\sv_world.sbr" : $(SOURCE) $(DEP_CPP_SV_WO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\sys_win.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_SYS_W=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\win32\conproc.h"\
	".\win32\winquake.h"\
	

"$(INTDIR)\sys_win.obj" : $(SOURCE) $(DEP_CPP_SYS_W) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_SYS_W=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\win32\conproc.h"\
	".\win32\winquake.h"\
	

"$(INTDIR)\sys_win.obj"	"$(INTDIR)\sys_win.sbr" : $(SOURCE) $(DEP_CPP_SYS_W)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\vid_dll.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_VID_D=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\win32\winquake.h"\
	

"$(INTDIR)\vid_dll.obj" : $(SOURCE) $(DEP_CPP_VID_D) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_VID_D=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	".\win32\winquake.h"\
	

"$(INTDIR)\vid_dll.obj"	"$(INTDIR)\vid_dll.sbr" : $(SOURCE) $(DEP_CPP_VID_D)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\vid_menu.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_VID_M=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\qmenu.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\vid_menu.obj" : $(SOURCE) $(DEP_CPP_VID_M) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_VID_M=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\qmenu.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\vid_menu.obj"	"$(INTDIR)\vid_menu.sbr" : $(SOURCE) $(DEP_CPP_VID_M)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\x86.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_X86_C=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\x86.obj" : $(SOURCE) $(DEP_CPP_X86_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_X86_C=\
	".\client\cdaudio.h"\
	".\client\client.h"\
	".\client\console.h"\
	".\client\input.h"\
	".\client\keys.h"\
	".\client\ref.h"\
	".\client\screen.h"\
	".\client\sound.h"\
	".\client\vid.h"\
	".\qcommon\q_shared2.h"\
	".\qcommon\qcommon.h"\
	".\qcommon\qfiles.h"\
	

"$(INTDIR)\x86.obj"	"$(INTDIR)\x86.sbr" : $(SOURCE) $(DEP_CPP_X86_C)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\q2.rc

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\q2.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\q2.res" /i "win32" /d "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\q2.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\q2.res" /i "win32" /d "_DEBUG" $(SOURCE)


!ENDIF 

SOURCE=.\zip\adler32.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_ADLER=\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	

"$(INTDIR)\adler32.obj" : $(SOURCE) $(DEP_CPP_ADLER) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_ADLER=\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\adler32.obj"	"$(INTDIR)\adler32.sbr" : $(SOURCE) $(DEP_CPP_ADLER)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\crc32.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_CRC32=\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	

"$(INTDIR)\crc32.obj" : $(SOURCE) $(DEP_CPP_CRC32) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_CRC32=\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\crc32.obj"	"$(INTDIR)\crc32.sbr" : $(SOURCE) $(DEP_CPP_CRC32)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\infblock.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_INFBL=\
	".\zip\infblock.h"\
	".\zip\infcodes.h"\
	".\zip\inftrees.h"\
	".\zip\infutil.h"\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	".\zip\zutil.h"\
	

"$(INTDIR)\infblock.obj" : $(SOURCE) $(DEP_CPP_INFBL) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_INFBL=\
	".\zip\infblock.h"\
	".\zip\infcodes.h"\
	".\zip\inftrees.h"\
	".\zip\infutil.h"\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	".\zip\zutil.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\infblock.obj"	"$(INTDIR)\infblock.sbr" : $(SOURCE) $(DEP_CPP_INFBL)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\infcodes.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_INFCO=\
	".\zip\infblock.h"\
	".\zip\infcodes.h"\
	".\zip\inffast.h"\
	".\zip\inftrees.h"\
	".\zip\infutil.h"\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	".\zip\zutil.h"\
	

"$(INTDIR)\infcodes.obj" : $(SOURCE) $(DEP_CPP_INFCO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_INFCO=\
	".\zip\infblock.h"\
	".\zip\infcodes.h"\
	".\zip\inffast.h"\
	".\zip\inftrees.h"\
	".\zip\infutil.h"\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	".\zip\zutil.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\infcodes.obj"	"$(INTDIR)\infcodes.sbr" : $(SOURCE) $(DEP_CPP_INFCO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\inffast.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_INFFA=\
	".\zip\infblock.h"\
	".\zip\infcodes.h"\
	".\zip\inffast.h"\
	".\zip\inftrees.h"\
	".\zip\infutil.h"\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	".\zip\zutil.h"\
	

"$(INTDIR)\inffast.obj" : $(SOURCE) $(DEP_CPP_INFFA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_INFFA=\
	".\zip\infblock.h"\
	".\zip\infcodes.h"\
	".\zip\inffast.h"\
	".\zip\inftrees.h"\
	".\zip\infutil.h"\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	".\zip\zutil.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\inffast.obj"	"$(INTDIR)\inffast.sbr" : $(SOURCE) $(DEP_CPP_INFFA)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\inflate.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_INFLA=\
	".\zip\infblock.h"\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	".\zip\zutil.h"\
	

"$(INTDIR)\inflate.obj" : $(SOURCE) $(DEP_CPP_INFLA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_INFLA=\
	".\zip\infblock.h"\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	".\zip\zutil.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\inflate.obj"	"$(INTDIR)\inflate.sbr" : $(SOURCE) $(DEP_CPP_INFLA)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\inftrees.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_INFTR=\
	".\zip\inffixed.h"\
	".\zip\inftrees.h"\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	".\zip\zutil.h"\
	

"$(INTDIR)\inftrees.obj" : $(SOURCE) $(DEP_CPP_INFTR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_INFTR=\
	".\zip\inffixed.h"\
	".\zip\inftrees.h"\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	".\zip\zutil.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\inftrees.obj"	"$(INTDIR)\inftrees.sbr" : $(SOURCE) $(DEP_CPP_INFTR)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\infutil.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_INFUT=\
	".\zip\infblock.h"\
	".\zip\infcodes.h"\
	".\zip\inftrees.h"\
	".\zip\infutil.h"\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	".\zip\zutil.h"\
	

"$(INTDIR)\infutil.obj" : $(SOURCE) $(DEP_CPP_INFUT) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_INFUT=\
	".\zip\infblock.h"\
	".\zip\infcodes.h"\
	".\zip\inftrees.h"\
	".\zip\infutil.h"\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	".\zip\zutil.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\infutil.obj"	"$(INTDIR)\infutil.sbr" : $(SOURCE) $(DEP_CPP_INFUT)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\zip.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_ZIP_C=\
	".\zip\zconf.h"\
	".\zip\zip.h"\
	".\zip\zlib.h"\
	

"$(INTDIR)\zip.obj" : $(SOURCE) $(DEP_CPP_ZIP_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_ZIP_C=\
	".\zip\zconf.h"\
	".\zip\zip.h"\
	".\zip\zlib.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\zip.obj"	"$(INTDIR)\zip.sbr" : $(SOURCE) $(DEP_CPP_ZIP_C)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\zutil.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_ZUTIL=\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	".\zip\zutil.h"\
	

"$(INTDIR)\zutil.obj" : $(SOURCE) $(DEP_CPP_ZUTIL) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_ZUTIL=\
	".\zip\zconf.h"\
	".\zip\zlib.h"\
	".\zip\zutil.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\zutil.obj"	"$(INTDIR)\zutil.sbr" : $(SOURCE) $(DEP_CPP_ZUTIL)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\jpeg\jccoefct.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JCCOE=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jccoefct.obj" : $(SOURCE) $(DEP_CPP_JCCOE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JCCOE=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jccoefct.obj"	"$(INTDIR)\jccoefct.sbr" : $(SOURCE) $(DEP_CPP_JCCOE)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jcdctmgr.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JCDCT=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jcdctmgr.obj" : $(SOURCE) $(DEP_CPP_JCDCT) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JCDCT=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jcdctmgr.obj"	"$(INTDIR)\jcdctmgr.sbr" : $(SOURCE) $(DEP_CPP_JCDCT)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jcdiffct.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JCDIF=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jcdiffct.obj" : $(SOURCE) $(DEP_CPP_JCDIF) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JCDIF=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jcdiffct.obj"	"$(INTDIR)\jcdiffct.sbr" : $(SOURCE) $(DEP_CPP_JCDIF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jchuff.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JCHUF=\
	".\jpeg\jchuff.h"\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jchuff.obj" : $(SOURCE) $(DEP_CPP_JCHUF) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JCHUF=\
	".\jpeg\jchuff.h"\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jchuff.obj"	"$(INTDIR)\jchuff.sbr" : $(SOURCE) $(DEP_CPP_JCHUF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jclhuff.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JCLHU=\
	".\jpeg\jchuff.h"\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jclhuff.obj" : $(SOURCE) $(DEP_CPP_JCLHU) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JCLHU=\
	".\jpeg\jchuff.h"\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jclhuff.obj"	"$(INTDIR)\jclhuff.sbr" : $(SOURCE) $(DEP_CPP_JCLHU)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jclossls.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JCLOS=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jclossls.obj" : $(SOURCE) $(DEP_CPP_JCLOS) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JCLOS=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jclossls.obj"	"$(INTDIR)\jclossls.sbr" : $(SOURCE) $(DEP_CPP_JCLOS)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jclossy.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JCLOSS=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jclossy.obj" : $(SOURCE) $(DEP_CPP_JCLOSS) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JCLOSS=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jclossy.obj"	"$(INTDIR)\jclossy.sbr" : $(SOURCE) $(DEP_CPP_JCLOSS)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jcodec.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JCODE=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jcodec.obj" : $(SOURCE) $(DEP_CPP_JCODE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JCODE=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jcodec.obj"	"$(INTDIR)\jcodec.sbr" : $(SOURCE) $(DEP_CPP_JCODE)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jcomapi.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JCOMA=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jcomapi.obj" : $(SOURCE) $(DEP_CPP_JCOMA) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JCOMA=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jcomapi.obj"	"$(INTDIR)\jcomapi.sbr" : $(SOURCE) $(DEP_CPP_JCOMA)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jcphuff.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JCPHU=\
	".\jpeg\jchuff.h"\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jcphuff.obj" : $(SOURCE) $(DEP_CPP_JCPHU) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JCPHU=\
	".\jpeg\jchuff.h"\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jcphuff.obj"	"$(INTDIR)\jcphuff.sbr" : $(SOURCE) $(DEP_CPP_JCPHU)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jcpred.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JCPRE=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jcpred.obj" : $(SOURCE) $(DEP_CPP_JCPRE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JCPRE=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jcpred.obj"	"$(INTDIR)\jcpred.sbr" : $(SOURCE) $(DEP_CPP_JCPRE)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jcscale.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JCSCA=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jcscale.obj" : $(SOURCE) $(DEP_CPP_JCSCA) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JCSCA=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jcscale.obj"	"$(INTDIR)\jcscale.sbr" : $(SOURCE) $(DEP_CPP_JCSCA)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jcshuff.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JCSHU=\
	".\jpeg\jchuff.h"\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jcshuff.obj" : $(SOURCE) $(DEP_CPP_JCSHU) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JCSHU=\
	".\jpeg\jchuff.h"\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jcshuff.obj"	"$(INTDIR)\jcshuff.sbr" : $(SOURCE) $(DEP_CPP_JCSHU)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdapimin.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDAPI=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdapimin.obj" : $(SOURCE) $(DEP_CPP_JDAPI) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDAPI=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdapimin.obj"	"$(INTDIR)\jdapimin.sbr" : $(SOURCE) $(DEP_CPP_JDAPI)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdapistd.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDAPIS=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdapistd.obj" : $(SOURCE) $(DEP_CPP_JDAPIS) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDAPIS=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdapistd.obj"	"$(INTDIR)\jdapistd.sbr" : $(SOURCE) $(DEP_CPP_JDAPIS)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdcoefct.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDCOE=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdcoefct.obj" : $(SOURCE) $(DEP_CPP_JDCOE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDCOE=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdcoefct.obj"	"$(INTDIR)\jdcoefct.sbr" : $(SOURCE) $(DEP_CPP_JDCOE)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdcolor.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDCOL=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdcolor.obj" : $(SOURCE) $(DEP_CPP_JDCOL) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDCOL=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdcolor.obj"	"$(INTDIR)\jdcolor.sbr" : $(SOURCE) $(DEP_CPP_JDCOL)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jddctmgr.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDDCT=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jddctmgr.obj" : $(SOURCE) $(DEP_CPP_JDDCT) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDDCT=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jddctmgr.obj"	"$(INTDIR)\jddctmgr.sbr" : $(SOURCE) $(DEP_CPP_JDDCT)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jddiffct.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDDIF=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jddiffct.obj" : $(SOURCE) $(DEP_CPP_JDDIF) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDDIF=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jddiffct.obj"	"$(INTDIR)\jddiffct.sbr" : $(SOURCE) $(DEP_CPP_JDDIF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdhuff.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDHUF=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdhuff.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdhuff.obj" : $(SOURCE) $(DEP_CPP_JDHUF) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDHUF=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdhuff.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdhuff.obj"	"$(INTDIR)\jdhuff.sbr" : $(SOURCE) $(DEP_CPP_JDHUF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdinput.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDINP=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdinput.obj" : $(SOURCE) $(DEP_CPP_JDINP) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDINP=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdinput.obj"	"$(INTDIR)\jdinput.sbr" : $(SOURCE) $(DEP_CPP_JDINP)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdlhuff.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDLHU=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdhuff.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdlhuff.obj" : $(SOURCE) $(DEP_CPP_JDLHU) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDLHU=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdhuff.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdlhuff.obj"	"$(INTDIR)\jdlhuff.sbr" : $(SOURCE) $(DEP_CPP_JDLHU)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdlossls.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDLOS=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdlossls.obj" : $(SOURCE) $(DEP_CPP_JDLOS) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDLOS=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdlossls.obj"	"$(INTDIR)\jdlossls.sbr" : $(SOURCE) $(DEP_CPP_JDLOS)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdlossy.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDLOSS=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdlossy.obj" : $(SOURCE) $(DEP_CPP_JDLOSS) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDLOSS=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdlossy.obj"	"$(INTDIR)\jdlossy.sbr" : $(SOURCE) $(DEP_CPP_JDLOSS)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdmainct.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDMAI=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdmainct.obj" : $(SOURCE) $(DEP_CPP_JDMAI) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDMAI=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdmainct.obj"	"$(INTDIR)\jdmainct.sbr" : $(SOURCE) $(DEP_CPP_JDMAI)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdmarker.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDMAR=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdmarker.obj" : $(SOURCE) $(DEP_CPP_JDMAR) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDMAR=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdmarker.obj"	"$(INTDIR)\jdmarker.sbr" : $(SOURCE) $(DEP_CPP_JDMAR)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdmaster.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDMAS=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdmaster.obj" : $(SOURCE) $(DEP_CPP_JDMAS) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDMAS=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdmaster.obj"	"$(INTDIR)\jdmaster.sbr" : $(SOURCE) $(DEP_CPP_JDMAS)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdmerge.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDMER=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdmerge.obj" : $(SOURCE) $(DEP_CPP_JDMER) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDMER=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdmerge.obj"	"$(INTDIR)\jdmerge.sbr" : $(SOURCE) $(DEP_CPP_JDMER)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdphuff.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDPHU=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdhuff.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdphuff.obj" : $(SOURCE) $(DEP_CPP_JDPHU) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDPHU=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdhuff.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdphuff.obj"	"$(INTDIR)\jdphuff.sbr" : $(SOURCE) $(DEP_CPP_JDPHU)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdpostct.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDPOS=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdpostct.obj" : $(SOURCE) $(DEP_CPP_JDPOS) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDPOS=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdpostct.obj"	"$(INTDIR)\jdpostct.sbr" : $(SOURCE) $(DEP_CPP_JDPOS)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdpred.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDPRE=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdpred.obj" : $(SOURCE) $(DEP_CPP_JDPRE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDPRE=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdpred.obj"	"$(INTDIR)\jdpred.sbr" : $(SOURCE) $(DEP_CPP_JDPRE)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdsample.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDSAM=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdsample.obj" : $(SOURCE) $(DEP_CPP_JDSAM) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDSAM=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdsample.obj"	"$(INTDIR)\jdsample.sbr" : $(SOURCE) $(DEP_CPP_JDSAM)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdscale.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDSCA=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdscale.obj" : $(SOURCE) $(DEP_CPP_JDSCA) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDSCA=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossls.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdscale.obj"	"$(INTDIR)\jdscale.sbr" : $(SOURCE) $(DEP_CPP_JDSCA)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdshuff.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JDSHU=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdhuff.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jdshuff.obj" : $(SOURCE) $(DEP_CPP_JDSHU) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JDSHU=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdhuff.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jlossy.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jdshuff.obj"	"$(INTDIR)\jdshuff.sbr" : $(SOURCE) $(DEP_CPP_JDSHU)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jerror.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JERRO=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpeglib.h"\
	".\jpeg\jversion.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jerror.obj" : $(SOURCE) $(DEP_CPP_JERRO) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JERRO=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	".\jpeg\jversion.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jerror.obj"	"$(INTDIR)\jerror.sbr" : $(SOURCE) $(DEP_CPP_JERRO)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jfdctflt.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JFDCT=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jfdctflt.obj" : $(SOURCE) $(DEP_CPP_JFDCT) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JFDCT=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jfdctflt.obj"	"$(INTDIR)\jfdctflt.sbr" : $(SOURCE) $(DEP_CPP_JFDCT)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jfdctfst.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JFDCTF=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jfdctfst.obj" : $(SOURCE) $(DEP_CPP_JFDCTF) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JFDCTF=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jfdctfst.obj"	"$(INTDIR)\jfdctfst.sbr" : $(SOURCE) $(DEP_CPP_JFDCTF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jfdctint.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JFDCTI=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jfdctint.obj" : $(SOURCE) $(DEP_CPP_JFDCTI) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JFDCTI=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jfdctint.obj"	"$(INTDIR)\jfdctint.sbr" : $(SOURCE) $(DEP_CPP_JFDCTI)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jidctflt.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JIDCT=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jidctflt.obj" : $(SOURCE) $(DEP_CPP_JIDCT) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JIDCT=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jidctflt.obj"	"$(INTDIR)\jidctflt.sbr" : $(SOURCE) $(DEP_CPP_JIDCT)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jidctfst.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JIDCTF=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jidctfst.obj" : $(SOURCE) $(DEP_CPP_JIDCTF) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JIDCTF=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jidctfst.obj"	"$(INTDIR)\jidctfst.sbr" : $(SOURCE) $(DEP_CPP_JIDCTF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jidctint.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JIDCTI=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jidctint.obj" : $(SOURCE) $(DEP_CPP_JIDCTI) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JIDCTI=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jidctint.obj"	"$(INTDIR)\jidctint.sbr" : $(SOURCE) $(DEP_CPP_JIDCTI)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jidctred.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JIDCTR=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jidctred.obj" : $(SOURCE) $(DEP_CPP_JIDCTR) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JIDCTR=\
	".\jpeg\jconfig.h"\
	".\jpeg\jdct.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jidctred.obj"	"$(INTDIR)\jidctred.sbr" : $(SOURCE) $(DEP_CPP_JIDCTR)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jmemansi.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JMEMA=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmemsys.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jmemansi.obj" : $(SOURCE) $(DEP_CPP_JMEMA) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JMEMA=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmemsys.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jmemansi.obj"	"$(INTDIR)\jmemansi.sbr" : $(SOURCE) $(DEP_CPP_JMEMA)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jmemmgr.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JMEMM=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmemsys.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jmemmgr.obj" : $(SOURCE) $(DEP_CPP_JMEMM) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JMEMM=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmemsys.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jmemmgr.obj"	"$(INTDIR)\jmemmgr.sbr" : $(SOURCE) $(DEP_CPP_JMEMM)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jquant1.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JQUAN=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jquant1.obj" : $(SOURCE) $(DEP_CPP_JQUAN) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JQUAN=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jquant1.obj"	"$(INTDIR)\jquant1.sbr" : $(SOURCE) $(DEP_CPP_JQUAN)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jquant2.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JQUANT=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jquant2.obj" : $(SOURCE) $(DEP_CPP_JQUANT) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JQUANT=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jquant2.obj"	"$(INTDIR)\jquant2.sbr" : $(SOURCE) $(DEP_CPP_JQUANT)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jutils.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

DEP_CPP_JUTIL=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	
CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\jutils.obj" : $(SOURCE) $(DEP_CPP_JUTIL) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

DEP_CPP_JUTIL=\
	".\jpeg\jconfig.h"\
	".\jpeg\jerror.h"\
	".\jpeg\jinclude.h"\
	".\jpeg\jmorecfg.h"\
	".\jpeg\jpegint.h"\
	".\jpeg\jpeglib.h"\
	{$(INCLUDE)}"sys\types.h"\
	
CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /ZI /c 

"$(INTDIR)\jutils.obj"	"$(INTDIR)\jutils.sbr" : $(SOURCE) $(DEP_CPP_JUTIL)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

!IF  "$(CFG)" == "quake2 - Win32 Release"

"ref_gl - Win32 Release" : 
   cd ".\ref_gl"
   $(MAKE) /$(MAKEFLAGS) /F .\ref_gl.mak CFG="ref_gl - Win32 Release" 
   cd ".."

"ref_gl - Win32 ReleaseCLEAN" : 
   cd ".\ref_gl"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F .\ref_gl.mak CFG="ref_gl - Win32 Release"\
 RECURSE=1 
   cd ".."

!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

"ref_gl - Win32 Debug" : 
   cd ".\ref_gl"
   $(MAKE) /$(MAKEFLAGS) /F .\ref_gl.mak CFG="ref_gl - Win32 Debug" 
   cd ".."

"ref_gl - Win32 DebugCLEAN" : 
   cd ".\ref_gl"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F .\ref_gl.mak CFG="ref_gl - Win32 Debug"\
 RECURSE=1 
   cd ".."

!ENDIF 

!IF  "$(CFG)" == "quake2 - Win32 Release"

"ref_soft - Win32 Release" : 
   cd ".\ref_soft"
   $(MAKE) /$(MAKEFLAGS) /F .\ref_soft.mak CFG="ref_soft - Win32 Release" 
   cd ".."

"ref_soft - Win32 ReleaseCLEAN" : 
   cd ".\ref_soft"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F .\ref_soft.mak CFG="ref_soft - Win32 Release"\
 RECURSE=1 
   cd ".."

!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

"ref_soft - Win32 Debug" : 
   cd ".\ref_soft"
   $(MAKE) /$(MAKEFLAGS) /F .\ref_soft.mak CFG="ref_soft - Win32 Debug" 
   cd ".."

"ref_soft - Win32 DebugCLEAN" : 
   cd ".\ref_soft"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F .\ref_soft.mak CFG="ref_soft - Win32 Debug"\
 RECURSE=1 
   cd ".."

!ENDIF 


!ENDIF 

