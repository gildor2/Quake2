# This file was automatically generated from "quake2.prj": do not edit

#------------------------------------------------------------------------------
#	symbolic targets
#------------------------------------------------------------------------------

ALL : STATIC DYNAMIC DEDICATED
STATIC : Release/quake2.exe
DEDICATED : Release/q2ded.exe
DYNAMIC : Release/quake2_dyn.exe Release/ref_gl.dll

#------------------------------------------------------------------------------
#	"Release/quake2.exe" target
#------------------------------------------------------------------------------

STATIC = \
	Release/obj/CoreMain.obj \
	Release/obj/DbgSymbols.obj \
	Release/obj/Memory.obj \
	Release/obj/OutputDevice.obj \
	Release/obj/ErrorMgr.obj \
	Release/obj/Strings.obj \
	Release/obj/DbgSymbolsWin32.obj \
	Release/obj/ExceptFilterWin32.obj \
	Release/obj/cl_cin.obj \
	Release/obj/cl_download.obj \
	Release/obj/cl_ents.obj \
	Release/obj/cl_fx.obj \
	Release/obj/cl_input.obj \
	Release/obj/cl_main.obj \
	Release/obj/cl_newfx.obj \
	Release/obj/cl_parse.obj \
	Release/obj/cl_pred.obj \
	Release/obj/cl_scrn.obj \
	Release/obj/cl_tent.obj \
	Release/obj/cl_view.obj \
	Release/obj/console.obj \
	Release/obj/keys.obj \
	Release/obj/menu.obj \
	Release/obj/qmenu.obj \
	Release/obj/ref_vars.obj \
	Release/obj/snd_dma.obj \
	Release/obj/snd_mem.obj \
	Release/obj/snd_mix.obj \
	Release/obj/sv_ccmds.obj \
	Release/obj/sv_ents.obj \
	Release/obj/sv_game.obj \
	Release/obj/sv_init.obj \
	Release/obj/sv_main.obj \
	Release/obj/sv_send.obj \
	Release/obj/sv_tokenize.obj \
	Release/obj/sv_user.obj \
	Release/obj/sv_world.obj \
	Release/obj/cmd.obj \
	Release/obj/cmodel.obj \
	Release/obj/common.obj \
	Release/obj/crc.obj \
	Release/obj/cvar.obj \
	Release/obj/files.obj \
	Release/obj/images.obj \
	Release/obj/md4.obj \
	Release/obj/model.obj \
	Release/obj/net_chan.obj \
	Release/obj/pmove.obj \
	Release/obj/q_shared2.obj \
	Release/obj/zip.obj \
	Release/obj/cd_win.obj \
	Release/obj/in_win.obj \
	Release/obj/net_wins.obj \
	Release/obj/q_shwin.obj \
	Release/obj/snd_win.obj \
	Release/obj/sys_win.obj \
	Release/obj/vid_dll.obj \
	Release/obj/vid_menu.obj \
	Release/obj/q2.res \
	Release/obj/makeres.obj \
	Release/obj/gl_backend.obj \
	Release/obj/gl_buffers.obj \
	Release/obj/gl_frontend.obj \
	Release/obj/gl_image.obj \
	Release/obj/gl_interface.obj \
	Release/obj/gl_light.obj \
	Release/obj/gl_lightmap.obj \
	Release/obj/gl_main.obj \
	Release/obj/gl_math.obj \
	Release/obj/gl_model.obj \
	Release/obj/gl_poly.obj \
	Release/obj/gl_shader.obj \
	Release/obj/gl_sky.obj \
	Release/obj/qgl_win.obj \
	Release/obj/glw_imp.obj

Release/quake2.exe : DIRS $(STATIC)
	echo Creating executable "Release/quake2.exe" ...
	link.exe -nologo -filealign:512 -incremental:no -out:"Release/quake2.exe" kernel32.lib user32.lib gdi32.lib winmm.lib wsock32.lib dinput.lib lib/lib.lib -map:"Release/quake2.map" -pdb:none $(STATIC)

#------------------------------------------------------------------------------
#	"Release/quake2_dyn.exe" target
#------------------------------------------------------------------------------

