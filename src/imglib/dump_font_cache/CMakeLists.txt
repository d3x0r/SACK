
project( DumpFontCache )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    dump_font_cache.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	COMPILE_DEFINITIONS "TARGET_LABEL=${PROJECT_NAME}" 
                  FOLDER utils
)

target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
if( __ANDROID__ )
#target_link_libraries( ${PROJECT_NAME} EGL GLESv1_CM )
endif( __ANDROID__ )
if( MAKING_SACK_CORE )
install_mode_dest( ${PROJECT_NAME} )
else( MAKING_SACK_CORE )
install_default_dest( ${PROJECT_NAME} )
endif( MAKING_SACK_CORE )
