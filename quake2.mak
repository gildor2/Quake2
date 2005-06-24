# This file was automatically generated from "quake2.prj": do not edit

#------------------------------------------------------------------------------
#	symbolic targets
#------------------------------------------------------------------------------

ALL : STATIC DEDICATED
STATIC : Release/quake2.exe
DEDICATED : Release/q2ded.exe

#------------------------------------------------------------------------------
#	"Release/quake2.exe" target
#------------------------------------------------------------------------------

STATIC = \
	Release/obj/CoreStatic/CoreMain.obj \
	Release/obj/CoreStatic/DbgSymbols.obj \
	Release/obj/CoreStatic/Memory.obj \
	Release/obj/CoreStatic/OutputDevice.obj \
	Release/obj/CoreStatic/ErrorMgr.obj \
	Release/obj/CoreStatic/ScriptParser.obj \
	Release/obj/CoreStatic/Strings.obj \
	Release/obj/CoreStatic/CoreWin32.obj \
	Release/obj/CoreStatic/DbgSymbolsWin32.obj \
	Release/obj/CoreStatic/ExceptFilterWin32.obj \
	Release/obj/q2stat/cl_cin.obj \
	Release/obj/q2stat/cl_download.obj \
	Release/obj/q2stat/cl_ents.obj \
	Release/obj/q2stat/cl_fx.obj \
	Release/obj/q2stat/cl_input.obj \
	Release/obj/q2stat/cl_main.obj \
	Release/obj/q2stat/cl_newfx.obj \
	Release/obj/q2stat/cl_parse.obj \
	Release/obj/q2stat/cl_playermodel.obj \
	Release/obj/q2stat/cl_pred.obj \
	Release/obj/q2stat/cl_scrn.obj \
	Release/obj/q2stat/cl_tent.obj \
	Release/obj/q2stat/cl_view.obj \
	Release/obj/q2stat/console.obj \
	Release/obj/q2stat/keys.obj \
	Release/obj/q2stat/menu.obj \
	Release/obj/q2stat/qmenu.obj \
	Release/obj/q2stat/ref_vars.obj \
	Release/obj/q2stat/snd_dma.obj \
	Release/obj/q2stat/snd_mem.obj \
	Release/obj/q2stat/snd_mix.obj \
	Release/obj/q2stat/sv_anim.obj \
	Release/obj/q2stat/sv_ccmds.obj \
	Release/obj/q2stat/sv_ents.obj \
	Release/obj/q2stat/sv_game.obj \
	Release/obj/q2stat/sv_init.obj \
	Release/obj/q2stat/sv_main.obj \
	Release/obj/q2stat/sv_send.obj \
	Release/obj/q2stat/sv_text.obj \
	Release/obj/q2stat/sv_tokenize.obj \
	Release/obj/q2stat/sv_user.obj \
	Release/obj/q2stat/sv_world.obj \
	Release/obj/q2stat/anorms.obj \
	Release/obj/q2stat/cmd.obj \
	Release/obj/q2stat/cmodel.obj \
	Release/obj/q2stat/common.obj \
	Release/obj/q2stat/crc.obj \
	Release/obj/q2stat/cvar.obj \
	Release/obj/q2stat/entdelta.obj \
	Release/obj/q2stat/files.obj \
	Release/obj/q2stat/images.obj \
	Release/obj/q2stat/md4.obj \
	Release/obj/q2stat/model.obj \
	Release/obj/q2stat/msg.obj \
	Release/obj/q2stat/net_chan.obj \
	Release/obj/q2stat/pmove.obj \
	Release/obj/q2stat/q_shared2.obj \
	Release/obj/q2stat/zip.obj \
	Release/obj/q2stat/cd_win.obj \
	Release/obj/q2stat/in_win.obj \
	Release/obj/q2stat/net_wins.obj \
	Release/obj/q2stat/fs_win.obj \
	Release/obj/q2stat/snd_win.obj \
	Release/obj/q2stat/sys_win.obj \
	Release/obj/q2stat/vid_dll.obj \
	Release/obj/q2stat/q2.res \
	Release/obj/q2stat/makeres.obj \
	Release/obj/q2stat/gl_backend.obj \
	Release/obj/q2stat/gl_buffers.obj \
	Release/obj/q2stat/gl_frontend.obj \
	Release/obj/q2stat/gl_image.obj \
	Release/obj/q2stat/gl_interface.obj \
	Release/obj/q2stat/gl_light.obj \
	Release/obj/q2stat/gl_lightmap.obj \
	Release/obj/q2stat/gl_main.obj \
	Release/obj/q2stat/gl_math.obj \
	Release/obj/q2stat/gl_poly.obj \
	Release/obj/q2stat/gl_shader.obj \
	Release/obj/q2stat/gl_sky.obj \
	Release/obj/q2stat/gl_text.obj \
	Release/obj/q2stat/gl_model.obj \
	Release/obj/q2stat/gl_bspmodel.obj \
	Release/obj/q2stat/gl_trimodel.obj \
	Release/obj/q2stat/qgl_win.obj \
	Release/obj/q2stat/gl_win.obj