DYN_EXE = \
	Release/obj/dyn_exe/CoreMain.obj \
	Release/obj/dyn_exe/DbgSymbols.obj \
	Release/obj/dyn_exe/Memory.obj \
	Release/obj/dyn_exe/OutputDevice.obj \
	Release/obj/dyn_exe/ErrorMgr.obj \
	Release/obj/dyn_exe/Strings.obj \
	Release/obj/dyn_exe/DbgSymbolsWin32.obj \
	Release/obj/dyn_exe/ExceptFilterWin32.obj \
	Release/obj/dyn_exe/cl_cin.obj \
	Release/obj/dyn_exe/cl_download.obj \
	Release/obj/dyn_exe/cl_ents.obj \
	Release/obj/dyn_exe/cl_fx.obj \
	Release/obj/dyn_exe/cl_input.obj \
	Release/obj/dyn_exe/cl_main.obj \
	Release/obj/dyn_exe/cl_newfx.obj \
	Release/obj/dyn_exe/cl_parse.obj \
	Release/obj/dyn_exe/cl_pred.obj \
	Release/obj/dyn_exe/cl_scrn.obj \
	Release/obj/dyn_exe/cl_tent.obj \
	Release/obj/dyn_exe/cl_view.obj \
	Release/obj/dyn_exe/console.obj \
	Release/obj/dyn_exe/keys.obj \
	Release/obj/dyn_exe/menu.obj \
	Release/obj/dyn_exe/qmenu.obj \
	Release/obj/dyn_exe/ref_vars.obj \
	Release/obj/dyn_exe/snd_dma.obj \
	Release/obj/dyn_exe/snd_mem.obj \
	Release/obj/dyn_exe/snd_mix.obj \
	Release/obj/dyn_exe/sv_ccmds.obj \
	Release/obj/dyn_exe/sv_ents.obj \
	Release/obj/dyn_exe/sv_game.obj \
	Release/obj/dyn_exe/sv_init.obj \
	Release/obj/dyn_exe/sv_main.obj \
	Release/obj/dyn_exe/sv_send.obj \
	Release/obj/dyn_exe/sv_tokenize.obj \
	Release/obj/dyn_exe/sv_user.obj \
	Release/obj/dyn_exe/sv_world.obj \
	Release/obj/dyn_exe/cmd.obj \
	Release/obj/dyn_exe/cmodel.obj \
	Release/obj/dyn_exe/common.obj \
	Release/obj/dyn_exe/crc.obj \
	Release/obj/dyn_exe/cvar.obj \
	Release/obj/dyn_exe/files.obj \
	Release/obj/dyn_exe/images.obj \
	Release/obj/dyn_exe/md4.obj \
	Release/obj/dyn_exe/model.obj \
	Release/obj/dyn_exe/net_chan.obj \
	Release/obj/dyn_exe/pmove.obj \
	Release/obj/dyn_exe/q_shared2.obj \
	Release/obj/dyn_exe/zip.obj \
	Release/obj/dyn_exe/makeres.obj \
	Release/obj/dyn_exe/cd_win.obj \
	Release/obj/dyn_exe/in_win.obj \
	Release/obj/dyn_exe/net_wins.obj \
	Release/obj/dyn_exe/q_shwin.obj \
	Release/obj/dyn_exe/snd_win.obj \
	Release/obj/dyn_exe/sys_win.obj \
	Release/obj/dyn_exe/vid_dll.obj \
	Release/obj/dyn_exe/vid_menu.obj \
	Release/obj/dyn_exe/q2.res

Release/quake2_dyn.exe : DIRS $(DYN_EXE)
	echo Creating executable "Release/quake2_dyn.exe" ...
	link.exe -nologo -filealign:512 -incremental:no -out:"Release/quake2_dyn.exe" kernel32.lib user32.lib gdi32.lib winmm.lib wsock32.lib dinput.lib lib/lib.lib -map:"Release/quake2_dyn.map" -pdb:none $(DYN_EXE)

#------------------------------------------------------------------------------
#	"Release/ref_gl.dll" target
#------------------------------------------------------------------------------

DYN_GL = \
	Release/obj/dyn_ref/gl_backend.obj \
	Release/obj/dyn_ref/gl_buffers.obj \
	Release/obj/dyn_ref/gl_frontend.obj \
	Release/obj/dyn_ref/gl_image.obj \
	Release/obj/dyn_ref/gl_interface.obj \
	Release/obj/dyn_ref/gl_light.obj \
	Release/obj/dyn_ref/gl_lightmap.obj \
	Release/obj/dyn_ref/gl_main.obj \
	Release/obj/dyn_ref/gl_math.obj \
	Release/obj/dyn_ref/gl_model.obj \
	Release/obj/dyn_ref/gl_poly.obj \
	Release/obj/dyn_ref/gl_shader.obj \
	Release/obj/dyn_ref/gl_sky.obj \
	Release/obj/dyn_ref/qgl_win.obj \
	Release/obj/dyn_ref/glw_imp.obj \
	Release/obj/dyn_ref/q_shared2.obj \
	Release/obj/dyn_ref/ref_vars.obj

Release/ref_gl.dll : DIRS $(DYN_GL)
	echo Creating shared "Release/ref_gl.dll" ...
	link.exe -nologo -filealign:512 -incremental:no -out:"Release/ref_gl.dll" kernel32.lib user32.lib gdi32.lib -map:"Release/ref_gl.map" -pdb:none $(DYN_GL) -dll -implib:"delete.lib"
	echo ... and deleting them ...
	del delete.lib
	del delete.exp

#------------------------------------------------------------------------------
#	"Release/q2ded.exe" target
#------------------------------------------------------------------------------

