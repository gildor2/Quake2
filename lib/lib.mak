# This file was automatically generated from "lib.prj": do not edit

#------------------------------------------------------------------------------
#	symbolic targets
#------------------------------------------------------------------------------

ALL : lib.lib

#------------------------------------------------------------------------------
#	"lib.lib" target
#------------------------------------------------------------------------------

LIB = \
	obj/jcapimin.obj \
	obj/jcapistd.obj \
	obj/jccoefct.obj \
	obj/jccolor.obj \
	obj/jcdctmgr.obj \
	obj/jcdiffct.obj \
	obj/jchuff.obj \
	obj/jcinit.obj \
	obj/jclhuff.obj \
	obj/jclossls.obj \
	obj/jclossy.obj \
	obj/jcmainct.obj \
	obj/jcmarker.obj \
	obj/jcmaster.obj \
	obj/jcodec.obj \
	obj/jcomapi.obj \
	obj/jcparam.obj \
	obj/jcphuff.obj \
	obj/jcpred.obj \
	obj/jcprepct.obj \
	obj/jcsample.obj \
	obj/jcscale.obj \
	obj/jcshuff.obj \
	obj/jdapimin.obj \
	obj/jdapistd.obj \
	obj/jdatadst.obj \
	obj/jdcoefct.obj \
	obj/jdcolor.obj \
	obj/jddctmgr.obj \
	obj/jddiffct.obj \
	obj/jdhuff.obj \
	obj/jdinput.obj \
	obj/jdlhuff.obj \
	obj/jdlossls.obj \
	obj/jdlossy.obj \
	obj/jdmainct.obj \
	obj/jdmarker.obj \
	obj/jdmaster.obj \
	obj/jdmerge.obj \
	obj/jdphuff.obj \
	obj/jdpostct.obj \
	obj/jdpred.obj \
	obj/jdsample.obj \
	obj/jdscale.obj \
	obj/jdshuff.obj \
	obj/jfdctflt.obj \
	obj/jidctflt.obj \
	obj/jidctred.obj \
	obj/jmemmgr.obj \
	obj/jmemnobs.obj \
	obj/jquant1.obj \
	obj/jquant2.obj \
	obj/jutils.obj \
	obj/adler32.obj \
	obj/crc32.obj \
	obj/inffast.obj \
	obj/inflate.obj \
	obj/inftrees.obj \
	obj/zutil.obj

lib.lib : DIRS $(LIB)
	echo Creating static "lib.lib" ...
	link.exe -lib -nologo -out:"lib.lib" $(LIB)

#------------------------------------------------------------------------------
#	compiling source files
#------------------------------------------------------------------------------

OPTIONS = -W2 -O1 -Ob2 -D INLINE=__inline -D JDCT_DEFAULT=JDCT_FLOAT -D JDCT_FASTEST=JDCT_FLOAT -D NO_GETENV

DEPENDS = \
	jpeglib/jchuff.h \
	jpeglib/jconfig.h \
	jpeglib/jerror.h \
	jpeglib/jinclude.h \
	jpeglib/jlossls.h \
	jpeglib/jmorecfg.h \
	jpeglib/jpegint.h \
	jpeglib/jpeglib.h

obj/jclhuff.obj : jpeglib/jclhuff.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jclhuff.obj" jpeglib/jclhuff.c

DEPENDS = \
	jpeglib/jchuff.h \
	jpeglib/jconfig.h \
	jpeglib/jerror.h \
	jpeglib/jinclude.h \
	jpeglib/jlossy.h \
	jpeglib/jmorecfg.h \
	jpeglib/jpegint.h \
	jpeglib/jpeglib.h

obj/jcphuff.obj : jpeglib/jcphuff.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcphuff.obj" jpeglib/jcphuff.c

obj/jcshuff.obj : jpeglib/jcshuff.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcshuff.obj" jpeglib/jcshuff.c

DEPENDS = \
	jpeglib/jchuff.h \
	jpeglib/jconfig.h \
	jpeglib/jerror.h \
	jpeglib/jinclude.h \
	jpeglib/jmorecfg.h \
	jpeglib/jpegint.h \
	jpeglib/jpeglib.h

obj/jchuff.obj : jpeglib/jchuff.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jchuff.obj" jpeglib/jchuff.c

DEPENDS = \
	jpeglib/jconfig.h \
	jpeglib/jdct.h \
	jpeglib/jerror.h \
	jpeglib/jinclude.h \
	jpeglib/jlossy.h \
	jpeglib/jmorecfg.h \
	jpeglib/jpegint.h \
	jpeglib/jpeglib.h

obj/jcdctmgr.obj : jpeglib/jcdctmgr.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcdctmgr.obj" jpeglib/jcdctmgr.c

obj/jddctmgr.obj : jpeglib/jddctmgr.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jddctmgr.obj" jpeglib/jddctmgr.c

