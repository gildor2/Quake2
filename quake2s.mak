# Microsoft Developer Studio Generated NMAKE File, Based on quake2s.dsp
!IF "$(CFG)" == ""
CFG=quake2s - Win32 Release
!MESSAGE No configuration specified. Defaulting to quake2s - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "quake2s - Win32 Release" && "$(CFG)" != "quake2s - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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

!IF  "$(CFG)" == "quake2s - Win32 Release"

OUTDIR=.\release
INTDIR=.\release
# Begin Custom Macros
OutDir=.\release
# End Custom Macros

ALL : "$(OUTDIR)\quake2.exe"


CLEAN :
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
	-@erase "$(INTDIR)\cvar.obj"
	-@erase "$(INTDIR)\files.obj"
	-@erase "$(INTDIR)\gl_backend.obj"
	-@erase "$(INTDIR)\gl_buffers.obj"
	-@erase "$(INTDIR)\gl_frontend.obj"
	-@erase "$(INTDIR)\gl_image.obj"
	-@erase "$(INTDIR)\gl_interface.obj"
	-@erase "$(INTDIR)\gl_light.obj"
	-@erase "$(INTDIR)\gl_lightmap.obj"
	-@erase "$(INTDIR)\gl_main.obj"
	-@erase "$(INTDIR)\gl_math.obj"
	-@erase "$(INTDIR)\gl_model.obj"
	-@erase "$(INTDIR)\gl_poly.obj"
	-@erase "$(INTDIR)\gl_shader.obj"
	-@erase "$(INTDIR)\gl_sky.obj"
	-@erase "$(INTDIR)\glw_imp.obj"
	-@erase "$(INTDIR)\images.obj"
	-@erase "$(INTDIR)\in_win.obj"
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
	-@erase "$(INTDIR)\qgl_win.obj"
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
	-@erase "$(INTDIR)\zip.obj"
	-@erase "$(OUTDIR)\quake2.exe"
	-@erase "$(OUTDIR)\quake2.map"
	-@erase ".\release\q_shwin.obj"
	-@erase ".\release\r_aclip.obj"
	-@erase ".\release\r_alias.obj"
	-@erase ".\release\r_bsp.obj"
	-@erase ".\release\r_draw.obj"
	-@erase ".\release\r_edge.obj"
	-@erase ".\release\r_image.obj"
	-@erase ".\release\r_light.obj"
	-@erase ".\release\r_main.obj"
	-@erase ".\release\r_misc.obj"
	-@erase ".\release\r_model.obj"
	-@erase ".\release\r_part.obj"
	-@erase ".\release\r_poly.obj"
	-@erase ".\release\r_polyse.obj"
	-@erase ".\release\r_rast.obj"
	-@erase ".\release\r_scan.obj"
	-@erase ".\release\r_sprite.obj"
	-@erase ".\release\r_surf.obj"
	-@erase ".\release\rw_ddraw.obj"
	-@erase ".\release\rw_dib.obj"
	-@erase ".\release\rw_imp.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\q2.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\quake2s.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=winmm.lib wsock32.lib kernel32.lib user32.lib gdi32.lib dinput.lib lib\lib.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\quake2.pdb" /map:"$(INTDIR)\quake2.map" /machine:I386 /out:"$(OUTDIR)\quake2.exe" /heap:16740352,524288 /filealign:512 
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
	"$(INTDIR)\zip.obj" \
	"$(INTDIR)\gl_backend.obj" \
	"$(INTDIR)\gl_buffers.obj" \
	"$(INTDIR)\gl_frontend.obj" \
	"$(INTDIR)\gl_image.obj" \
	"$(INTDIR)\gl_interface.obj" \
	"$(INTDIR)\gl_light.obj" \
	"$(INTDIR)\gl_lightmap.obj" \
	"$(INTDIR)\gl_main.obj" \
	"$(INTDIR)\gl_math.obj" \
	"$(INTDIR)\gl_model.obj" \
	"$(INTDIR)\gl_poly.obj" \
	"$(INTDIR)\gl_shader.obj" \
	"$(INTDIR)\gl_sky.obj" \
	"$(INTDIR)\glw_imp.obj" \
	"$(INTDIR)\qgl_win.obj" \
	".\release\q_shwin.obj" \
	".\release\r_aclip.obj" \
	".\release\r_alias.obj" \
	".\release\r_bsp.obj" \
	".\release\r_draw.obj" \
	".\release\r_edge.obj" \
	".\release\r_image.obj" \
	".\release\r_light.obj" \
	".\release\r_main.obj" \
	".\release\r_misc.obj" \
	".\release\r_model.obj" \
	".\release\r_part.obj" \
	".\release\r_poly.obj" \
	".\release\r_polyse.obj" \
	".\release\r_rast.obj" \
	".\release\r_scan.obj" \
	".\release\r_sprite.obj" \
	".\release\r_surf.obj" \
	".\release\rw_ddraw.obj" \
	".\release\rw_dib.obj" \
	".\release\rw_imp.obj" \
	"$(INTDIR)\q2.res" \
	"$(INTDIR)\resources.obj"