DEDICATED = \
	Release/obj/dedicated/sv_dedicated.obj \
	Release/obj/dedicated/sv_ccmds.obj \
	Release/obj/dedicated/sv_ents.obj \
	Release/obj/dedicated/sv_game.obj \
	Release/obj/dedicated/sv_init.obj \
	Release/obj/dedicated/sv_main.obj \
	Release/obj/dedicated/sv_send.obj \
	Release/obj/dedicated/sv_tokenize.obj \
	Release/obj/dedicated/sv_user.obj \
	Release/obj/dedicated/sv_world.obj \
	Release/obj/dedicated/cmd.obj \
	Release/obj/dedicated/cmodel.obj \
	Release/obj/dedicated/common.obj \
	Release/obj/dedicated/crc.obj \
	Release/obj/dedicated/cvar.obj \
	Release/obj/dedicated/files.obj \
	Release/obj/dedicated/images.obj \
	Release/obj/dedicated/md4.obj \
	Release/obj/dedicated/model.obj \
	Release/obj/dedicated/net_chan.obj \
	Release/obj/dedicated/pmove.obj \
	Release/obj/dedicated/q_shared2.obj \
	Release/obj/dedicated/zip.obj \
	Release/obj/dedicated/net_wins.obj \
	Release/obj/dedicated/q_shwin.obj \
	Release/obj/dedicated/sys_win.obj \
	Release/obj/dedicated/makeres.obj

Release/q2ded.exe : DIRS $(DEDICATED)
	echo Creating executable "Release/q2ded.exe" ...
	link.exe -nologo -filealign:512 -incremental:no -out:"Release/q2ded.exe" kernel32.lib user32.lib gdi32.lib winmm.lib wsock32.lib lib/lib.lib -map:"Release/q2ded.map" -pdb:none $(DEDICATED)

#------------------------------------------------------------------------------
#	compiling source files
#------------------------------------------------------------------------------

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

Release/obj/dyn_exe/md4.obj : qcommon/md4.cpp
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/md4.obj" qcommon/md4.cpp

Release/obj/dyn_exe/q2.res : win32/q2.rc
	rc.exe -l 0x409 -i win32/ -fo"Release/obj/dyn_exe/q2.res" -dNDEBUG win32/q2.rc

OPTIONS = -W3 -O1 -D DEDICATED_ONLY -I Core/Inc

Release/obj/dedicated/md4.obj : qcommon/md4.cpp
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/md4.obj" qcommon/md4.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/md4.obj : qcommon/md4.cpp
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/md4.obj" qcommon/md4.cpp

Release/obj/q2.res : win32/q2.rc
	rc.exe -l 0x409 -i win32/ -fo"Release/obj/q2.res" -dNDEBUG win32/q2.rc

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h

Release/obj/dyn_exe/DbgSymbols.obj : Core/Src/DbgSymbols.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/DbgSymbols.obj" Core/Src/DbgSymbols.cpp

Release/obj/dyn_exe/DbgSymbolsWin32.obj : Core/Src/DbgSymbolsWin32.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/DbgSymbolsWin32.obj" Core/Src/DbgSymbolsWin32.cpp

Release/obj/dyn_exe/ErrorMgr.obj : Core/Src/ErrorMgr.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/ErrorMgr.obj" Core/Src/ErrorMgr.cpp

Release/obj/dyn_exe/ExceptFilterWin32.obj : Core/Src/ExceptFilterWin32.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/ExceptFilterWin32.obj" Core/Src/ExceptFilterWin32.cpp

Release/obj/dyn_exe/Memory.obj : Core/Src/Memory.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/Memory.obj" Core/Src/Memory.cpp

Release/obj/dyn_exe/OutputDevice.obj : Core/Src/OutputDevice.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/OutputDevice.obj" Core/Src/OutputDevice.cpp

Release/obj/dyn_exe/Strings.obj : Core/Src/Strings.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/Strings.obj" Core/Src/Strings.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/DbgSymbols.obj : Core/Src/DbgSymbols.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/DbgSymbols.obj" Core/Src/DbgSymbols.cpp

Release/obj/DbgSymbolsWin32.obj : Core/Src/DbgSymbolsWin32.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/DbgSymbolsWin32.obj" Core/Src/DbgSymbolsWin32.cpp

Release/obj/ErrorMgr.obj : Core/Src/ErrorMgr.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/ErrorMgr.obj" Core/Src/ErrorMgr.cpp

Release/obj/ExceptFilterWin32.obj : Core/Src/ExceptFilterWin32.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/ExceptFilterWin32.obj" Core/Src/ExceptFilterWin32.cpp

Release/obj/Memory.obj : Core/Src/Memory.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/Memory.obj" Core/Src/Memory.cpp

Release/obj/OutputDevice.obj : Core/Src/OutputDevice.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/OutputDevice.obj" Core/Src/OutputDevice.cpp

Release/obj/Strings.obj : Core/Src/Strings.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/Strings.obj" Core/Src/Strings.cpp

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	Core/Src/CoreLocal.h

Release/obj/dyn_exe/CoreMain.obj : Core/Src/CoreMain.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/CoreMain.obj" Core/Src/CoreMain.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/CoreMain.obj : Core/Src/CoreMain.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/CoreMain.obj" Core/Src/CoreMain.cpp

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/cdaudio.h \
	client/client.h \
	client/console.h \
	client/input.h \
	client/keys.h \
	client/m_flash.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	client/screen.h \
	client/sound.h \
	client/vid.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h

Release/obj/dyn_exe/cl_fx.obj : client/cl_fx.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/cl_fx.obj" client/cl_fx.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/cl_fx.obj : client/cl_fx.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/cl_fx.obj" client/cl_fx.cpp

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/cdaudio.h \
	client/client.h \
	client/console.h \
	client/input.h \
	client/keys.h \
	client/qmenu.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	client/screen.h \
	client/sound.h \
	client/vid.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h