Release/quake2.exe : DIRS $(STATIC)
	echo Creating executable "Release/quake2.exe" ...
	link.exe -nologo -filealign:512 -incremental:no -out:"Release/quake2.exe" -libpath:"SDK/lib" kernel32.lib user32.lib gdi32.lib winmm.lib lib/lib.lib -map:"Release/quake2.map" $(STATIC)

#------------------------------------------------------------------------------
#	"Release/q2ded.exe" target
#------------------------------------------------------------------------------

DEDICATED = \
	Release/obj/CoreStatic/CoreMain.obj \
	Release/obj/CoreStatic/DbgSymbols.obj \
	Release/obj/CoreStatic/Memory.obj \
	Release/obj/CoreStatic/OutputDevice.obj \
	Release/obj/CoreStatic/ErrorMgr.obj \
	Release/obj/CoreStatic/ScriptParser.obj \
	Release/obj/CoreStatic/Strings.obj \
	Release/obj/CoreStatic/CoreWin32.obj \
	Release/obj/CoreStatic/DbgSymbolsWin32.obj \
	Release/obj/CoreStatic/ExceptFilterWin32.obj \
	Release/obj/dedstat/sv_dedicated.obj \
	Release/obj/dedstat/sv_anim.obj \
	Release/obj/dedstat/sv_ccmds.obj \
	Release/obj/dedstat/sv_ents.obj \
	Release/obj/dedstat/sv_game.obj \
	Release/obj/dedstat/sv_init.obj \
	Release/obj/dedstat/sv_main.obj \
	Release/obj/dedstat/sv_send.obj \
	Release/obj/dedstat/sv_text.obj \
	Release/obj/dedstat/sv_tokenize.obj \
	Release/obj/dedstat/sv_user.obj \
	Release/obj/dedstat/sv_world.obj \
	Release/obj/dedstat/anorms.obj \
	Release/obj/dedstat/cmd.obj \
	Release/obj/dedstat/cmodel.obj \
	Release/obj/dedstat/common.obj \
	Release/obj/dedstat/crc.obj \
	Release/obj/dedstat/cvar.obj \
	Release/obj/dedstat/entdelta.obj \
	Release/obj/dedstat/files.obj \
	Release/obj/dedstat/images.obj \
	Release/obj/dedstat/md4.obj \
	Release/obj/dedstat/model.obj \
	Release/obj/dedstat/msg.obj \
	Release/obj/dedstat/net_chan.obj \
	Release/obj/dedstat/pmove.obj \
	Release/obj/dedstat/q_shared2.obj \
	Release/obj/dedstat/zip.obj \
	Release/obj/dedstat/net_wins.obj \
	Release/obj/dedstat/fs_win.obj \
	Release/obj/dedstat/sys_win.obj \
	Release/obj/dedstat/makeres.obj

Release/q2ded.exe : DIRS $(DEDICATED)
	echo Creating executable "Release/q2ded.exe" ...
	link.exe -nologo -filealign:512 -incremental:no -out:"Release/q2ded.exe" -libpath:"SDK/lib" kernel32.lib user32.lib gdi32.lib winmm.lib lib/lib.lib -map:"Release/q2ded.map" $(DEDICATED)

#------------------------------------------------------------------------------
#	compiling source files
#------------------------------------------------------------------------------

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D DEDICATED_ONLY -I SDK/include -I Core/Inc -I qcommon

Release/obj/dedstat/md4.obj : qcommon/md4.cpp
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/md4.obj" qcommon/md4.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D SINGLE_RENDERER -I SDK/include -I Core/Inc -I qcommon

