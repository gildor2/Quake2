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

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "quake2 - Win32 Release"

OUTDIR=.\release
INTDIR=.\release
# Begin Custom Macros
OutDir=.\release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\quake2.exe"

!ELSE 

ALL : "ref_soft - Win32 Release" "ref_gl - Win32 Release" "$(OUTDIR)\quake2.exe"

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
	-@erase "$(INTDIR)\memory.obj"
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
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vid_dll.obj"
	-@erase "$(INTDIR)\vid_menu.obj"
	-@erase "$(INTDIR)\x86.obj"
	-@erase "$(INTDIR)\zip.obj"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase "$(OUTDIR)\quake2.exe"
	-@erase "$(OUTDIR)\quake2.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /G5 /MD /W3 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\q2.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\quake2.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=winmm.lib wsock32.lib kernel32.lib user32.lib gdi32.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\quake2.pdb" /map:"$(INTDIR)\quake2.map" /machine:I386 /out:"$(OUTDIR)\quake2.exe" /heap:16740352,524288 
LINK32_OBJS= \
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
	"$(INTDIR)\cvar.obj" \
	"$(INTDIR)\files.obj" \
	"$(INTDIR)\images.obj" \
	"$(INTDIR)\in_win.obj" \
	"$(INTDIR)\keys.obj" \
	"$(INTDIR)\m_flash.obj" \
	"$(INTDIR)\md4.obj" \
	"$(INTDIR)\memory.obj" \
	"$(INTDIR)\menu.obj" \
	"$(INTDIR)\model.obj" \
	"$(INTDIR)\net_chan.obj" \
	"$(INTDIR)\net_wins.obj" \
	"$(INTDIR)\pmove.obj" \
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
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\infblock.obj" \
	"$(INTDIR)\infcodes.obj" \
	"$(INTDIR)\inffast.obj" \
	"$(INTDIR)\inflate.obj" \
	"$(INTDIR)\inftrees.obj" \
	"$(INTDIR)\infutil.obj" \
	"$(INTDIR)\zip.obj" \
	"$(INTDIR)\zutil.obj" \
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
	"$(INTDIR)\q2.res" \
	".\ref_soft\release\ref_soft.lib"

"$(OUTDIR)\quake2.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

OUTDIR=.\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\quake2.exe" "$(OUTDIR)\quake2.bsc"

!ELSE 

ALL : "ref_soft - Win32 Debug" "ref_gl - Win32 Debug" "$(OUTDIR)\quake2.exe" "$(OUTDIR)\quake2.bsc"

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
	-@erase "$(INTDIR)\memory.obj"
	-@erase "$(INTDIR)\memory.sbr"
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
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
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

CPP_PROJ=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\q2.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\quake2.bsc" 
BSC32_SBRS= \
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
	"$(INTDIR)\cvar.sbr" \
	"$(INTDIR)\files.sbr" \
	"$(INTDIR)\images.sbr" \
	"$(INTDIR)\in_win.sbr" \
	"$(INTDIR)\keys.sbr" \
	"$(INTDIR)\m_flash.sbr" \
	"$(INTDIR)\md4.sbr" \
	"$(INTDIR)\memory.sbr" \
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
	"$(INTDIR)\adler32.sbr" \
	"$(INTDIR)\crc32.sbr" \
	"$(INTDIR)\infblock.sbr" \
	"$(INTDIR)\infcodes.sbr" \
	"$(INTDIR)\inffast.sbr" \
	"$(INTDIR)\inflate.sbr" \
	"$(INTDIR)\inftrees.sbr" \
	"$(INTDIR)\infutil.sbr" \
	"$(INTDIR)\zip.sbr" \
	"$(INTDIR)\zutil.sbr" \
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
	"$(INTDIR)\jutils.sbr"

"$(OUTDIR)\quake2.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=winmm.lib wsock32.lib kernel32.lib user32.lib gdi32.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\quake2.pdb" /map:"$(INTDIR)\quake2.map" /debug /machine:I386 /out:"$(OUTDIR)\quake2.exe" 
LINK32_OBJS= \
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
	"$(INTDIR)\cvar.obj" \
	"$(INTDIR)\files.obj" \
	"$(INTDIR)\images.obj" \
	"$(INTDIR)\in_win.obj" \
	"$(INTDIR)\keys.obj" \
	"$(INTDIR)\m_flash.obj" \
	"$(INTDIR)\md4.obj" \
	"$(INTDIR)\memory.obj" \
	"$(INTDIR)\menu.obj" \
	"$(INTDIR)\model.obj" \
	"$(INTDIR)\net_chan.obj" \
	"$(INTDIR)\net_wins.obj" \
	"$(INTDIR)\pmove.obj" \
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
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\infblock.obj" \
	"$(INTDIR)\infcodes.obj" \
	"$(INTDIR)\inffast.obj" \
	"$(INTDIR)\inflate.obj" \
	"$(INTDIR)\inftrees.obj" \
	"$(INTDIR)\infutil.obj" \
	"$(INTDIR)\zip.obj" \
	"$(INTDIR)\zutil.obj" \
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
	"$(INTDIR)\q2.res"