Release/obj/dyn_exe/keys.obj : client/keys.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/keys.obj" client/keys.cpp

Release/obj/dyn_exe/menu.obj : client/menu.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/menu.obj" client/menu.cpp

Release/obj/dyn_exe/qmenu.obj : client/qmenu.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/qmenu.obj" client/qmenu.cpp

Release/obj/dyn_exe/vid_menu.obj : win32/vid_menu.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/vid_menu.obj" win32/vid_menu.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/keys.obj : client/keys.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/keys.obj" client/keys.cpp

Release/obj/menu.obj : client/menu.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/menu.obj" client/menu.cpp

Release/obj/qmenu.obj : client/qmenu.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/qmenu.obj" client/qmenu.cpp

Release/obj/vid_menu.obj : win32/vid_menu.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/vid_menu.obj" win32/vid_menu.cpp

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/cdaudio.h \
	client/client.h \
	client/console.h \
	client/input.h \
	client/keys.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	client/ref_impl.h \
	client/screen.h \
	client/sound.h \
	client/vid.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	win32/resource.h \
	win32/winquake.h

Release/obj/dyn_exe/vid_dll.obj : win32/vid_dll.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/vid_dll.obj" win32/vid_dll.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/vid_dll.obj : win32/vid_dll.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/vid_dll.obj" win32/vid_dll.cpp

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/cdaudio.h \
	client/client.h \
	client/console.h \
	client/input.h \
	client/keys.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	client/screen.h \
	client/snd_loc.h \
	client/sound.h \
	client/vid.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h

Release/obj/dyn_exe/cl_tent.obj : client/cl_tent.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/cl_tent.obj" client/cl_tent.cpp

Release/obj/dyn_exe/snd_dma.obj : client/snd_dma.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/snd_dma.obj" client/snd_dma.cpp

Release/obj/dyn_exe/snd_mem.obj : client/snd_mem.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/snd_mem.obj" client/snd_mem.cpp

Release/obj/dyn_exe/snd_mix.obj : client/snd_mix.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/snd_mix.obj" client/snd_mix.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/cl_tent.obj : client/cl_tent.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/cl_tent.obj" client/cl_tent.cpp

Release/obj/snd_dma.obj : client/snd_dma.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/snd_dma.obj" client/snd_dma.cpp

Release/obj/snd_mem.obj : client/snd_mem.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/snd_mem.obj" client/snd_mem.cpp

Release/obj/snd_mix.obj : client/snd_mix.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/snd_mix.obj" client/snd_mix.cpp

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/cdaudio.h \
	client/client.h \
	client/console.h \
	client/input.h \
	client/keys.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	client/screen.h \
	client/snd_loc.h \
	client/sound.h \
	client/vid.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	win32/winquake.h

Release/obj/dyn_exe/snd_win.obj : win32/snd_win.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/snd_win.obj" win32/snd_win.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/snd_win.obj : win32/snd_win.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/snd_win.obj" win32/snd_win.cpp

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/cdaudio.h \
	client/client.h \
	client/console.h \
	client/input.h \
	client/keys.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	client/screen.h \
	client/sound.h \
	client/vid.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h

Release/obj/dyn_exe/cl_cin.obj : client/cl_cin.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/cl_cin.obj" client/cl_cin.cpp

Release/obj/dyn_exe/cl_download.obj : client/cl_download.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/cl_download.obj" client/cl_download.cpp

Release/obj/dyn_exe/cl_ents.obj : client/cl_ents.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/cl_ents.obj" client/cl_ents.cpp

Release/obj/dyn_exe/cl_input.obj : client/cl_input.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/cl_input.obj" client/cl_input.cpp

Release/obj/dyn_exe/cl_main.obj : client/cl_main.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/cl_main.obj" client/cl_main.cpp

Release/obj/dyn_exe/cl_newfx.obj : client/cl_newfx.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/cl_newfx.obj" client/cl_newfx.cpp

Release/obj/dyn_exe/cl_parse.obj : client/cl_parse.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/cl_parse.obj" client/cl_parse.cpp

Release/obj/dyn_exe/cl_pred.obj : client/cl_pred.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/cl_pred.obj" client/cl_pred.cpp

Release/obj/dyn_exe/cl_scrn.obj : client/cl_scrn.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/cl_scrn.obj" client/cl_scrn.cpp

Release/obj/dyn_exe/cl_view.obj : client/cl_view.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/cl_view.obj" client/cl_view.cpp

Release/obj/dyn_exe/console.obj : client/console.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/console.obj" client/console.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/cl_cin.obj : client/cl_cin.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/cl_cin.obj" client/cl_cin.cpp

Release/obj/cl_download.obj : client/cl_download.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/cl_download.obj" client/cl_download.cpp

Release/obj/cl_ents.obj : client/cl_ents.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/cl_ents.obj" client/cl_ents.cpp

Release/obj/cl_input.obj : client/cl_input.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/cl_input.obj" client/cl_input.cpp

Release/obj/cl_main.obj : client/cl_main.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/cl_main.obj" client/cl_main.cpp