Release/obj/q2stat/md4.obj : qcommon/md4.cpp
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/md4.obj" qcommon/md4.cpp

Release/obj/q2stat/q2.res : win32/q2.rc
	rc.exe -l 0x409 -i win32/ -fo"Release/obj/q2stat/q2.res" -dNDEBUG win32/q2.rc

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -I SDK/include -I Core/Inc -I qcommon

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h

Release/obj/CoreStatic/CoreWin32.obj : Core/Src/CoreWin32.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/CoreStatic/CoreWin32.obj" Core/Src/CoreWin32.cpp

Release/obj/CoreStatic/DbgSymbols.obj : Core/Src/DbgSymbols.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/CoreStatic/DbgSymbols.obj" Core/Src/DbgSymbols.cpp

Release/obj/CoreStatic/DbgSymbolsWin32.obj : Core/Src/DbgSymbolsWin32.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/CoreStatic/DbgSymbolsWin32.obj" Core/Src/DbgSymbolsWin32.cpp

Release/obj/CoreStatic/ErrorMgr.obj : Core/Src/ErrorMgr.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/CoreStatic/ErrorMgr.obj" Core/Src/ErrorMgr.cpp

Release/obj/CoreStatic/ExceptFilterWin32.obj : Core/Src/ExceptFilterWin32.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/CoreStatic/ExceptFilterWin32.obj" Core/Src/ExceptFilterWin32.cpp

Release/obj/CoreStatic/Memory.obj : Core/Src/Memory.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/CoreStatic/Memory.obj" Core/Src/Memory.cpp

Release/obj/CoreStatic/OutputDevice.obj : Core/Src/OutputDevice.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/CoreStatic/OutputDevice.obj" Core/Src/OutputDevice.cpp

Release/obj/CoreStatic/ScriptParser.obj : Core/Src/ScriptParser.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/CoreStatic/ScriptParser.obj" Core/Src/ScriptParser.cpp

Release/obj/CoreStatic/Strings.obj : Core/Src/Strings.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/CoreStatic/Strings.obj" Core/Src/Strings.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	Core/Src/CoreLocal.h

Release/obj/CoreStatic/CoreMain.obj : Core/Src/CoreMain.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/CoreStatic/CoreMain.obj" Core/Src/CoreMain.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D SINGLE_RENDERER -I SDK/include -I Core/Inc -I qcommon

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/dinput.h \
	client/cdaudio.h \
	client/cl_playermodel.h \
	client/client.h \
	client/console.h \
	client/engine.h \
	client/engine_intf.h \
	client/input.h \
	client/keys.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	client/screen.h \
	client/sound.h \
	client/vid.h \
	qcommon/protocol.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	win32/winquake.h

Release/obj/q2stat/in_win.obj : win32/in_win.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/in_win.obj" win32/in_win.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_exp.h \
	client/rexp_intf.h \
	qcommon/cmodel.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_backend.h \
	ref_gl/gl_buffers.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_math.h \
	ref_gl/gl_model.h \
	ref_gl/gl_shader.h \
	ref_gl/qgl_decl.h

Release/obj/q2stat/gl_main.obj : ref_gl/gl_main.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_main.obj" ref_gl/gl_main.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/cmodel.h \
	qcommon/protocol.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_backend.h \
	ref_gl/gl_buffers.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_lightmap.h \
	ref_gl/gl_math.h \
	ref_gl/gl_model.h \
	ref_gl/gl_shader.h \
	ref_gl/gl_sky.h \
	ref_gl/qgl_decl.h

Release/obj/q2stat/gl_backend.obj : ref_gl/gl_backend.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_backend.obj" ref_gl/gl_backend.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/cmodel.h \
	qcommon/protocol.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_backend.h \
	ref_gl/gl_buffers.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_math.h \
	ref_gl/gl_model.h \
	ref_gl/gl_shader.h \
	ref_gl/gl_sky.h \
	ref_gl/qgl_decl.h

Release/obj/q2stat/gl_frontend.obj : ref_gl/gl_frontend.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_frontend.obj" ref_gl/gl_frontend.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/cmodel.h \
	qcommon/protocol.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_backend.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_math.h \
	ref_gl/gl_model.h \
	ref_gl/gl_shader.h \
	ref_gl/qgl_decl.h