"$(OUTDIR)\quake2.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

OUTDIR=.\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\debug
# End Custom Macros

ALL : "$(OUTDIR)\quake2.exe" "$(OUTDIR)\quake2s.bsc"


CLEAN :
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
	-@erase "$(INTDIR)\cvar.obj"
	-@erase "$(INTDIR)\cvar.sbr"
	-@erase "$(INTDIR)\files.obj"
	-@erase "$(INTDIR)\files.sbr"
	-@erase "$(INTDIR)\gl_backend.obj"
	-@erase "$(INTDIR)\gl_backend.sbr"
	-@erase "$(INTDIR)\gl_buffers.obj"
	-@erase "$(INTDIR)\gl_buffers.sbr"
	-@erase "$(INTDIR)\gl_frontend.obj"
	-@erase "$(INTDIR)\gl_frontend.sbr"
	-@erase "$(INTDIR)\gl_image.obj"
	-@erase "$(INTDIR)\gl_image.sbr"
	-@erase "$(INTDIR)\gl_interface.obj"
	-@erase "$(INTDIR)\gl_interface.sbr"
	-@erase "$(INTDIR)\gl_light.obj"
	-@erase "$(INTDIR)\gl_light.sbr"
	-@erase "$(INTDIR)\gl_lightmap.obj"
	-@erase "$(INTDIR)\gl_lightmap.sbr"
	-@erase "$(INTDIR)\gl_main.obj"
	-@erase "$(INTDIR)\gl_main.sbr"
	-@erase "$(INTDIR)\gl_math.obj"
	-@erase "$(INTDIR)\gl_math.sbr"
	-@erase "$(INTDIR)\gl_model.obj"
	-@erase "$(INTDIR)\gl_model.sbr"
	-@erase "$(INTDIR)\gl_poly.obj"
	-@erase "$(INTDIR)\gl_poly.sbr"
	-@erase "$(INTDIR)\gl_shader.obj"
	-@erase "$(INTDIR)\gl_shader.sbr"
	-@erase "$(INTDIR)\gl_sky.obj"
	-@erase "$(INTDIR)\gl_sky.sbr"
	-@erase "$(INTDIR)\glw_imp.obj"
	-@erase "$(INTDIR)\glw_imp.sbr"
	-@erase "$(INTDIR)\images.obj"
	-@erase "$(INTDIR)\images.sbr"
	-@erase "$(INTDIR)\in_win.obj"
	-@erase "$(INTDIR)\in_win.sbr"
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
	-@erase "$(INTDIR)\qgl_win.obj"
	-@erase "$(INTDIR)\qgl_win.sbr"
	-@erase "$(INTDIR)\qmenu.obj"
	-@erase "$(INTDIR)\qmenu.sbr"
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
	-@erase "$(INTDIR)\zip.obj"
	-@erase "$(INTDIR)\zip.sbr"
	-@erase "$(OUTDIR)\quake2.exe"
	-@erase "$(OUTDIR)\quake2.map"
	-@erase "$(OUTDIR)\quake2.pdb"
	-@erase "$(OUTDIR)\quake2s.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\q2.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\quake2s.bsc" 
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
	"$(INTDIR)\zip.sbr" \
	"$(INTDIR)\gl_backend.sbr" \
	"$(INTDIR)\gl_buffers.sbr" \
	"$(INTDIR)\gl_frontend.sbr" \
	"$(INTDIR)\gl_image.sbr" \
	"$(INTDIR)\gl_interface.sbr" \
	"$(INTDIR)\gl_light.sbr" \
	"$(INTDIR)\gl_lightmap.sbr" \
	"$(INTDIR)\gl_main.sbr" \
	"$(INTDIR)\gl_math.sbr" \
	"$(INTDIR)\gl_model.sbr" \
	"$(INTDIR)\gl_poly.sbr" \
	"$(INTDIR)\gl_shader.sbr" \
	"$(INTDIR)\gl_sky.sbr" \
	"$(INTDIR)\glw_imp.sbr" \
	"$(INTDIR)\qgl_win.sbr" \
	"$(INTDIR)\q_shwin.sbr" \
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