Release/obj/cl_newfx.obj : client/cl_newfx.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/cl_newfx.obj" client/cl_newfx.cpp

Release/obj/cl_parse.obj : client/cl_parse.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/cl_parse.obj" client/cl_parse.cpp

Release/obj/cl_pred.obj : client/cl_pred.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/cl_pred.obj" client/cl_pred.cpp

Release/obj/cl_scrn.obj : client/cl_scrn.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/cl_scrn.obj" client/cl_scrn.cpp

Release/obj/cl_view.obj : client/cl_view.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/cl_view.obj" client/cl_view.cpp

Release/obj/console.obj : client/console.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/console.obj" client/console.cpp

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/cdaudio.h \
	client/client.h \
	client/console.h \
	client/input.h \
	client/keys.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	client/screen.h \
	client/sound.h \
	client/vid.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	win32/winquake.h

Release/obj/dyn_exe/cd_win.obj : win32/cd_win.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/cd_win.obj" win32/cd_win.cpp

Release/obj/dyn_exe/in_win.obj : win32/in_win.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/in_win.obj" win32/in_win.cpp

Release/obj/dyn_exe/sys_win.obj : win32/sys_win.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/sys_win.obj" win32/sys_win.cpp

OPTIONS = -W3 -O1 -D DEDICATED_ONLY -I Core/Inc

Release/obj/dedicated/sys_win.obj : win32/sys_win.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/sys_win.obj" win32/sys_win.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/cd_win.obj : win32/cd_win.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/cd_win.obj" win32/cd_win.cpp

Release/obj/in_win.obj : win32/in_win.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/in_win.obj" win32/in_win.cpp

Release/obj/sys_win.obj : win32/sys_win.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/sys_win.obj" win32/sys_win.cpp

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/anorms.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h

Release/obj/dyn_exe/common.obj : qcommon/common.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/common.obj" qcommon/common.cpp

OPTIONS = -W3 -O1 -D DEDICATED_ONLY -I Core/Inc

Release/obj/dedicated/common.obj : qcommon/common.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/common.obj" qcommon/common.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/common.obj : qcommon/common.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/common.obj" qcommon/common.cpp

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h

Release/obj/dyn_exe/ref_vars.obj : client/ref_vars.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/ref_vars.obj" client/ref_vars.cpp

OPTIONS = -W3 -O1 -D DYNAMIC_REF -I Core/Inc

Release/obj/dyn_ref/ref_vars.obj : client/ref_vars.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/ref_vars.obj" client/ref_vars.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/ref_vars.obj : client/ref_vars.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/ref_vars.obj" client/ref_vars.cpp

OPTIONS = -W3 -O1 -D DYNAMIC_REF -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/gl_backend.h \
	ref_gl/gl_buffers.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_lightmap.h \
	ref_gl/gl_local.h \
	ref_gl/gl_math.h \
	ref_gl/gl_model.h \
	ref_gl/gl_shader.h \
	ref_gl/gl_sky.h \
	ref_gl/glext.h \
	ref_gl/qgl_decl.h

Release/obj/dyn_ref/gl_backend.obj : ref_gl/gl_backend.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/gl_backend.obj" ref_gl/gl_backend.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/gl_backend.obj : ref_gl/gl_backend.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/gl_backend.obj" ref_gl/gl_backend.cpp

OPTIONS = -W3 -O1 -D DYNAMIC_REF -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/gl_backend.h \
	ref_gl/gl_buffers.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_local.h \
	ref_gl/gl_math.h \
	ref_gl/gl_model.h \
	ref_gl/gl_shader.h \
	ref_gl/glext.h \
	ref_gl/qgl_decl.h

Release/obj/dyn_ref/gl_frontend.obj : ref_gl/gl_frontend.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/gl_frontend.obj" ref_gl/gl_frontend.cpp

Release/obj/dyn_ref/gl_main.obj : ref_gl/gl_main.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/gl_main.obj" ref_gl/gl_main.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/gl_frontend.obj : ref_gl/gl_frontend.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/gl_frontend.obj" ref_gl/gl_frontend.cpp

Release/obj/gl_main.obj : ref_gl/gl_main.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/gl_main.obj" ref_gl/gl_main.cpp

OPTIONS = -W3 -O1 -D DYNAMIC_REF -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/gl_backend.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_local.h \
	ref_gl/gl_math.h \
	ref_gl/gl_model.h \
	ref_gl/gl_shader.h \
	ref_gl/gl_sky.h \
	ref_gl/glext.h \
	ref_gl/qgl_decl.h

Release/obj/dyn_ref/gl_sky.obj : ref_gl/gl_sky.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/gl_sky.obj" ref_gl/gl_sky.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/gl_sky.obj : ref_gl/gl_sky.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/gl_sky.obj" ref_gl/gl_sky.cpp

OPTIONS = -W3 -O1 -D DYNAMIC_REF -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/gl_backend.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_local.h \
	ref_gl/gl_math.h \
	ref_gl/gl_model.h \
	ref_gl/gl_shader.h \
	ref_gl/glext.h \
	ref_gl/qgl_decl.h

Release/obj/dyn_ref/gl_light.obj : ref_gl/gl_light.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/gl_light.obj" ref_gl/gl_light.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/gl_light.obj : ref_gl/gl_light.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/gl_light.obj" ref_gl/gl_light.cpp