Release/obj/q2stat/gl_light.obj : ref_gl/gl_light.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_light.obj" ref_gl/gl_light.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/cmodel.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_backend.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_math.h \
	ref_gl/gl_model.h \
	ref_gl/gl_shader.h \
	ref_gl/gl_sky.h \
	ref_gl/qgl_decl.h

Release/obj/q2stat/gl_sky.obj : ref_gl/gl_sky.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_sky.obj" ref_gl/gl_sky.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/cmodel.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_backend.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_math.h \
	ref_gl/gl_model.h \
	ref_gl/gl_shader.h \
	ref_gl/qgl_decl.h

Release/obj/q2stat/gl_text.obj : ref_gl/gl_text.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_text.obj" ref_gl/gl_text.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/cmodel.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_backend.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_model.h \
	ref_gl/gl_shader.h \
	ref_gl/qgl_decl.h

Release/obj/q2stat/gl_buffers.obj : ref_gl/gl_buffers.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_buffers.obj" ref_gl/gl_buffers.cpp

Release/obj/q2stat/gl_image.obj : ref_gl/gl_image.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_image.obj" ref_gl/gl_image.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/cmodel.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_lightmap.h \
	ref_gl/gl_math.h \
	ref_gl/gl_model.h \
	ref_gl/gl_poly.h \
	ref_gl/gl_shader.h \
	ref_gl/qgl_decl.h

Release/obj/q2stat/gl_bspmodel.obj : ref_gl/gl_bspmodel.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_bspmodel.obj" ref_gl/gl_bspmodel.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/cmodel.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_lightmap.h \
	ref_gl/gl_math.h \
	ref_gl/gl_model.h \
	ref_gl/gl_shader.h \
	ref_gl/qgl_decl.h

Release/obj/q2stat/gl_lightmap.obj : ref_gl/gl_lightmap.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_lightmap.obj" ref_gl/gl_lightmap.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/cmodel.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_math.h \
	ref_gl/gl_model.h \
	ref_gl/gl_shader.h \
	ref_gl/qgl_decl.h

Release/obj/q2stat/gl_trimodel.obj : ref_gl/gl_trimodel.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_trimodel.obj" ref_gl/gl_trimodel.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/cmodel.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_light.h \
	ref_gl/gl_model.h \
	ref_gl/gl_shader.h \
	ref_gl/qgl_decl.h

Release/obj/q2stat/gl_model.obj : ref_gl/gl_model.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_model.obj" ref_gl/gl_model.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_buffers.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_shader.h \
	ref_gl/gl_shadersyntax.h \
	ref_gl/qgl_decl.h

Release/obj/q2stat/gl_shader.obj : ref_gl/gl_shader.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_shader.obj" ref_gl/gl_shader.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_math.h \
	ref_gl/gl_shader.h \
	ref_gl/qgl_decl.h

Release/obj/q2stat/gl_math.obj : ref_gl/gl_math.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_math.obj" ref_gl/gl_math.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_frontend.h \
	ref_gl/gl_image.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_shader.h \
	ref_gl/qgl_decl.h

Release/obj/q2stat/gl_interface.obj : ref_gl/gl_interface.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_interface.obj" ref_gl/gl_interface.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_interface.h \
	ref_gl/gl_poly.h \
	ref_gl/qgl_decl.h

Release/obj/q2stat/gl_poly.obj : ref_gl/gl_poly.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_poly.obj" ref_gl/gl_poly.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_interface.h \
	ref_gl/qgl_decl.h \
	ref_gl/qgl_impl.h \
	win32/gl_win.h

Release/obj/q2stat/qgl_win.obj : win32/qgl_win.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/qgl_win.obj" win32/qgl_win.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	SDK/include/glext.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	ref_gl/OpenGLDrv.h \
	ref_gl/gl_interface.h \
	ref_gl/qgl_decl.h \
	win32/gl_win.h

Release/obj/q2stat/gl_win.obj : win32/gl_win.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/gl_win.obj" win32/gl_win.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/cdaudio.h \
	client/cl_playermodel.h \
	client/client.h \
	client/console.h \
	client/engine.h \
	client/engine_exp.h \
	client/engine_intf.h \
	client/input.h \
	client/keys.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	client/screen.h \
	client/sound.h \
	client/vid.h \
	qcommon/protocol.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	win32/resource.h \
	win32/winquake.h