"$(OUTDIR)\quake2.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

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


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("quake2.dep")
!INCLUDE "quake2.dep"
!ELSE 
!MESSAGE Warning: cannot find "quake2.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "quake2 - Win32 Release" || "$(CFG)" == "quake2 - Win32 Debug"
SOURCE=.\win32\cd_win.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\cd_win.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\cd_win.obj"	"$(INTDIR)\cd_win.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_cin.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\cl_cin.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\cl_cin.obj"	"$(INTDIR)\cl_cin.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_ents.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\cl_ents.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\cl_ents.obj"	"$(INTDIR)\cl_ents.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_fx.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\cl_fx.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\cl_fx.obj"	"$(INTDIR)\cl_fx.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_input.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\cl_input.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\cl_input.obj"	"$(INTDIR)\cl_input.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_inv.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\cl_inv.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\cl_inv.obj"	"$(INTDIR)\cl_inv.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_main.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\cl_main.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\cl_main.obj"	"$(INTDIR)\cl_main.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_newfx.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\cl_newfx.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\cl_newfx.obj"	"$(INTDIR)\cl_newfx.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_parse.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\cl_parse.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\cl_parse.obj"	"$(INTDIR)\cl_parse.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_pred.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\cl_pred.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\cl_pred.obj"	"$(INTDIR)\cl_pred.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_scrn.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\cl_scrn.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\cl_scrn.obj"	"$(INTDIR)\cl_scrn.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_tent.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\cl_tent.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\cl_tent.obj"	"$(INTDIR)\cl_tent.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_view.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\cl_view.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\cl_view.obj"	"$(INTDIR)\cl_view.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\cmd.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\cmd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\cmd.obj"	"$(INTDIR)\cmd.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\cmodel.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\cmodel.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\cmodel.obj"	"$(INTDIR)\cmodel.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\common.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\common.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\common.obj"	"$(INTDIR)\common.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\conproc.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\conproc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\conproc.obj"	"$(INTDIR)\conproc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\console.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\console.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\console.obj"	"$(INTDIR)\console.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\crc.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\crc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\crc.obj"	"$(INTDIR)\crc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\cvar.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\cvar.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\cvar.obj"	"$(INTDIR)\cvar.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\files.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\files.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\files.obj"	"$(INTDIR)\files.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\images.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\images.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\images.obj"	"$(INTDIR)\images.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\in_win.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\in_win.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\in_win.obj"	"$(INTDIR)\in_win.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\keys.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\keys.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\keys.obj"	"$(INTDIR)\keys.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\game\m_flash.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\m_flash.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\m_flash.obj"	"$(INTDIR)\m_flash.sbr" : $(SOURCE) "$(INTDIR)"
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

SOURCE=.\qcommon\memory.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\memory.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\memory.obj"	"$(INTDIR)\memory.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\menu.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\menu.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\menu.obj"	"$(INTDIR)\menu.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\model.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\model.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\model.obj"	"$(INTDIR)\model.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\net_chan.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\net_chan.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\net_chan.obj"	"$(INTDIR)\net_chan.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\net_wins.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\net_wins.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\net_wins.obj"	"$(INTDIR)\net_wins.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\pmove.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\pmove.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\pmove.obj"	"$(INTDIR)\pmove.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\q_shared2.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\q_shared2.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\q_shared2.obj"	"$(INTDIR)\q_shared2.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\q_shwin.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\q_shwin.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\q_shwin.obj"	"$(INTDIR)\q_shwin.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\qmenu.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\qmenu.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\qmenu.obj"	"$(INTDIR)\qmenu.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\snd_dma.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\snd_dma.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\snd_dma.obj"	"$(INTDIR)\snd_dma.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\snd_mem.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\snd_mem.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\snd_mem.obj"	"$(INTDIR)\snd_mem.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\snd_mix.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\snd_mix.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\snd_mix.obj"	"$(INTDIR)\snd_mix.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\snd_win.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\snd_win.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\snd_win.obj"	"$(INTDIR)\snd_win.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_ccmds.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\sv_ccmds.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\sv_ccmds.obj"	"$(INTDIR)\sv_ccmds.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_ents.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\sv_ents.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\sv_ents.obj"	"$(INTDIR)\sv_ents.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_game.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\sv_game.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\sv_game.obj"	"$(INTDIR)\sv_game.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_init.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\sv_init.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\sv_init.obj"	"$(INTDIR)\sv_init.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_main.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\sv_main.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\sv_main.obj"	"$(INTDIR)\sv_main.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_send.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\sv_send.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\sv_send.obj"	"$(INTDIR)\sv_send.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_user.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\sv_user.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\sv_user.obj"	"$(INTDIR)\sv_user.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_world.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\sv_world.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\sv_world.obj"	"$(INTDIR)\sv_world.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\sys_win.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\sys_win.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\sys_win.obj"	"$(INTDIR)\sys_win.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\vid_dll.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\vid_dll.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\vid_dll.obj"	"$(INTDIR)\vid_dll.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\vid_menu.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\vid_menu.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\vid_menu.obj"	"$(INTDIR)\vid_menu.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\x86.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\x86.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\x86.obj"	"$(INTDIR)\x86.sbr" : $(SOURCE) "$(INTDIR)"
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