DEPENDS = \
	jpeglib/jconfig.h \
	jpeglib/jdct.h \
	jpeglib/jerror.h \
	jpeglib/jinclude.h \
	jpeglib/jmorecfg.h \
	jpeglib/jpegint.h \
	jpeglib/jpeglib.h

obj/jfdctflt.obj : jpeglib/jfdctflt.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jfdctflt.obj" jpeglib/jfdctflt.c

obj/jidctflt.obj : jpeglib/jidctflt.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jidctflt.obj" jpeglib/jidctflt.c

obj/jidctred.obj : jpeglib/jidctred.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jidctred.obj" jpeglib/jidctred.c

DEPENDS = \
	jpeglib/jconfig.h \
	jpeglib/jdhuff.h \
	jpeglib/jerror.h \
	jpeglib/jinclude.h \
	jpeglib/jlossls.h \
	jpeglib/jlossy.h \
	jpeglib/jmorecfg.h \
	jpeglib/jpegint.h \
	jpeglib/jpeglib.h

obj/jdhuff.obj : jpeglib/jdhuff.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdhuff.obj" jpeglib/jdhuff.c

DEPENDS = \
	jpeglib/jconfig.h \
	jpeglib/jdhuff.h \
	jpeglib/jerror.h \
	jpeglib/jinclude.h \
	jpeglib/jlossls.h \
	jpeglib/jmorecfg.h \
	jpeglib/jpegint.h \
	jpeglib/jpeglib.h

obj/jdlhuff.obj : jpeglib/jdlhuff.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdlhuff.obj" jpeglib/jdlhuff.c

DEPENDS = \
	jpeglib/jconfig.h \
	jpeglib/jdhuff.h \
	jpeglib/jerror.h \
	jpeglib/jinclude.h \
	jpeglib/jlossy.h \
	jpeglib/jmorecfg.h \
	jpeglib/jpegint.h \
	jpeglib/jpeglib.h

obj/jdphuff.obj : jpeglib/jdphuff.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdphuff.obj" jpeglib/jdphuff.c

obj/jdshuff.obj : jpeglib/jdshuff.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdshuff.obj" jpeglib/jdshuff.c

DEPENDS = \
	jpeglib/jconfig.h \
	jpeglib/jerror.h \
	jpeglib/jinclude.h \
	jpeglib/jlossls.h \
	jpeglib/jlossy.h \
	jpeglib/jmorecfg.h \
	jpeglib/jpegint.h \
	jpeglib/jpeglib.h

obj/jcodec.obj : jpeglib/jcodec.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcodec.obj" jpeglib/jcodec.c

DEPENDS = \
	jpeglib/jconfig.h \
	jpeglib/jerror.h \
	jpeglib/jinclude.h \
	jpeglib/jlossls.h \
	jpeglib/jmorecfg.h \
	jpeglib/jpegint.h \
	jpeglib/jpeglib.h

obj/jcdiffct.obj : jpeglib/jcdiffct.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcdiffct.obj" jpeglib/jcdiffct.c

obj/jclossls.obj : jpeglib/jclossls.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jclossls.obj" jpeglib/jclossls.c

obj/jcpred.obj : jpeglib/jcpred.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcpred.obj" jpeglib/jcpred.c

obj/jcscale.obj : jpeglib/jcscale.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcscale.obj" jpeglib/jcscale.c

obj/jddiffct.obj : jpeglib/jddiffct.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jddiffct.obj" jpeglib/jddiffct.c

obj/jdlossls.obj : jpeglib/jdlossls.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdlossls.obj" jpeglib/jdlossls.c

obj/jdpred.obj : jpeglib/jdpred.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdpred.obj" jpeglib/jdpred.c

obj/jdscale.obj : jpeglib/jdscale.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdscale.obj" jpeglib/jdscale.c

DEPENDS = \
	jpeglib/jconfig.h \
	jpeglib/jerror.h \
	jpeglib/jinclude.h \
	jpeglib/jlossy.h \
	jpeglib/jmorecfg.h \
	jpeglib/jpegint.h \
	jpeglib/jpeglib.h

obj/jccoefct.obj : jpeglib/jccoefct.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jccoefct.obj" jpeglib/jccoefct.c

obj/jclossy.obj : jpeglib/jclossy.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jclossy.obj" jpeglib/jclossy.c

obj/jcmaster.obj : jpeglib/jcmaster.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcmaster.obj" jpeglib/jcmaster.c

obj/jdcoefct.obj : jpeglib/jdcoefct.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdcoefct.obj" jpeglib/jdcoefct.c

obj/jdlossy.obj : jpeglib/jdlossy.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdlossy.obj" jpeglib/jdlossy.c

DEPENDS = \
	jpeglib/jconfig.h \
	jpeglib/jerror.h \
	jpeglib/jinclude.h \
	jpeglib/jmemsys.h \
	jpeglib/jmorecfg.h \
	jpeglib/jpegint.h \
	jpeglib/jpeglib.h

obj/jmemmgr.obj : jpeglib/jmemmgr.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jmemmgr.obj" jpeglib/jmemmgr.c

