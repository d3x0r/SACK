
cmake_minimum_required(VERSION 2.8)

project( launchcmd )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    launchcmd
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME}" 
COMPILE_DEFINITIONS "TARGETNAME=\"${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}\""
                  FOLDER utils/launcher
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

if( WIN32 )
project( launchwin )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE}
    launchcmd
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME} -DWINDOWS_MODE" 
COMPILE_DEFINITIONS "TARGETNAME=\"${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}\""
                  FOLDER utils/launcher
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

endif( WIN32 )

add_subdirectory( portable )