"$(INTDIR)\adler32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\adler32.obj"	"$(INTDIR)\adler32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\crc32.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\crc32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\crc32.obj"	"$(INTDIR)\crc32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\infblock.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\infblock.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\infblock.obj"	"$(INTDIR)\infblock.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\infcodes.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\infcodes.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\infcodes.obj"	"$(INTDIR)\infcodes.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\inffast.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\inffast.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\inffast.obj"	"$(INTDIR)\inffast.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\inflate.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\inflate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\inflate.obj"	"$(INTDIR)\inflate.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\inftrees.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\inftrees.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\inftrees.obj"	"$(INTDIR)\inftrees.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\infutil.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\infutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\infutil.obj"	"$(INTDIR)\infutil.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\zip.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\zip.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\zip.obj"	"$(INTDIR)\zip.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\zip\zutil.c

!IF  "$(CFG)" == "quake2 - Win32 Release"


"$(INTDIR)\zutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"


"$(INTDIR)\zutil.obj"	"$(INTDIR)\zutil.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\jpeg\jccoefct.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jccoefct.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jccoefct.obj"	"$(INTDIR)\jccoefct.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jcdctmgr.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jcdctmgr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jcdctmgr.obj"	"$(INTDIR)\jcdctmgr.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jcdiffct.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jcdiffct.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jcdiffct.obj"	"$(INTDIR)\jcdiffct.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jchuff.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jchuff.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jchuff.obj"	"$(INTDIR)\jchuff.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jclhuff.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jclhuff.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jclhuff.obj"	"$(INTDIR)\jclhuff.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jclossls.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jclossls.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jclossls.obj"	"$(INTDIR)\jclossls.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jclossy.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jclossy.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jclossy.obj"	"$(INTDIR)\jclossy.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jcodec.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jcodec.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jcodec.obj"	"$(INTDIR)\jcodec.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jcomapi.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jcomapi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jcomapi.obj"	"$(INTDIR)\jcomapi.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jcphuff.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jcphuff.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jcphuff.obj"	"$(INTDIR)\jcphuff.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jcpred.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jcpred.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jcpred.obj"	"$(INTDIR)\jcpred.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jcscale.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jcscale.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jcscale.obj"	"$(INTDIR)\jcscale.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jcshuff.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jcshuff.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jcshuff.obj"	"$(INTDIR)\jcshuff.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdapimin.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdapimin.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdapimin.obj"	"$(INTDIR)\jdapimin.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdapistd.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdapistd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdapistd.obj"	"$(INTDIR)\jdapistd.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdcoefct.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdcoefct.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdcoefct.obj"	"$(INTDIR)\jdcoefct.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdcolor.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdcolor.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdcolor.obj"	"$(INTDIR)\jdcolor.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jddctmgr.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jddctmgr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jddctmgr.obj"	"$(INTDIR)\jddctmgr.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jddiffct.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jddiffct.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jddiffct.obj"	"$(INTDIR)\jddiffct.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdhuff.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdhuff.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdhuff.obj"	"$(INTDIR)\jdhuff.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdinput.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdinput.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdinput.obj"	"$(INTDIR)\jdinput.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdlhuff.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdlhuff.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdlhuff.obj"	"$(INTDIR)\jdlhuff.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdlossls.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdlossls.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdlossls.obj"	"$(INTDIR)\jdlossls.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdlossy.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdlossy.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdlossy.obj"	"$(INTDIR)\jdlossy.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdmainct.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdmainct.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdmainct.obj"	"$(INTDIR)\jdmainct.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdmarker.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdmarker.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdmarker.obj"	"$(INTDIR)\jdmarker.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdmaster.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdmaster.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdmaster.obj"	"$(INTDIR)\jdmaster.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdmerge.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdmerge.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdmerge.obj"	"$(INTDIR)\jdmerge.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdphuff.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdphuff.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdphuff.obj"	"$(INTDIR)\jdphuff.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdpostct.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdpostct.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdpostct.obj"	"$(INTDIR)\jdpostct.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdpred.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdpred.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdpred.obj"	"$(INTDIR)\jdpred.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdsample.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdsample.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdsample.obj"	"$(INTDIR)\jdsample.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdscale.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdscale.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdscale.obj"	"$(INTDIR)\jdscale.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jdshuff.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdshuff.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jdshuff.obj"	"$(INTDIR)\jdshuff.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jerror.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jerror.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jerror.obj"	"$(INTDIR)\jerror.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jfdctflt.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jfdctflt.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jfdctflt.obj"	"$(INTDIR)\jfdctflt.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jfdctfst.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jfdctfst.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jfdctfst.obj"	"$(INTDIR)\jfdctfst.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jfdctint.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jfdctint.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jfdctint.obj"	"$(INTDIR)\jfdctint.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jidctflt.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jidctflt.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jidctflt.obj"	"$(INTDIR)\jidctflt.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jidctfst.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jidctfst.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jidctfst.obj"	"$(INTDIR)\jidctfst.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jidctint.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jidctint.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jidctint.obj"	"$(INTDIR)\jidctint.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jidctred.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jidctred.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jidctred.obj"	"$(INTDIR)\jidctred.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jmemansi.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jmemansi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jmemansi.obj"	"$(INTDIR)\jmemansi.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jmemmgr.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jmemmgr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jmemmgr.obj"	"$(INTDIR)\jmemmgr.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jquant1.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jquant1.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jquant1.obj"	"$(INTDIR)\jquant1.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jquant2.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jquant2.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jquant2.obj"	"$(INTDIR)\jquant2.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\jpeg\jutils.c

