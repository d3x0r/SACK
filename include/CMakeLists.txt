
# something like, 
# if !(MAKE_DEV)

SET( SACK_SEXPAT_HEADERS
../src/sexpat/expat.h
../src/sexpat/expat_external.h
)

SET( SACK_GENX_HEADERS
../src/genx/genx.h
)

SET( SYS_IPC_HEADERS 
sys.sack/ipc.h sys.sack/msg.h
)

SET( SACK_HEADERS
generic_cache.h
colordef.h
configscript.h
construct.h
controls.h
deadstart.h
exit_priorities.h
filedotnet.h
filemon.h
filesys.h
fractions.h
hidtables.h
idle.h
image.h image3d.h
interface.h
keybrd.h
listports.h
listhids.h
loadsock.h
logging.h
menuflags.h
msgclient.h
msgprotocol.h
msgserver.h
netservice.h
network.h
procreg.h
psi.h
pssql.h
rcomlib.h
render.h render3d.h
sack_types.h
http.h
sha1.h
md5.h
sharemem.h
skelhdr.h
spacetree.h
sql_support.h
sqlgetoption.h
sqloptint.h
stdhdrs.h
final_types.h
signed_unsigned_comparisons.h
sackcomm.h
strlib.hpp
symbits.h
sack_system.h
systray.h
timers.h
typelib.h
vectlib.h
vectypes.h
vidlib.h
service_hook.h
html5.canvas.h
html5.websocket.h
html5.websocket.client.h
)

# sqlite3.h

SET( VIDLIB_HEADERS 
vidlib/keydef.h
vidlib/vidstruc.h
)

SET( IMGLIB_HEADERS 
imglib/fontstruct.h
imglib/imagestruct.h
)

SET( ZLIB_HEADERS
../src/zlib-1.2.7/zlib.h
../src/zlib-1.2.7/zconf.h
)



#FILE( GLOB PSI_HEADERS psi/* )

if( CMAKE_PROJECT_NAME )
INSTALL( 
  FILES
    ${SACK_HEADERS}
  DESTINATION
    include/${CMAKE_PROJECT_NAME}
)

INSTALL( 
  FILES
    ${VIDLIB_HEADERS}
  DESTINATION
    include/${CMAKE_PROJECT_NAME}/vidlib
)
INSTALL( 
  FILES
    ${IMGLIB_HEADERS}
  DESTINATION
    include/${CMAKE_PROJECT_NAME}/imglib
)
INSTALL( 
  DIRECTORY psi
  DESTINATION
    include/${CMAKE_PROJECT_NAME}
)

INSTALL( 
  FILES
    ${SACK_GENX_HEADERS}
  DESTINATION
    include/${CMAKE_PROJECT_NAME}/genx
)

INSTALL( 
  FILES
    ${SACK_SEXPAT_HEADERS}
  DESTINATION
    include/${CMAKE_PROJECT_NAME}/sexpat
)

INSTALL( 
  FILES
    ${SYS_IPC_HEADERS}
  DESTINATION
    include/${CMAKE_PROJECT_NAME}/sys.sack
)

if( NEED_ZLIB )
INSTALL( 
  FILES
    ${ZLIB_HEADERS}
  DESTINATION
    include/${CMAKE_PROJECT_NAME}/zlib
)
endif( NEED_ZLIB )

if( NEED_PNG )
INSTALL( 
  FILES
    ../src/libpng-1.5.12/png.h
    ../src/libpng-1.5.12/pngconf.h
    ../src/libpng-1.5.12/pnglibconf.h
  DESTINATION
    include/${CMAKE_PROJECT_NAME}/png
)
endif( NEED_PNG )

if( NEED_JPEG )
INSTALL( 
  FILES
    ../src/jpeg-8d/jpeglib.h
    ../src/jpeg-8d/jconfig.h
    ../src/jpeg-8d/jmorecfg.h
  DESTINATION
    include/${CMAKE_PROJECT_NAME}/jpeg
)
endif( NEED_JPEG )

if( NEED_FREETYPE )
INSTALL(
  DIRECTORY
  	../src/freetype-2.5.0.1/include/
  DESTINATION
  	include/${CMAKE_PROJECT_NAME}
)
endif( NEED_FREETYPE )

endif()