OPTIONS = -W3 -O1 -D DYNAMIC_REF -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/gl_backend.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_local.h \
	ref_gl/gl_model.h \
	ref_gl/gl_shader.h \
	ref_gl/glext.h \
	ref_gl/qgl_decl.h

Release/obj/dyn_ref/gl_buffers.obj : ref_gl/gl_buffers.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/gl_buffers.obj" ref_gl/gl_buffers.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/gl_buffers.obj : ref_gl/gl_buffers.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/gl_buffers.obj" ref_gl/gl_buffers.cpp

OPTIONS = -W3 -O1 -D DYNAMIC_REF -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/gl_buffers.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_local.h \
	ref_gl/gl_shader.h \
	ref_gl/glext.h \
	ref_gl/qgl_decl.h

Release/obj/dyn_ref/gl_shader.obj : ref_gl/gl_shader.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/gl_shader.obj" ref_gl/gl_shader.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/gl_shader.obj : ref_gl/gl_shader.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/gl_shader.obj" ref_gl/gl_shader.cpp

OPTIONS = -W3 -O1 -D DYNAMIC_REF -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_lightmap.h \
	ref_gl/gl_local.h \
	ref_gl/gl_math.h \
	ref_gl/gl_model.h \
	ref_gl/gl_poly.h \
	ref_gl/gl_shader.h \
	ref_gl/glext.h \
	ref_gl/qgl_decl.h

Release/obj/dyn_ref/gl_model.obj : ref_gl/gl_model.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/gl_model.obj" ref_gl/gl_model.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/gl_model.obj : ref_gl/gl_model.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/gl_model.obj" ref_gl/gl_model.cpp

OPTIONS = -W3 -O1 -D DYNAMIC_REF -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_lightmap.h \
	ref_gl/gl_local.h \
	ref_gl/gl_math.h \
	ref_gl/gl_model.h \
	ref_gl/gl_shader.h \
	ref_gl/glext.h \
	ref_gl/qgl_decl.h

Release/obj/dyn_ref/gl_lightmap.obj : ref_gl/gl_lightmap.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/gl_lightmap.obj" ref_gl/gl_lightmap.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/gl_lightmap.obj : ref_gl/gl_lightmap.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/gl_lightmap.obj" ref_gl/gl_lightmap.cpp

OPTIONS = -W3 -O1 -D DYNAMIC_REF -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_local.h \
	ref_gl/gl_math.h \
	ref_gl/gl_shader.h \
	ref_gl/glext.h \
	ref_gl/qgl_decl.h

Release/obj/dyn_ref/gl_image.obj : ref_gl/gl_image.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/gl_image.obj" ref_gl/gl_image.cpp

Release/obj/dyn_ref/gl_math.obj : ref_gl/gl_math.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/gl_math.obj" ref_gl/gl_math.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/gl_image.obj : ref_gl/gl_image.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/gl_image.obj" ref_gl/gl_image.cpp

Release/obj/gl_math.obj : ref_gl/gl_math.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/gl_math.obj" ref_gl/gl_math.cpp

OPTIONS = -W3 -O1 -D DYNAMIC_REF -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_local.h \
	ref_gl/gl_shader.h \
	ref_gl/glext.h \
	ref_gl/qgl_decl.h

Release/obj/dyn_ref/gl_interface.obj : ref_gl/gl_interface.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/gl_interface.obj" ref_gl/gl_interface.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/gl_interface.obj : ref_gl/gl_interface.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/gl_interface.obj" ref_gl/gl_interface.cpp

OPTIONS = -W3 -O1 -D DYNAMIC_REF -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_local.h \
	ref_gl/gl_poly.h \
	ref_gl/glext.h \
	ref_gl/qgl_decl.h

Release/obj/dyn_ref/gl_poly.obj : ref_gl/gl_poly.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/gl_poly.obj" ref_gl/gl_poly.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/gl_poly.obj : ref_gl/gl_poly.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/gl_poly.obj" ref_gl/gl_poly.cpp

OPTIONS = -W3 -O1 -D DYNAMIC_REF -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_local.h \
	ref_gl/glext.h \
	ref_gl/qgl_decl.h \
	ref_gl/qgl_impl.h \
	win32/glw_win.h

Release/obj/dyn_ref/qgl_win.obj : win32/qgl_win.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/qgl_win.obj" win32/qgl_win.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/qgl_win.obj : win32/qgl_win.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/qgl_win.obj" win32/qgl_win.cpp

OPTIONS = -W3 -O1 -D DYNAMIC_REF -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_local.h \
	ref_gl/glext.h \
	ref_gl/qgl_decl.h \
	win32/glw_win.h

Release/obj/dyn_ref/glw_imp.obj : win32/glw_imp.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/glw_imp.obj" win32/glw_imp.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/glw_imp.obj : win32/glw_imp.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/glw_imp.obj" win32/glw_imp.cpp

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	server/game.h \
	server/server.h

Release/obj/dyn_exe/sv_ccmds.obj : server/sv_ccmds.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/sv_ccmds.obj" server/sv_ccmds.cpp