Release/obj/q2stat/vid_dll.obj : win32/vid_dll.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/vid_dll.obj" win32/vid_dll.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/cdaudio.h \
	client/cl_playermodel.h \
	client/client.h \
	client/console.h \
	client/engine.h \
	client/engine_intf.h \
	client/input.h \
	client/keys.h \
	client/monster_flash.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	client/screen.h \
	client/sound.h \
	client/vid.h \
	qcommon/protocol.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h

Release/obj/q2stat/cl_fx.obj : client/cl_fx.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cl_fx.obj" client/cl_fx.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/cdaudio.h \
	client/cl_playermodel.h \
	client/client.h \
	client/console.h \
	client/engine.h \
	client/engine_intf.h \
	client/input.h \
	client/keys.h \
	client/qmenu.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	client/screen.h \
	client/sound.h \
	client/vid.h \
	qcommon/protocol.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h

Release/obj/q2stat/keys.obj : client/keys.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/keys.obj" client/keys.cpp

Release/obj/q2stat/menu.obj : client/menu.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/menu.obj" client/menu.cpp

Release/obj/q2stat/qmenu.obj : client/qmenu.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/qmenu.obj" client/qmenu.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/cdaudio.h \
	client/cl_playermodel.h \
	client/client.h \
	client/console.h \
	client/engine.h \
	client/engine_intf.h \
	client/input.h \
	client/keys.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	client/screen.h \
	client/snd_loc.h \
	client/sound.h \
	client/vid.h \
	qcommon/protocol.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h

Release/obj/q2stat/cl_tent.obj : client/cl_tent.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cl_tent.obj" client/cl_tent.cpp

Release/obj/q2stat/snd_dma.obj : client/snd_dma.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/snd_dma.obj" client/snd_dma.cpp

Release/obj/q2stat/snd_mem.obj : client/snd_mem.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/snd_mem.obj" client/snd_mem.cpp

Release/obj/q2stat/snd_mix.obj : client/snd_mix.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/snd_mix.obj" client/snd_mix.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/cdaudio.h \
	client/cl_playermodel.h \
	client/client.h \
	client/console.h \
	client/engine.h \
	client/engine_intf.h \
	client/input.h \
	client/keys.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	client/screen.h \
	client/snd_loc.h \
	client/sound.h \
	client/vid.h \
	qcommon/protocol.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	win32/winquake.h

Release/obj/q2stat/snd_win.obj : win32/snd_win.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/snd_win.obj" win32/snd_win.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/cdaudio.h \
	client/cl_playermodel.h \
	client/client.h \
	client/console.h \
	client/engine.h \
	client/engine_intf.h \
	client/input.h \
	client/keys.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	client/screen.h \
	client/sound.h \
	client/vid.h \
	qcommon/cmodel.h \
	qcommon/protocol.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h

Release/obj/q2stat/cl_ents.obj : client/cl_ents.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cl_ents.obj" client/cl_ents.cpp

Release/obj/q2stat/cl_pred.obj : client/cl_pred.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cl_pred.obj" client/cl_pred.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/cdaudio.h \
	client/cl_playermodel.h \
	client/client.h \
	client/console.h \
	client/engine.h \
	client/engine_intf.h \
	client/input.h \
	client/keys.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	client/screen.h \
	client/sound.h \
	client/vid.h \
	qcommon/protocol.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h

Release/obj/q2stat/cl_cin.obj : client/cl_cin.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cl_cin.obj" client/cl_cin.cpp

Release/obj/q2stat/cl_input.obj : client/cl_input.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cl_input.obj" client/cl_input.cpp

Release/obj/q2stat/cl_main.obj : client/cl_main.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cl_main.obj" client/cl_main.cpp

Release/obj/q2stat/cl_newfx.obj : client/cl_newfx.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cl_newfx.obj" client/cl_newfx.cpp

Release/obj/q2stat/cl_parse.obj : client/cl_parse.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cl_parse.obj" client/cl_parse.cpp

Release/obj/q2stat/cl_playermodel.obj : client/cl_playermodel.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cl_playermodel.obj" client/cl_playermodel.cpp

Release/obj/q2stat/cl_scrn.obj : client/cl_scrn.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cl_scrn.obj" client/cl_scrn.cpp

