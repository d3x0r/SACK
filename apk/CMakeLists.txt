

set( SOURCE_ROOT .. )

set( TYPE_LIBRARY_SOURCES
  ${SOURCE_ROOT}/src/typelib/binarylist.c
  ${SOURCE_ROOT}/src/typelib/familytree.c
  ${SOURCE_ROOT}/src/typelib/input.c
  ${SOURCE_ROOT}/src/typelib/sets.c
  ${SOURCE_ROOT}/src/typelib/text.c
  ${SOURCE_ROOT}/src/typelib/http.c
  ${SOURCE_ROOT}/src/typelib/typecode.c
  ${SOURCE_ROOT}/src/typelib/spacetree.c
  ${MSG_QUEUE_SOURCES}
)

set( NETWORK_SOURCES
  ${SOURCE_ROOT}/src/netlib/network.c
  ${SOURCE_ROOT}/src/netlib/tcpnetwork.c
  ${SOURCE_ROOT}/src/netlib/udpnetwork.c
  ${MORE_NETWORK_SOURCES}
)

SET( VECTLIB_SOURCES src/vectlib/vectlib.c )

set( FILESYSTEM_SOURCES 
  ${SOURCE_ROOT}/src/filesyslib/pathops.c
  ${SOURCE_ROOT}/src/filesyslib/filescan.c
  ${SOURCE_ROOT}/src/filesyslib/winfiles.c
)

set( ODBC_SOURCES
  ${SOURCE_ROOT}/src/SQLlib/sqlparse3.c
  ${SOURCE_ROOT}/src/SQLlib/sqlstub.c
  ${SOURCE_ROOT}/src/SQLlib/guid.c
  ${SOURCE_ROOT}/src/SQLlib/sqlutil.c
  ${SOURCE_ROOT}/src/SQLlib/sqlwrap.c
)

SET( OPTION_SOURCE
   ${SOURCE_ROOT}/src/SQLlib/optlib/getoption.c
   ${SOURCE_ROOT}/src/SQLlib/optlib/getoption_new.c
   ${SOURCE_ROOT}/src/SQLlib/optlib/getoption_v4.c
   ${SOURCE_ROOT}/src/SQLlib/optlib/optionutil.c
   ${SOURCE_ROOT}/src/SQLlib/optlib/optionutil_new.c
   ${SOURCE_ROOT}/src/SQLlib/optlib/optionutil_v4.c
)


set ( EXPAT_SOURCES 
  ${SOURCE_ROOT}/src/sexpat/xmlparse.c
  ${SOURCE_ROOT}/src/sexpat/xmlrole.c
  ${SOURCE_ROOT}/src/sexpat/xmltok.c
)

set( GENX_SOURCES
  ${SOURCE_ROOT}/src/genx/charProps.c
  ${SOURCE_ROOT}/src/genx/genx.c
)

set( IMAGE_SOURCES
  ${SOURCE_ROOT}/src/imglib/alphastab.c
  ${SOURCE_ROOT}/src/imglib/alphatab.c
  ${SOURCE_ROOT}/src/imglib/blotdirect.c
  ${SOURCE_ROOT}/src/imglib/blotscaled.c
  ${SOURCE_ROOT}/src/imglib/bmpimage.c
  ${SOURCE_ROOT}/src/imglib/fntcache.c
  ${SOURCE_ROOT}/src/imglib/fntrender.c
  ${SOURCE_ROOT}/src/imglib/font.c
  ${SOURCE_ROOT}/src/imglib/gifimage.c
  ${SOURCE_ROOT}/src/imglib/image.c
  ${SOURCE_ROOT}/src/imglib/image_client.c
  ${SOURCE_ROOT}/src/imglib/interface.c
  ${SOURCE_ROOT}/src/imglib/jpgimage.c
  ${SOURCE_ROOT}/src/imglib/line.c
  ${SOURCE_ROOT}/src/imglib/lucidaconsole.c
  ${SOURCE_ROOT}/src/imglib/math.c
  ${SOURCE_ROOT}/src/imglib/pngimage.c
  ${SOURCE_ROOT}/src/imglib/sprite_common.c
  ${SOURCE_ROOT}/src/imglib/sprite.c
)

set( PSI_MORE_CONTROLS
  ${SOURCE_ROOT}/src/psilib/console/history.c
  ${SOURCE_ROOT}/src/psilib/console/console_keydefs.c
  ${SOURCE_ROOT}/src/psilib/console/paste.c
  ${SOURCE_ROOT}/src/psilib/console/psicon.c
  ${SOURCE_ROOT}/src/psilib/console/psicon_interface.c
  ${SOURCE_ROOT}/src/psilib/console/regaccess.c
  ${SOURCE_ROOT}/src/psilib/console/WinLogic.c
  ${SOURCE_ROOT}/src/psilib/console/console_block_writer.c
)

