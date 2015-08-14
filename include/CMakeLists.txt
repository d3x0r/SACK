
# something like, 
# if !(MAKE_DEV)

SET( SACK_SEXPAT_HEADERS
../src/contrib/sexpat/expat.h
../src/contrib/sexpat/expat_external.h
)

SET( SACK_GENX_HEADERS
../src/contrib/genx/genx.h
)

SET( SYS_IPC_HEADERS 
sys.sack/ipc.h sys.sack/msg.h
)

if( USE_HARDWARE_INFO)
	set( MORE_HEADERS hidtables.h listports.h listhids.h )
endif( USE_HARDWARE_INFO)

SET( SACK_HEADERS
salty_generator.h
mt19937ar.h
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
idle.h
image.h image3d.h
interface.h
keybrd.h
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
sha2.h
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
sack_vfs.h
systray.h
timers.h
translation.h
typelib.h
vectlib.h
vectypes.h
vidlib.h
service_hook.h
html5.canvas.h
html5.websocket.h
html5.websocket.client.h
json_emitter.h
printer.h
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
../src/contrib/zlib-1.2.7/zlib.h
../src/contrib/zlib-1.2.7/zconf.h
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

if( NEED_UUID )
INSTALL( 
  FILES
    ../src/contrib/uuid-1.6.2/uuid.h
  DESTINATION
    include/${CMAKE_PROJECT_NAME}/uuid
)
endif( NEED_UUID )

if( NEED_PNG )
INSTALL( 
  FILES
    ../${PBASEDIR}/png.h
    ../${PBASEDIR}/pngconf.h
    ../${PBASEDIR}/pnglibconf.h
  DESTINATION
    include/${CMAKE_PROJECT_NAME}/png
)
endif( NEED_PNG )

if( NEED_JPEG )
INSTALL( 
  FILES
    ../${JBASEDIR}/jpeglib.h
    ../${JBASEDIR}/jconfig.h
    ../${JBASEDIR}/jmorecfg.h
  DESTINATION
    include/${CMAKE_PROJECT_NAME}/jpeg
)
endif( NEED_JPEG )

if( NEED_FREETYPE )
INSTALL(
  DIRECTORY
  	../src/contrib/freetype-2.5.0.1/include/
  DESTINATION
  	include/${CMAKE_PROJECT_NAME}
)
endif( NEED_FREETYPE )

endif()