Release/obj/q2stat/cl_view.obj : client/cl_view.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cl_view.obj" client/cl_view.cpp

Release/obj/q2stat/console.obj : client/console.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/console.obj" client/console.cpp

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/cdaudio.h \
	client/cl_playermodel.h \
	client/client.h \
	client/console.h \
	client/engine.h \
	client/engine_intf.h \
	client/input.h \
	client/keys.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	client/screen.h \
	client/sound.h \
	client/vid.h \
	qcommon/protocol.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h

Release/obj/q2stat/cl_download.obj : client/cl_download.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cl_download.obj" client/cl_download.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D DEDICATED_ONLY -I SDK/include -I Core/Inc -I qcommon

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/cdaudio.h \
	client/cl_playermodel.h \
	client/client.h \
	client/console.h \
	client/engine.h \
	client/engine_intf.h \
	client/input.h \
	client/keys.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	client/screen.h \
	client/sound.h \
	client/vid.h \
	qcommon/protocol.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	win32/winquake.h

Release/obj/dedstat/sys_win.obj : win32/sys_win.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/sys_win.obj" win32/sys_win.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D SINGLE_RENDERER -I SDK/include -I Core/Inc -I qcommon

Release/obj/q2stat/cd_win.obj : win32/cd_win.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cd_win.obj" win32/cd_win.cpp

Release/obj/q2stat/sys_win.obj : win32/sys_win.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/sys_win.obj" win32/sys_win.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D DEDICATED_ONLY -I SDK/include -I Core/Inc -I qcommon

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/protocol.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	server/game.h \
	server/server.h

Release/obj/dedstat/sv_ccmds.obj : server/sv_ccmds.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/sv_ccmds.obj" server/sv_ccmds.cpp

Release/obj/dedstat/sv_text.obj : server/sv_text.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/sv_text.obj" server/sv_text.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D SINGLE_RENDERER -I SDK/include -I Core/Inc -I qcommon

Release/obj/q2stat/sv_ccmds.obj : server/sv_ccmds.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/sv_ccmds.obj" server/sv_ccmds.cpp

Release/obj/q2stat/sv_text.obj : server/sv_text.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/sv_text.obj" server/sv_text.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D DEDICATED_ONLY -I SDK/include -I Core/Inc -I qcommon

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/engine.h \
	client/engine_intf.h \
	client/ref.h \
	client/renderer.h \
	client/rexp_intf.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h

Release/obj/dedstat/common.obj : qcommon/common.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/common.obj" qcommon/common.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D SINGLE_RENDERER -I SDK/include -I Core/Inc -I qcommon

Release/obj/q2stat/common.obj : qcommon/common.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/common.obj" qcommon/common.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D DEDICATED_ONLY -I SDK/include -I Core/Inc -I qcommon

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/engine.h \
	client/engine_intf.h \
	lib/jpeglib/jconfig.h \
	lib/jpeglib/jerror.h \
	lib/jpeglib/jmorecfg.h \
	lib/jpeglib/jpegint.h \
	lib/jpeglib/jpeglib.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h

Release/obj/dedstat/images.obj : qcommon/images.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/images.obj" qcommon/images.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D SINGLE_RENDERER -I SDK/include -I Core/Inc -I qcommon

Release/obj/q2stat/images.obj : qcommon/images.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/images.obj" qcommon/images.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D DEDICATED_ONLY -I SDK/include -I Core/Inc -I qcommon

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/engine.h \
	client/engine_intf.h \
	lib/zlib/zconf.h \
	lib/zlib/zlib.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/zip.h

Release/obj/dedstat/files.obj : qcommon/files.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/files.obj" qcommon/files.cpp

Release/obj/dedstat/zip.obj : qcommon/zip.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/zip.obj" qcommon/zip.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D SINGLE_RENDERER -I SDK/include -I Core/Inc -I qcommon

Release/obj/q2stat/files.obj : qcommon/files.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/files.obj" qcommon/files.cpp

Release/obj/q2stat/zip.obj : qcommon/zip.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/zip.obj" qcommon/zip.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D DEDICATED_ONLY -I SDK/include -I Core/Inc -I qcommon

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/engine.h \
	client/engine_intf.h \
	qcommon/cmodel.h \
	qcommon/protocol.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	server/game.h \
	server/server.h

