
if( NOT __NO_GUI__ )

if( NEED_JPEG )
SET( JBASEDIR src/jpeg-9 )
SET( SYSDEPMEM jmemnobs.c )

# library object files common to compression and decompression
SET( COMSRCS  jaricom.c jcomapi.c jutils.c jerror.c jmemmgr.c ${SYSDEPMEM} )

# compression library object files
SET( CLIBSRCS jcarith.c jcapimin.c jcapistd.c jctrans.c jcparam.c jdatadst.c 
        jcinit.c jcmaster.c jcmarker.c jcmainct jcprepct.c 
        jccoefct.c jccolor.c jcsample.c jchuff.c 
        jcdctmgr.c jfdctfst.c jfdctflt.c jfdctint.c )

# decompression library object files
SET( DLIBSRCS jdarith.c jdapimin.c jdapistd.c jdtrans.c jdatasrc.c 
        jdmaster.c jdinput.c jdmarker.c jdhuff.c 
        jdmainct.c jdcoefct.c jdpostct.c jddctmgr.c jidctfst.c 
        jidctflt.c jidctint.c jdsample.c jdcolor.c 
        jquant1.c jquant2.c jdmerge.c )
# These objectfiles are included in libjpeg.lib
FOREACH( SRC ${CLIBSRCS} ${COMSRCS} ${DLIBSRCS} )
  LIST( APPEND JPEG_SOURCE ${JBASEDIR}/${SRC} )
ENDFOREACH( SRC )
Set( ExternalExtraDefinitions ${ExternalExtraDefinitions} JPEG_SOURCE;NO_GETENV )
# ya, this is sorta redundant... should fix that someday.
include_directories( ${SACK_BASE}/${JBASEDIR} )

#message( adding ${SACK_BASE}/${JPEG_SOURCE} )
if( FORCE_CXX )
set_source_files_properties(${JPEG_SOURCE} PROPERTIES LANGUAGE CXX )
endif( FORCE_CXX )

source_group("Source Files\\Jpeg-9 Library" FILES ${JPEG_SOURCE})


endif( NEED_JPEG )


if( NEED_PNG )

 if( NEED_ZLIB )
  SET( ZBASEDIR src/zlib-1.2.7 )
  include( ${ZBASEDIR}/CMakeLists.part )
if( FORCE_CXX )
set_source_files_properties(${ZLIB_SOURCE} PROPERTIES LANGUAGE CXX )
endif( FORCE_CXX )
source_group("Source Files\\zlib-1.2.7 Library" FILES ${ZLIB_SOURCE})
 endif( NEED_ZLIB )


 SET( PBASEDIR src/libpng-1.5.12 )
 include( ${PBASEDIR}/CMakeLists.part )

if( FORCE_CXX )
set_source_files_properties(${PNG_SOURCE} PROPERTIES LANGUAGE CXX )
endif( FORCE_CXX )
source_group("Source Files\\libpng-1.5.12 Library" FILES ${PNG_SOURCE})
endif( NEED_PNG )


if( NEED_FREETYPE )
SET( FBASEDIR src/freetype-2.5.0.1/src )

Set( ExternalExtraDefinitions ${ExternalExtraDefinitions} FREETYPE_SOURCE FT2_BUILD_LIBRARY )

SET( FT_SRCS autofit/autofit.c 
     base/ftbase.c 
     bdf/bdf.c 
     cache/ftcache.c 
     cff/cff.c 
     cid/type1cid.c 
     lzw/ftlzw.c 
     gzip/ftgzip.c 
     otvalid/otvalid.c 
     pcf/pcf.c 
     pfr/pfr.c 
     psnames/psmodule.c 
     psaux/psaux.c 
     pshinter/pshinter.c 
     raster/raster.c 
     sfnt/sfnt.c 
     smooth/smooth.c 
     truetype/truetype.c 
     type1/type1.c 
     type42/type42.c 
     winfonts/winfnt.c 
     base/ftbitmap.c 
     base/ftgasp.c 
     base/ftglyph.c 
     base/ftgxval.c 
     base/ftinit.c 
     base/ftmm.c 
     base/ftotval.c 
     base/ftpfr.c 
     base/ftstroke.c 
     base/ftsynth.c 
     base/ftsystem.c 
     base/fttype1.c 
     base/ftwinfnt.c 
     base/ftxf86.c  )

include_directories( ${FBASEDIR}/../include )
FOREACH( SRC ${FT_SRCS} )
  LIST( APPEND FREETYPE_SOURCE ${FBASEDIR}/${SRC} )
ENDFOREACH()
if( FORCE_CXX )
set_source_files_properties(${FREETYPE_SOURCE} PROPERTIES LANGUAGE CXX )
endif( FORCE_CXX )
endif()

source_group("Source Files\\Freetype-2.5.0.1 Library" FILES ${FREETYPE_SOURCE})

endif( NOT __NO_GUI__ )

