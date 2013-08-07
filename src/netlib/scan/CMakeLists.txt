
PROJECT( scan_ports )
add_executable( ${PROJECT_NAME} ${FIRST_GCC_PROGRAM_SOURCE} 
scanner 
${LAST_GCC_PROGRAM_SOURCE}
)
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
if( WIN32 )
target_link_libraries( ${PROJECT_NAME} wsock32 )
endif( WIN32 )
install_default_dest( ${PROJECT_NAME} )

if( NOT __ARM__ )
PROJECT( scan_addresses )
add_executable( ${PROJECT_NAME} ${FIRST_GCC_PROGRAM_SOURCE} 
pinger 
${LAST_GCC_PROGRAM_SOURCE}
)
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
if( WIN32 )
target_link_libraries( ${PROJECT_NAME} wsock32 )
endif( WIN32 )
install_default_dest( ${PROJECT_NAME} )
endif( NOT __ARM__ )

PROJECT( nameof )
add_executable( ${PROJECT_NAME} ${FIRST_GCC_PROGRAM_SOURCE} 
nameof 
${LAST_GCC_PROGRAM_SOURCE}
)
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
if( WIN32 )
target_link_libraries( ${PROJECT_NAME} wsock32 )
endif( WIN32 )
install_default_dest( ${PROJECT_NAME} )

PROJECT( ipof )
add_executable( ${PROJECT_NAME} ${FIRST_GCC_PROGRAM_SOURCE} 
ipof 
${LAST_GCC_PROGRAM_SOURCE}
)
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )


add_subdirectory( portable )