"$(OUTDIR)\quake2s.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
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
	"$(INTDIR)\zip.obj" \
	"$(INTDIR)\gl_backend.obj" \
	"$(INTDIR)\gl_buffers.obj" \
	"$(INTDIR)\gl_frontend.obj" \
	"$(INTDIR)\gl_image.obj" \
	"$(INTDIR)\gl_interface.obj" \
	"$(INTDIR)\gl_light.obj" \
	"$(INTDIR)\gl_lightmap.obj" \
	"$(INTDIR)\gl_main.obj" \
	"$(INTDIR)\gl_math.obj" \
	"$(INTDIR)\gl_model.obj" \
	"$(INTDIR)\gl_poly.obj" \
	"$(INTDIR)\gl_shader.obj" \
	"$(INTDIR)\gl_sky.obj" \
	"$(INTDIR)\glw_imp.obj" \
	"$(INTDIR)\qgl_win.obj" \
	"$(INTDIR)\q_shwin.obj" \
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
	"$(INTDIR)\q2.res" \
	"$(INTDIR)\r_aclipa.obj" \
	"$(INTDIR)\r_draw16.obj" \
	"$(INTDIR)\r_drawa.obj" \
	"$(INTDIR)\r_edgea.obj" \
	"$(INTDIR)\r_polysa.obj" \
	"$(INTDIR)\r_scana.obj" \
	"$(INTDIR)\r_spr8.obj" \
	"$(INTDIR)\r_surf8.obj" \
	"$(INTDIR)\r_varsa.obj"

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
!IF EXISTS("quake2s.dep")
!INCLUDE "quake2s.dep"
!ELSE 
!MESSAGE Warning: cannot find "quake2s.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "quake2s - Win32 Release" || "$(CFG)" == "quake2s - Win32 Debug"
SOURCE=.\win32\cd_win.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\cd_win.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\cd_win.obj"	"$(INTDIR)\cd_win.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_cin.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\cl_cin.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\cl_cin.obj"	"$(INTDIR)\cl_cin.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_ents.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\cl_ents.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\cl_ents.obj"	"$(INTDIR)\cl_ents.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_fx.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\cl_fx.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\cl_fx.obj"	"$(INTDIR)\cl_fx.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_input.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\cl_input.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\cl_input.obj"	"$(INTDIR)\cl_input.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_inv.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\cl_inv.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\cl_inv.obj"	"$(INTDIR)\cl_inv.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_main.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\cl_main.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\cl_main.obj"	"$(INTDIR)\cl_main.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_newfx.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\cl_newfx.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\cl_newfx.obj"	"$(INTDIR)\cl_newfx.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_parse.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\cl_parse.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\cl_parse.obj"	"$(INTDIR)\cl_parse.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_pred.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\cl_pred.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\cl_pred.obj"	"$(INTDIR)\cl_pred.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_scrn.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\cl_scrn.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\cl_scrn.obj"	"$(INTDIR)\cl_scrn.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_tent.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\cl_tent.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\cl_tent.obj"	"$(INTDIR)\cl_tent.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\cl_view.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\cl_view.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\cl_view.obj"	"$(INTDIR)\cl_view.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\cmd.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\cmd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\cmd.obj"	"$(INTDIR)\cmd.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\cmodel.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\cmodel.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\cmodel.obj"	"$(INTDIR)\cmodel.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\common.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\common.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\common.obj"	"$(INTDIR)\common.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\conproc.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\conproc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\conproc.obj"	"$(INTDIR)\conproc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\console.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\console.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\console.obj"	"$(INTDIR)\console.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\crc.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\crc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\crc.obj"	"$(INTDIR)\crc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\cvar.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\cvar.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\cvar.obj"	"$(INTDIR)\cvar.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\files.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\files.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\files.obj"	"$(INTDIR)\files.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\images.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\images.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\images.obj"	"$(INTDIR)\images.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\in_win.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\in_win.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\in_win.obj"	"$(INTDIR)\in_win.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\keys.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\keys.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\keys.obj"	"$(INTDIR)\keys.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\game\m_flash.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\m_flash.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\m_flash.obj"	"$(INTDIR)\m_flash.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\md4.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\md4.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\md4.obj"	"$(INTDIR)\md4.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\memory.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\memory.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\memory.obj"	"$(INTDIR)\memory.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\menu.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\menu.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\menu.obj"	"$(INTDIR)\menu.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\model.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\model.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\model.obj"	"$(INTDIR)\model.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\net_chan.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\net_chan.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\net_chan.obj"	"$(INTDIR)\net_chan.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\net_wins.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\net_wins.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\net_wins.obj"	"$(INTDIR)\net_wins.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\pmove.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\pmove.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\pmove.obj"	"$(INTDIR)\pmove.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\q_shared2.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\q_shared2.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\q_shared2.obj"	"$(INTDIR)\q_shared2.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\qmenu.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\qmenu.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\qmenu.obj"	"$(INTDIR)\qmenu.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\snd_dma.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\snd_dma.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\snd_dma.obj"	"$(INTDIR)\snd_dma.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\snd_mem.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\snd_mem.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\snd_mem.obj"	"$(INTDIR)\snd_mem.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\client\snd_mix.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\snd_mix.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\snd_mix.obj"	"$(INTDIR)\snd_mix.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\snd_win.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\snd_win.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\snd_win.obj"	"$(INTDIR)\snd_win.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_ccmds.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\sv_ccmds.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\sv_ccmds.obj"	"$(INTDIR)\sv_ccmds.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_ents.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\sv_ents.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\sv_ents.obj"	"$(INTDIR)\sv_ents.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_game.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\sv_game.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\sv_game.obj"	"$(INTDIR)\sv_game.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_init.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\sv_init.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\sv_init.obj"	"$(INTDIR)\sv_init.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_main.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\sv_main.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\sv_main.obj"	"$(INTDIR)\sv_main.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_send.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\sv_send.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\sv_send.obj"	"$(INTDIR)\sv_send.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_user.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\sv_user.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\sv_user.obj"	"$(INTDIR)\sv_user.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\server\sv_world.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\sv_world.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\sv_world.obj"	"$(INTDIR)\sv_world.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\sys_win.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\sys_win.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\sys_win.obj"	"$(INTDIR)\sys_win.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\vid_dll.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\vid_dll.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\vid_dll.obj"	"$(INTDIR)\vid_dll.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\vid_menu.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\vid_menu.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\vid_menu.obj"	"$(INTDIR)\vid_menu.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\qcommon\zip.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\zip.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\zip.obj"	"$(INTDIR)\zip.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\q2.rc

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\q2.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\q2.res" /i "win32" /d "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\q2.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\q2.res" /i "win32" /d "_DEBUG" $(SOURCE)


