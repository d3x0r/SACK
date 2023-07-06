
project( linux_syslog_scanner )

set(BASE_SOURCES
    syslog_scanner.c
)

sack_add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER utils )
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )



set( PROJECT httpd_access_scanner )
set(BASE_SOURCES
    httpd_access_scanner.c
)

sack_add_executable(${PROJECT} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES FOLDER utils )
target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT} )



set( PROJECT arp_scanner )
set(BASE_SOURCES
    arp_scanner.c
)

sack_add_executable(${PROJECT} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES FOLDER utils )
target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT} )

#add_subdirectory( portable )
ExternalProject_Add( ${project}_portable 
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/portable
	INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        ${EXTERNAL_BUILD_EXTRA_FLAGS}
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> 
		-DCMAKE_BUILD_TYPE:PATH=${CMAKE_BUILD_TYPE} 
		-DSACK_BASE=${SACK_BASE}
		-DPLATFORM_LIBRARIES=${PLATFORM_LIBRARIES_ARG}
		-DPLATFORM_DEFINES=${PLATFORM_DEFINES_ARG}
		-DPLATFORM_CMAKE_DEFINES=${PLATFORM_CMAKE_DEFINES_ARG}
)