obj/jmemnobs.obj : jpeglib/jmemnobs.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jmemnobs.obj" jpeglib/jmemnobs.c

DEPENDS = \
	jpeglib/jconfig.h \
	jpeglib/jerror.h \
	jpeglib/jinclude.h \
	jpeglib/jmorecfg.h \
	jpeglib/jpegint.h \
	jpeglib/jpeglib.h

obj/jcapimin.obj : jpeglib/jcapimin.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcapimin.obj" jpeglib/jcapimin.c

obj/jcapistd.obj : jpeglib/jcapistd.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcapistd.obj" jpeglib/jcapistd.c

obj/jccolor.obj : jpeglib/jccolor.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jccolor.obj" jpeglib/jccolor.c

obj/jcinit.obj : jpeglib/jcinit.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcinit.obj" jpeglib/jcinit.c

obj/jcmainct.obj : jpeglib/jcmainct.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcmainct.obj" jpeglib/jcmainct.c

obj/jcmarker.obj : jpeglib/jcmarker.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcmarker.obj" jpeglib/jcmarker.c

obj/jcomapi.obj : jpeglib/jcomapi.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcomapi.obj" jpeglib/jcomapi.c

obj/jcparam.obj : jpeglib/jcparam.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcparam.obj" jpeglib/jcparam.c

obj/jcprepct.obj : jpeglib/jcprepct.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcprepct.obj" jpeglib/jcprepct.c

obj/jcsample.obj : jpeglib/jcsample.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jcsample.obj" jpeglib/jcsample.c

obj/jdapimin.obj : jpeglib/jdapimin.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdapimin.obj" jpeglib/jdapimin.c

obj/jdapistd.obj : jpeglib/jdapistd.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdapistd.obj" jpeglib/jdapistd.c

obj/jdatadst.obj : jpeglib/jdatadst.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdatadst.obj" jpeglib/jdatadst.c

obj/jdcolor.obj : jpeglib/jdcolor.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdcolor.obj" jpeglib/jdcolor.c

obj/jdinput.obj : jpeglib/jdinput.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdinput.obj" jpeglib/jdinput.c

obj/jdmainct.obj : jpeglib/jdmainct.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdmainct.obj" jpeglib/jdmainct.c

obj/jdmarker.obj : jpeglib/jdmarker.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdmarker.obj" jpeglib/jdmarker.c

obj/jdmaster.obj : jpeglib/jdmaster.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdmaster.obj" jpeglib/jdmaster.c

obj/jdmerge.obj : jpeglib/jdmerge.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdmerge.obj" jpeglib/jdmerge.c

obj/jdpostct.obj : jpeglib/jdpostct.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdpostct.obj" jpeglib/jdpostct.c

obj/jdsample.obj : jpeglib/jdsample.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jdsample.obj" jpeglib/jdsample.c

obj/jquant1.obj : jpeglib/jquant1.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jquant1.obj" jpeglib/jquant1.c

obj/jquant2.obj : jpeglib/jquant2.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jquant2.obj" jpeglib/jquant2.c

obj/jutils.obj : jpeglib/jutils.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/jutils.obj" jpeglib/jutils.c

OPTIONS = -W2 -O1 -Ob2 -D DYNAMIC_CRC_TABLE -D BUILDFIXED

DEPENDS = \
	zlib/crc32.h \
	zlib/zconf.h \
	zlib/zlib.h \
	zlib/zutil.h

obj/crc32.obj : zlib/crc32.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/crc32.obj" zlib/crc32.c

DEPENDS = \
	zlib/inffast.h \
	zlib/inffixed.h \
	zlib/inflate.h \
	zlib/inftrees.h \
	zlib/zconf.h \
	zlib/zlib.h \
	zlib/zutil.h

obj/inflate.obj : zlib/inflate.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/inflate.obj" zlib/inflate.c

DEPENDS = \
	zlib/inffast.h \
	zlib/inflate.h \
	zlib/inftrees.h \
	zlib/zconf.h \
	zlib/zlib.h \
	zlib/zutil.h

obj/inffast.obj : zlib/inffast.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/inffast.obj" zlib/inffast.c

DEPENDS = \
	zlib/inftrees.h \
	zlib/zconf.h \
	zlib/zlib.h \
	zlib/zutil.h

obj/inftrees.obj : zlib/inftrees.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/inftrees.obj" zlib/inftrees.c

DEPENDS = \
	zlib/zconf.h \
	zlib/zlib.h

obj/adler32.obj : zlib/adler32.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/adler32.obj" zlib/adler32.c

DEPENDS = \
	zlib/zconf.h \
	zlib/zlib.h \
	zlib/zutil.h

obj/zutil.obj : zlib/zutil.c $(DEPENDS)
	cl.exe -nologo -MD -c -D WIN32 -D _WINDOWS $(OPTIONS) -Fo"obj/zutil.obj" zlib/zutil.c

#------------------------------------------------------------------------------
#	creating output directories
#------------------------------------------------------------------------------

DIRS :
	if not exist "obj" mkdir "obj"