set( PSI_SOURCES
  ${SOURCE_ROOT}/src/psilib/borders.c
  ${SOURCE_ROOT}/src/psilib/calctl/calender.c
  ${SOURCE_ROOT}/src/psilib/calctl/clock.c
  ${SOURCE_ROOT}/src/psilib/calctl/analog.c
  ${SOURCE_ROOT}/src/psilib/controls.c
  ${SOURCE_ROOT}/src/psilib/control_physical.c
  ${SOURCE_ROOT}/src/psilib/ctlbutton.c
  ${SOURCE_ROOT}/src/psilib/ctledit.c
  ${SOURCE_ROOT}/src/psilib/ctllistbox.c
  ${SOURCE_ROOT}/src/psilib/ctlmisc.c
  ${SOURCE_ROOT}/src/psilib/ctlprop.c
  ${SOURCE_ROOT}/src/psilib/ctlscroll.c
  ${SOURCE_ROOT}/src/psilib/ctlsheet.c
  ${SOURCE_ROOT}/src/psilib/ctlslider.c
  ${SOURCE_ROOT}/src/psilib/ctltext.c
  ${SOURCE_ROOT}/src/psilib/fileopen.c
  ${SOURCE_ROOT}/src/psilib/fntdlg.c
#  ${SOURCE_ROOT}/src/psilib/loadsave.c
  ${SOURCE_ROOT}/src/psilib/mouse.c
  ${SOURCE_ROOT}/src/psilib/palette.c
  ${SOURCE_ROOT}/src/psilib/popups.c
  ${SOURCE_ROOT}/src/psilib/xml_load.c
  ${SOURCE_ROOT}/src/psilib/xml_save.c
  ${SOURCE_ROOT}/src/psilib/option_frame.c
  ${SOURCE_ROOT}/src/psilib/scrollknob.c
  ${SOURCE_ROOT}/${PSI_MORE_CONTROLS}
)

SET( PSI_SOURCES ${PSI_SOURCES}  
  ${SOURCE_ROOT}src/systraylib/systray_client.c )

set( PSI_SOURCES ${PSI_SOURCES} 
  ${SOURCE_ROOT}/src/psilib/fntcache.c
)

SET( HTML5_SOURCES 
		src/psilib/html5.canvas/html5.canvas.c
		src/netlib/html5.websocket/html5.websocket.c
	)

set( SACK_MSG_SERVICES 
  ${SOURCE_ROOT}/src/msgsvr/client/client.c
  ${SOURCE_ROOT}/src/msgsvr/summoner/construct.c
  )

set( FILEMONITOR_SOURCES
  ${SOURCE_ROOT}/src/filesyslib/filemon/allfiles.c
  ${SOURCE_ROOT}/src/filesyslib/winfiles.c
)

if( __LINUX__ )
set(FILEMONITOR_SOURCES ${FILEMONITOR_SOURCES}
  ${SOURCE_ROOT}/src/filesyslib/filemon/linuxfiles.c
)
else( __LINUX__ )
set(FILEMONITOR_SOURCES ${FILEMONITOR_SOURCES}
  ${SOURCE_ROOT}/src/filesyslib/filemon/windowsfiles.c
)
endif( __LINUX__ )

set(BASE_SOURCES ${BASE_SOURCES} ${SACK_MSG_SERVICES} ${FILEMONITOR_SOURCES} ${HTML5_SOURCES} )

set(BASE_SOURCES  ${BASE_SOURCES}
  ${SOURCE_ROOT}/src/sysloglib/syslog.c
  ${SOURCE_ROOT}/src/memlib/sharemem.c
  ${SOURCE_ROOT}/src/memlib/memory_operations.c
  ${MEM_ASSEMBLY_SOURCES}
  ${TYPE_LIBRARY_SOURCES}
  ${SOURCE_ROOT}/src/timerlib/timers.c
  ${SOURCE_ROOT}/src/idlelib/idle.c
  ${SOURCE_ROOT}/src/fractionlib/fractions.c
  ${SOURCE_ROOT}/src/configlib/configscript.c
  ${SOURCE_ROOT}/src/procreglib/names.c
  ${SOURCE_ROOT}/src/systemlib/args.c
  ${SOURCE_ROOT}/src/systemlib/system.c
  ${SOURCE_ROOT}/src/systemlib/spawntask.c
  ${NETWORK_SOURCES}
  ${FILESYSTEM_SOURCES}
  ${SOURCE_ROOT}/src/deadstart/deadstart_core.c
  ${SHA1_SOURCES}
  ${MD5_SOURCES}
  ${MORE_SYSTEM_SOURCES}
  ${SERVICE_STUB_SOURCES}
  ${COMM_SOURCES}
  ${OPTION_SOURCE}
)




set( CORE_SOURCES ${SOURCE_ROOT}/