OPTIONS = -W3 -O1 -D DEDICATED_ONLY -I Core/Inc

Release/obj/dedicated/sv_ccmds.obj : server/sv_ccmds.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/sv_ccmds.obj" server/sv_ccmds.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/sv_ccmds.obj : server/sv_ccmds.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/sv_ccmds.obj" server/sv_ccmds.cpp

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref_decl.h \
	client/ref_defs.h \
	lib/jpeglib/jconfig.h \
	lib/jpeglib/jerror.h \
	lib/jpeglib/jmorecfg.h \
	lib/jpeglib/jpegint.h \
	lib/jpeglib/jpeglib.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h

Release/obj/dyn_exe/images.obj : qcommon/images.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/images.obj" qcommon/images.cpp

OPTIONS = -W3 -O1 -D DEDICATED_ONLY -I Core/Inc

Release/obj/dedicated/images.obj : qcommon/images.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/images.obj" qcommon/images.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/images.obj : qcommon/images.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/images.obj" qcommon/images.cpp

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref_decl.h \
	client/ref_defs.h \
	lib/zlib/zconf.h \
	lib/zlib/zlib.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	qcommon/zip.h

Release/obj/dyn_exe/files.obj : qcommon/files.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/files.obj" qcommon/files.cpp

Release/obj/dyn_exe/zip.obj : qcommon/zip.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/zip.obj" qcommon/zip.cpp

OPTIONS = -W3 -O1 -D DEDICATED_ONLY -I Core/Inc

Release/obj/dedicated/files.obj : qcommon/files.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/files.obj" qcommon/files.cpp

Release/obj/dedicated/zip.obj : qcommon/zip.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/zip.obj" qcommon/zip.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/files.obj : qcommon/files.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/files.obj" qcommon/files.cpp

Release/obj/zip.obj : qcommon/zip.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/zip.obj" qcommon/zip.cpp

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h

Release/obj/dyn_exe/cmd.obj : qcommon/cmd.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/cmd.obj" qcommon/cmd.cpp

Release/obj/dyn_exe/cmodel.obj : qcommon/cmodel.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/cmodel.obj" qcommon/cmodel.cpp

Release/obj/dyn_exe/crc.obj : qcommon/crc.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/crc.obj" qcommon/crc.cpp

Release/obj/dyn_exe/cvar.obj : qcommon/cvar.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/cvar.obj" qcommon/cvar.cpp

Release/obj/dyn_exe/model.obj : qcommon/model.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/model.obj" qcommon/model.cpp

Release/obj/dyn_exe/net_chan.obj : qcommon/net_chan.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/net_chan.obj" qcommon/net_chan.cpp

Release/obj/dyn_exe/pmove.obj : qcommon/pmove.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/pmove.obj" qcommon/pmove.cpp

Release/obj/dyn_exe/q_shared2.obj : qcommon/q_shared2.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/q_shared2.obj" qcommon/q_shared2.cpp

Release/obj/dyn_exe/sv_tokenize.obj : server/sv_tokenize.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/sv_tokenize.obj" server/sv_tokenize.cpp

Release/obj/dyn_exe/net_wins.obj : win32/net_wins.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/net_wins.obj" win32/net_wins.cpp

Release/obj/dyn_exe/q_shwin.obj : win32/q_shwin.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/q_shwin.obj" win32/q_shwin.cpp

OPTIONS = -W3 -O1 -D DEDICATED_ONLY -I Core/Inc

Release/obj/dedicated/cmd.obj : qcommon/cmd.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/cmd.obj" qcommon/cmd.cpp

Release/obj/dedicated/cmodel.obj : qcommon/cmodel.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/cmodel.obj" qcommon/cmodel.cpp

Release/obj/dedicated/crc.obj : qcommon/crc.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/crc.obj" qcommon/crc.cpp

Release/obj/dedicated/cvar.obj : qcommon/cvar.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/cvar.obj" qcommon/cvar.cpp

Release/obj/dedicated/model.obj : qcommon/model.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/model.obj" qcommon/model.cpp

Release/obj/dedicated/net_chan.obj : qcommon/net_chan.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/net_chan.obj" qcommon/net_chan.cpp

Release/obj/dedicated/pmove.obj : qcommon/pmove.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/pmove.obj" qcommon/pmove.cpp

Release/obj/dedicated/q_shared2.obj : qcommon/q_shared2.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/q_shared2.obj" qcommon/q_shared2.cpp

Release/obj/dedicated/sv_dedicated.obj : server/sv_dedicated.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/sv_dedicated.obj" server/sv_dedicated.cpp

Release/obj/dedicated/sv_tokenize.obj : server/sv_tokenize.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/sv_tokenize.obj" server/sv_tokenize.cpp

Release/obj/dedicated/net_wins.obj : win32/net_wins.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/net_wins.obj" win32/net_wins.cpp

Release/obj/dedicated/q_shwin.obj : win32/q_shwin.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/q_shwin.obj" win32/q_shwin.cpp

OPTIONS = -W3 -O1 -D DYNAMIC_REF -I Core/Inc

Release/obj/dyn_ref/q_shared2.obj : qcommon/q_shared2.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_ref/q_shared2.obj" qcommon/q_shared2.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/cmd.obj : qcommon/cmd.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/cmd.obj" qcommon/cmd.cpp