!ENDIF 

SOURCE=.\ref_gl\gl_backend.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\gl_backend.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\gl_backend.obj"	"$(INTDIR)\gl_backend.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\ref_gl\gl_buffers.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\gl_buffers.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\gl_buffers.obj"	"$(INTDIR)\gl_buffers.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\ref_gl\gl_frontend.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\gl_frontend.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\gl_frontend.obj"	"$(INTDIR)\gl_frontend.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\ref_gl\gl_image.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\gl_image.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\gl_image.obj"	"$(INTDIR)\gl_image.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\ref_gl\gl_interface.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\gl_interface.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\gl_interface.obj"	"$(INTDIR)\gl_interface.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\ref_gl\gl_light.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\gl_light.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\gl_light.obj"	"$(INTDIR)\gl_light.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\ref_gl\gl_lightmap.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\gl_lightmap.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\gl_lightmap.obj"	"$(INTDIR)\gl_lightmap.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\ref_gl\gl_main.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\gl_main.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\gl_main.obj"	"$(INTDIR)\gl_main.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\ref_gl\gl_math.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\gl_math.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\gl_math.obj"	"$(INTDIR)\gl_math.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\ref_gl\gl_model.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\gl_model.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\gl_model.obj"	"$(INTDIR)\gl_model.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\ref_gl\gl_poly.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\gl_poly.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\gl_poly.obj"	"$(INTDIR)\gl_poly.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\ref_gl\gl_shader.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\gl_shader.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\gl_shader.obj"	"$(INTDIR)\gl_shader.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\ref_gl\gl_sky.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\gl_sky.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\gl_sky.obj"	"$(INTDIR)\gl_sky.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\glw_imp.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\glw_imp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\glw_imp.obj"	"$(INTDIR)\glw_imp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\qgl_win.c

!IF  "$(CFG)" == "quake2s - Win32 Release"


"$(INTDIR)\qgl_win.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"


