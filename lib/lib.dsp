# Microsoft Developer Studio Project File - Name="lib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=lib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "lib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "lib.mak" CFG="lib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "lib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "lib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "lib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /GX /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib.lib"

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "lib - Win32 Release"
# Name "lib - Win32 Debug"
# Begin Group "JPEGlib"

# PROP Default_Filter ""
# Begin Group "JPEGlib-src"

# PROP Default_Filter "*.c,*.cpp"
# Begin Source File

SOURCE=.\jpeglib\jcapimin.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcapistd.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jccoefct.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jccolor.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcdctmgr.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcdiffct.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jchuff.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcinit.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jclhuff.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jclossls.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jclossy.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcmainct.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcmarker.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcmaster.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcodec.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcomapi.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcparam.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcphuff.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcpred.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcprepct.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcsample.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcscale.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcshuff.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdapimin.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdapistd.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdatadst.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdcoefct.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdcolor.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jddctmgr.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jddiffct.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdhuff.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdinput.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdlhuff.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdlossls.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdlossy.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdmainct.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdmarker.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdmaster.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdmerge.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdphuff.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdpostct.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdpred.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdsample.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdscale.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdshuff.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jfdctflt.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jidctflt.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jidctred.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jmemmgr.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jmemnobs.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jquant1.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jquant2.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeglib\jutils.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D INLINE=__inline /D JDCT_DEFAULT=JDCT_FLOAT /D JDCT_FASTEST=JDCT_FLOAT

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\jpeglib\jchuff.h
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jconfig.h
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdct.h
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdhuff.h
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jerror.h
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jinclude.h
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jlossls.h
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jlossy.h
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jmemsys.h
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jmorecfg.h
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jpegint.h
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jpeglib.h
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jversion.h
# End Source File
# End Group
# Begin Group "ZLib"

# PROP Default_Filter ""
# Begin Group "zlib-src"

# PROP Default_Filter "*.c,*.cpp"
# Begin Source File

SOURCE=.\zlib\adler32.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D "DYNAMIC_CRC_TABLE" /D "BUILDFIXED"

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\zlib\crc32.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D "DYNAMIC_CRC_TABLE" /D "BUILDFIXED"

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\zlib\inffast.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D "DYNAMIC_CRC_TABLE" /D "BUILDFIXED"

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\zlib\inflate.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D "DYNAMIC_CRC_TABLE" /D "BUILDFIXED"

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\zlib\inftrees.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D "DYNAMIC_CRC_TABLE" /D "BUILDFIXED"

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\zlib\zutil.c

!IF  "$(CFG)" == "lib - Win32 Release"

# ADD CPP /D "DYNAMIC_CRC_TABLE" /D "BUILDFIXED"

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\zlib\crc32.h
# End Source File
# Begin Source File

SOURCE=.\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=.\zlib\inffixed.h
# End Source File
# Begin Source File

SOURCE=.\zlib\inflate.h
# End Source File
# Begin Source File

SOURCE=.\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=.\zlib\zlib.h
# End Source File
# Begin Source File

SOURCE=.\zlib\zutil.h
# End Source File
# End Group
# End Target
# End Project