Release/obj/dedstat/sv_game.obj : server/sv_game.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/sv_game.obj" server/sv_game.cpp

Release/obj/dedstat/sv_main.obj : server/sv_main.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/sv_main.obj" server/sv_main.cpp

Release/obj/dedstat/sv_world.obj : server/sv_world.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/sv_world.obj" server/sv_world.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D SINGLE_RENDERER -I SDK/include -I Core/Inc -I qcommon

Release/obj/q2stat/sv_game.obj : server/sv_game.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/sv_game.obj" server/sv_game.cpp

Release/obj/q2stat/sv_main.obj : server/sv_main.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/sv_main.obj" server/sv_main.cpp

Release/obj/q2stat/sv_world.obj : server/sv_world.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/sv_world.obj" server/sv_world.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D DEDICATED_ONLY -I SDK/include -I Core/Inc -I qcommon

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/engine.h \
	client/engine_intf.h \
	qcommon/cmodel.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	qcommon/qfiles.h

Release/obj/dedstat/cmodel.obj : qcommon/cmodel.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/cmodel.obj" qcommon/cmodel.cpp

Release/obj/dedstat/model.obj : qcommon/model.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/model.obj" qcommon/model.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D SINGLE_RENDERER -I SDK/include -I Core/Inc -I qcommon

Release/obj/q2stat/cmodel.obj : qcommon/cmodel.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cmodel.obj" qcommon/cmodel.cpp

Release/obj/q2stat/model.obj : qcommon/model.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/model.obj" qcommon/model.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D DEDICATED_ONLY -I SDK/include -I Core/Inc -I qcommon

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/engine.h \
	client/engine_intf.h \
	qcommon/protocol.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	server/game.h \
	server/server.h

Release/obj/dedstat/sv_anim.obj : server/sv_anim.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/sv_anim.obj" server/sv_anim.cpp

Release/obj/dedstat/sv_ents.obj : server/sv_ents.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/sv_ents.obj" server/sv_ents.cpp

Release/obj/dedstat/sv_init.obj : server/sv_init.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/sv_init.obj" server/sv_init.cpp

Release/obj/dedstat/sv_send.obj : server/sv_send.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/sv_send.obj" server/sv_send.cpp

Release/obj/dedstat/sv_user.obj : server/sv_user.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/sv_user.obj" server/sv_user.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D SINGLE_RENDERER -I SDK/include -I Core/Inc -I qcommon

Release/obj/q2stat/sv_anim.obj : server/sv_anim.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/sv_anim.obj" server/sv_anim.cpp

Release/obj/q2stat/sv_ents.obj : server/sv_ents.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/sv_ents.obj" server/sv_ents.cpp

Release/obj/q2stat/sv_init.obj : server/sv_init.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/sv_init.obj" server/sv_init.cpp

Release/obj/q2stat/sv_send.obj : server/sv_send.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/sv_send.obj" server/sv_send.cpp

Release/obj/q2stat/sv_user.obj : server/sv_user.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/sv_user.obj" server/sv_user.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D DEDICATED_ONLY -I SDK/include -I Core/Inc -I qcommon

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/engine.h \
	client/engine_intf.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h

Release/obj/dedstat/anorms.obj : qcommon/anorms.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/anorms.obj" qcommon/anorms.cpp

Release/obj/dedstat/cmd.obj : qcommon/cmd.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/cmd.obj" qcommon/cmd.cpp

Release/obj/dedstat/crc.obj : qcommon/crc.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/crc.obj" qcommon/crc.cpp

Release/obj/dedstat/cvar.obj : qcommon/cvar.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/cvar.obj" qcommon/cvar.cpp

Release/obj/dedstat/entdelta.obj : qcommon/entdelta.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/entdelta.obj" qcommon/entdelta.cpp

Release/obj/dedstat/msg.obj : qcommon/msg.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/msg.obj" qcommon/msg.cpp

Release/obj/dedstat/net_chan.obj : qcommon/net_chan.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/net_chan.obj" qcommon/net_chan.cpp

Release/obj/dedstat/pmove.obj : qcommon/pmove.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/pmove.obj" qcommon/pmove.cpp

Release/obj/dedstat/q_shared2.obj : qcommon/q_shared2.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/q_shared2.obj" qcommon/q_shared2.cpp

