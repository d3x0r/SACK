cmake_minimum_required(VERSION 3.1)

PROJECT( localStorageServer )

if( NOT Android )
	ENABLE_LANGUAGE(RC)
endif( NOT Android )


add_custom_command( OUTPUT ${PROJECT_BINARY_DIR}/all_resources.res
                     DEPENDS systray/systray.rc
                     COMMAND echo ${CMAKE_RC_COMPILER} -i${WATCOM_CMAKE_CURRENT_SOURCE_DIR} -fo${WATCOM_PROJECT_BINARY_DIR}\\all_resources.res ${WATCOM_CMAKE_CURRENT_SOURCE_DIR}\\systray\\systray.rc
                     COMMAND ${CMAKE_RC_COMPILER} -i${WATCOM_CMAKE_CURRENT_SOURCE_DIR} -fo${WATCOM_PROJECT_BINARY_DIR}\\all_resources.res ${WATCOM_CMAKE_CURRENT_SOURCE_DIR}\\systray\\systray.rc
 )
# ADD_CUSTOM_TARGET( generate_foo DEPENDS ${PROJECT_BINARY_DIR}/all_resources.res)

add_custom_command( OUTPUT ${PROJECT_BINARY_DIR}/object-storage-remote.h
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/object-storage-remote.js
                     COMMAND echo converting script to header...
                     COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin2header ${CMAKE_CURRENT_SOURCE_DIR}/object-storage-remote.js ${PROJECT_BINARY_DIR}/object-storage-remote.h storage_js
 )
# ADD_CUSTOM_TARGET( generate_foo2 DEPENDS ${PROJECT_BINARY_DIR}/all_resources.res)

add_custom_command( OUTPUT ${PROJECT_BINARY_DIR}/jsox.h
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/jsox.mjs
                     COMMAND echo converting JSOX script to header...
                     COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin2header ${CMAKE_CURRENT_SOURCE_DIR}/jsox.mjs ${PROJECT_BINARY_DIR}/jsox.h jsox
 )

if( WIN32 ) 
  ADD_DEFINITIONS( -DTARGETNAME="localStorageServer.exe" )
endif( WIN32 )

INCLUDE_DIRECTORIES( ${PROJECT_BINARY_DIR} )


add_executable( localStorageServer 
	systray/systray.rc
	${PROJECT_BINARY_DIR}/object-storage-remote.h
	${PROJECT_BINARY_DIR}/jsox.h

	sack_ucb_networking.c
	localStorageServer.c
        systray/systray.c
        )

target_link_libraries( localStorageServer ws2_32 iphlpapi )