Release/obj/cmodel.obj : qcommon/cmodel.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/cmodel.obj" qcommon/cmodel.cpp

Release/obj/crc.obj : qcommon/crc.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/crc.obj" qcommon/crc.cpp

Release/obj/cvar.obj : qcommon/cvar.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/cvar.obj" qcommon/cvar.cpp

Release/obj/model.obj : qcommon/model.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/model.obj" qcommon/model.cpp

Release/obj/net_chan.obj : qcommon/net_chan.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/net_chan.obj" qcommon/net_chan.cpp

Release/obj/pmove.obj : qcommon/pmove.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/pmove.obj" qcommon/pmove.cpp

Release/obj/q_shared2.obj : qcommon/q_shared2.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/q_shared2.obj" qcommon/q_shared2.cpp

Release/obj/sv_tokenize.obj : server/sv_tokenize.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/sv_tokenize.obj" server/sv_tokenize.cpp

Release/obj/net_wins.obj : win32/net_wins.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/net_wins.obj" win32/net_wins.cpp

Release/obj/q_shwin.obj : win32/q_shwin.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/q_shwin.obj" win32/q_shwin.cpp

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/ref_decl.h \
	client/ref_defs.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	server/game.h \
	server/server.h

Release/obj/dyn_exe/sv_ents.obj : server/sv_ents.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/sv_ents.obj" server/sv_ents.cpp

Release/obj/dyn_exe/sv_game.obj : server/sv_game.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/sv_game.obj" server/sv_game.cpp

Release/obj/dyn_exe/sv_init.obj : server/sv_init.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/sv_init.obj" server/sv_init.cpp

Release/obj/dyn_exe/sv_main.obj : server/sv_main.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/sv_main.obj" server/sv_main.cpp

Release/obj/dyn_exe/sv_send.obj : server/sv_send.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/sv_send.obj" server/sv_send.cpp

Release/obj/dyn_exe/sv_user.obj : server/sv_user.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/sv_user.obj" server/sv_user.cpp

Release/obj/dyn_exe/sv_world.obj : server/sv_world.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dyn_exe/sv_world.obj" server/sv_world.cpp

OPTIONS = -W3 -O1 -D DEDICATED_ONLY -I Core/Inc

Release/obj/dedicated/sv_ents.obj : server/sv_ents.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/sv_ents.obj" server/sv_ents.cpp

Release/obj/dedicated/sv_game.obj : server/sv_game.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/sv_game.obj" server/sv_game.cpp

Release/obj/dedicated/sv_init.obj : server/sv_init.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/sv_init.obj" server/sv_init.cpp

Release/obj/dedicated/sv_main.obj : server/sv_main.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/sv_main.obj" server/sv_main.cpp

Release/obj/dedicated/sv_send.obj : server/sv_send.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/sv_send.obj" server/sv_send.cpp

Release/obj/dedicated/sv_user.obj : server/sv_user.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/sv_user.obj" server/sv_user.cpp

Release/obj/dedicated/sv_world.obj : server/sv_world.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/dedicated/sv_world.obj" server/sv_world.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/sv_ents.obj : server/sv_ents.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/sv_ents.obj" server/sv_ents.cpp

Release/obj/sv_game.obj : server/sv_game.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/sv_game.obj" server/sv_game.cpp

Release/obj/sv_init.obj : server/sv_init.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/sv_init.obj" server/sv_init.cpp

Release/obj/sv_main.obj : server/sv_main.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/sv_main.obj" server/sv_main.cpp

Release/obj/sv_send.obj : server/sv_send.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/sv_send.obj" server/sv_send.cpp

Release/obj/sv_user.obj : server/sv_user.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/sv_user.obj" server/sv_user.cpp

Release/obj/sv_world.obj : server/sv_world.cpp $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"Release/obj/sv_world.obj" server/sv_world.cpp

OPTIONS = -W3 -O1 -D CORE_API= -I Core/Inc

DEPENDS = \
	resource/archive.gz

Release/obj/dyn_exe/makeres.obj : resource/makeres.asm $(DEPENDS)
	nasm.exe -f win32 -i resource/ -o "Release/obj/dyn_exe/makeres.obj" resource/makeres.asm

OPTIONS = -W3 -O1 -D DEDICATED_ONLY -I Core/Inc

Release/obj/dedicated/makeres.obj : resource/makeres.asm $(DEPENDS)
	nasm.exe -f win32 -i resource/ -o "Release/obj/dedicated/makeres.obj" resource/makeres.asm

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I Core/Inc

Release/obj/makeres.obj : resource/makeres.asm $(DEPENDS)
	nasm.exe -f win32 -i resource/ -o "Release/obj/makeres.obj" resource/makeres.asm

#------------------------------------------------------------------------------
#	creating output directories
#------------------------------------------------------------------------------

DIRS :
	if not exist "Release/obj" mkdir "Release/obj"
	if not exist "Release/obj/dyn_exe" mkdir "Release/obj/dyn_exe"
	if not exist "Release/obj/dyn_ref" mkdir "Release/obj/dyn_ref"
	if not exist "Release/obj/dedicated" mkdir "Release/obj/dedicated"

