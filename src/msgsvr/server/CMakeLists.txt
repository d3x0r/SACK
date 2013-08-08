

cmake_minimum_required(VERSION 2.8)


project( sack.msgsvr.service.plugin )


set(SOURCES 
${FIRST_GCC_LIBRARY_SOURCE}
server2
${LAST_GCC_LIBRARY_SOURCE}
)


add_library_force_source( ${PROJECT_NAME} SHARED ${SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  PREFIX ""
                  SUFFIX ""
                  FOLDER "Message Service"
)

target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT_NAME} )


project( msgsvr )

set(SERVER_SOURCES 
${FIRST_GCC_PROGRAM_SOURCE}
server3
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable_force_source( ${PROJECT_NAME} WIN32 ${SERVER_SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} -DSERVERMSG_SOURCE -DBAG"
                  FOLDER "Message Service"
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )

install_mode_dest( ${PROJECT_NAME} )

if( NOT __LINUX__ )
project( sack.msgsvr.service )

set(SERVER_SOURCES 
${FIRST_GCC_PROGRAM_SOURCE}
server_service
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable_force_source( ${PROJECT_NAME} WIN32 ${SERVER_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} -DSERVERMSG_SOURCE -DBAG"
                  FOLDER "Message Service"
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )

install_mode_dest( ${PROJECT_NAME} )

endif( NOT __LINUX__ )