"$(INTDIR)\qgl_win.obj"	"$(INTDIR)\qgl_win.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\win32\q_shwin.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\q_shwin.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\q_shwin.obj"	"$(INTDIR)\q_shwin.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_aclip.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_aclip.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_aclip.obj"	"$(INTDIR)\r_aclip.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_alias.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_alias.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_alias.obj"	"$(INTDIR)\r_alias.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_bsp.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_bsp.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_bsp.obj"	"$(INTDIR)\r_bsp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_draw.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_draw.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_draw.obj"	"$(INTDIR)\r_draw.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_edge.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_edge.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_edge.obj"	"$(INTDIR)\r_edge.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_image.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_image.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_image.obj"	"$(INTDIR)\r_image.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_light.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_light.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_light.obj"	"$(INTDIR)\r_light.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_main.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_main.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_main.obj"	"$(INTDIR)\r_main.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_misc.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_misc.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_misc.obj"	"$(INTDIR)\r_misc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_model.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_model.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_model.obj"	"$(INTDIR)\r_model.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_part.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_part.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_part.obj"	"$(INTDIR)\r_part.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_poly.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_poly.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_poly.obj"	"$(INTDIR)\r_poly.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_polyse.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_polyse.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_polyse.obj"	"$(INTDIR)\r_polyse.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_rast.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_rast.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_rast.obj"	"$(INTDIR)\r_rast.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_scan.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_scan.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_scan.obj"	"$(INTDIR)\r_scan.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_sprite.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_sprite.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_sprite.obj"	"$(INTDIR)\r_sprite.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_surf.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\r_surf.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\r_surf.obj"	"$(INTDIR)\r_surf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\win32\rw_ddraw.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\rw_ddraw.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\rw_ddraw.obj"	"$(INTDIR)\rw_ddraw.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\win32\rw_dib.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\rw_dib.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\rw_dib.obj"	"$(INTDIR)\rw_dib.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\win32\rw_imp.c

!IF  "$(CFG)" == "quake2s - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MD /W3 /GX /Zd /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "REF_HARD_LINKED" /Fo".\release/" /Fd".\release/" /FD /c 

".\release\rw_imp.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

CPP_SWITCHES=/nologo /G5 /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\quake2s.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\rw_imp.obj"	"$(INTDIR)\rw_imp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ref_soft\r_aclipa.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

OutDir=.\debug
InputPath=.\ref_soft\r_aclipa.asm
InputName=r_aclipa

"$(INTDIR)\r_aclipa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\ref_soft\r_draw16.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

OutDir=.\debug
InputPath=.\ref_soft\r_draw16.asm
InputName=r_draw16

"$(INTDIR)\r_draw16.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\ref_soft\r_drawa.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

OutDir=.\debug
InputPath=.\ref_soft\r_drawa.asm
InputName=r_drawa

"$(INTDIR)\r_drawa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\ref_soft\r_edgea.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

OutDir=.\debug
InputPath=.\ref_soft\r_edgea.asm
InputName=r_edgea

"$(INTDIR)\r_edgea.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\ref_soft\r_polysa.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

OutDir=.\debug
InputPath=.\ref_soft\r_polysa.asm
InputName=r_polysa

"$(INTDIR)\r_polysa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\ref_soft\r_scana.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

OutDir=.\debug
InputPath=.\ref_soft\r_scana.asm
InputName=r_scana

"$(INTDIR)\r_scana.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\ref_soft\r_spr8.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

OutDir=.\debug
InputPath=.\ref_soft\r_spr8.asm
InputName=r_spr8

"$(INTDIR)\r_spr8.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\ref_soft\r_surf8.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

OutDir=.\debug
InputPath=.\ref_soft\r_surf8.asm
InputName=r_surf8

"$(INTDIR)\r_surf8.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\ref_soft\r_varsa.asm

!IF  "$(CFG)" == "quake2s - Win32 Release"

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

OutDir=.\debug
InputPath=.\ref_soft\r_varsa.asm
InputName=r_varsa

"$(INTDIR)\r_varsa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	ml /nologo /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(InputPath)
<< 
	

!ENDIF 

SOURCE=.\resource\archive.gz

!IF  "$(CFG)" == "quake2s - Win32 Release"

InputDir=.\resource
IntDir=.\release
InputPath=.\resource\archive.gz

"$(INTDIR)\resources.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	nasm -f win32 -o $(IntDir)\resources.obj -Darc=\"$(InputPath)\" $(InputDir)\make.asm
<< 
	

!ELSEIF  "$(CFG)" == "quake2s - Win32 Debug"

!ENDIF 


!ENDIF 

