
project( linux_syslog_scanner )

set(BASE_SOURCES
    syslog_scanner.c
)

sack_add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER utils )
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

add_subdirectory( portable )


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