Release/obj/dedstat/sv_dedicated.obj : server/sv_dedicated.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/sv_dedicated.obj" server/sv_dedicated.cpp

Release/obj/dedstat/sv_tokenize.obj : server/sv_tokenize.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/sv_tokenize.obj" server/sv_tokenize.cpp

Release/obj/dedstat/net_wins.obj : win32/net_wins.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/net_wins.obj" win32/net_wins.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D SINGLE_RENDERER -I SDK/include -I Core/Inc -I qcommon

Release/obj/q2stat/ref_vars.obj : client/ref_vars.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/ref_vars.obj" client/ref_vars.cpp

Release/obj/q2stat/anorms.obj : qcommon/anorms.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/anorms.obj" qcommon/anorms.cpp

Release/obj/q2stat/cmd.obj : qcommon/cmd.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cmd.obj" qcommon/cmd.cpp

Release/obj/q2stat/crc.obj : qcommon/crc.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/crc.obj" qcommon/crc.cpp

Release/obj/q2stat/cvar.obj : qcommon/cvar.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/cvar.obj" qcommon/cvar.cpp

Release/obj/q2stat/entdelta.obj : qcommon/entdelta.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/entdelta.obj" qcommon/entdelta.cpp

Release/obj/q2stat/msg.obj : qcommon/msg.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/msg.obj" qcommon/msg.cpp

Release/obj/q2stat/net_chan.obj : qcommon/net_chan.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/net_chan.obj" qcommon/net_chan.cpp

Release/obj/q2stat/pmove.obj : qcommon/pmove.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/pmove.obj" qcommon/pmove.cpp

Release/obj/q2stat/q_shared2.obj : qcommon/q_shared2.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/q_shared2.obj" qcommon/q_shared2.cpp

Release/obj/q2stat/sv_tokenize.obj : server/sv_tokenize.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/sv_tokenize.obj" server/sv_tokenize.cpp

Release/obj/q2stat/net_wins.obj : win32/net_wins.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/net_wins.obj" win32/net_wins.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D DEDICATED_ONLY -I SDK/include -I Core/Inc -I qcommon

DEPENDS = \
	Core/Inc/Build.h \
	Core/Inc/Commands.h \
	Core/Inc/Core.h \
	Core/Inc/DbgSymbols.h \
	Core/Inc/Macro.h \
	Core/Inc/MemoryMgr.h \
	Core/Inc/ScriptParser.h \
	Core/Inc/Strings.h \
	Core/Inc/VcWin32.h \
	client/engine.h \
	client/engine_intf.h \
	qcommon/q_shared2.h \
	qcommon/qcommon.h \
	win32/winquake.h

Release/obj/dedstat/fs_win.obj : win32/fs_win.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/dedstat/fs_win.obj" win32/fs_win.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D SINGLE_RENDERER -I SDK/include -I Core/Inc -I qcommon

Release/obj/q2stat/fs_win.obj : win32/fs_win.cpp $(DEPENDS)
	cl.exe -nologo -c -D WIN32 -D _WINDOWS -MD $(OPTIONS) -Fo"Release/obj/q2stat/fs_win.obj" win32/fs_win.cpp

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D DEDICATED_ONLY -I SDK/include -I Core/Inc -I qcommon

DEPENDS = \
	resource/archive.gz

Release/obj/dedstat/makeres.obj : resource/makeres.asm $(DEPENDS)
	nasm.exe -f win32 -i resource/ -o "Release/obj/dedstat/makeres.obj" resource/makeres.asm

OPTIONS = -W3 -O1 -D STATIC_BUILD -D CORE_API= -D SINGLE_RENDERER -I SDK/include -I Core/Inc -I qcommon

Release/obj/q2stat/makeres.obj : resource/makeres.asm $(DEPENDS)
	nasm.exe -f win32 -i resource/ -o "Release/obj/q2stat/makeres.obj" resource/makeres.asm

#------------------------------------------------------------------------------
#	creating output directories
#------------------------------------------------------------------------------

DIRS :
	if not exist "Release/obj/CoreStatic" mkdir "Release/obj/CoreStatic"
	if not exist "Release/obj/q2stat" mkdir "Release/obj/q2stat"
	if not exist "Release/obj/dedstat" mkdir "Release/obj/dedstat"