!IF  "$(CFG)" == "quake2 - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W1 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jutils.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\jutils.obj"	"$(INTDIR)\jutils.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

!IF  "$(CFG)" == "quake2 - Win32 Release"

"ref_gl - Win32 Release" : 
   cd ".\ref_gl.old"
   $(MAKE) /$(MAKEFLAGS) /F .\ref_gl.mak CFG="ref_gl - Win32 Release" 
   cd ".."

"ref_gl - Win32 ReleaseCLEAN" : 
   cd ".\ref_gl.old"
   $(MAKE) /$(MAKEFLAGS) /F .\ref_gl.mak CFG="ref_gl - Win32 Release" RECURSE=1 CLEAN 
   cd ".."

!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

"ref_gl - Win32 Debug" : 
   cd ".\ref_gl.old"
   $(MAKE) /$(MAKEFLAGS) /F .\ref_gl.mak CFG="ref_gl - Win32 Debug" 
   cd ".."

"ref_gl - Win32 DebugCLEAN" : 
   cd ".\ref_gl.old"
   $(MAKE) /$(MAKEFLAGS) /F .\ref_gl.mak CFG="ref_gl - Win32 Debug" RECURSE=1 CLEAN 
   cd ".."

!ENDIF 

!IF  "$(CFG)" == "quake2 - Win32 Release"

"ref_soft - Win32 Release" : 
   cd ".\ref_soft"
   $(MAKE) /$(MAKEFLAGS) /F .\ref_soft.mak CFG="ref_soft - Win32 Release" 
   cd ".."

"ref_soft - Win32 ReleaseCLEAN" : 
   cd ".\ref_soft"
   $(MAKE) /$(MAKEFLAGS) /F .\ref_soft.mak CFG="ref_soft - Win32 Release" RECURSE=1 CLEAN 
   cd ".."

!ELSEIF  "$(CFG)" == "quake2 - Win32 Debug"

"ref_soft - Win32 Debug" : 
   cd ".\ref_soft"
   $(MAKE) /$(MAKEFLAGS) /F .\ref_soft.mak CFG="ref_soft - Win32 Debug" 
   cd ".."

"ref_soft - Win32 DebugCLEAN" : 
   cd ".\ref_soft"
   $(MAKE) /$(MAKEFLAGS) /F .\ref_soft.mak CFG="ref_soft - Win32 Debug" RECURSE=1 CLEAN 
   cd ".."

!ENDIF 


!ENDIF 

