
cmake_minimum_required(VERSION 2.8)

project( systray_shell )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    systray_shell.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} WIN32 ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
COMPILE_DEFINITIONS "DEDICATE_REBOOT"
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

if( NOT WAS_MONOLITHIC )
  add_subdirectory( portable )
endif( NOT WAS_MONOLITHIC )
