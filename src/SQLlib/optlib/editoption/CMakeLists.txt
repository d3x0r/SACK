
set( PROJECT EditOptions )

set(BASE_SOURCES
    editopt
)

add_program(${PROJECT} WIN32 
${FIRST_GCC_PROGRAM_SOURCE} 
   ${BASE_SOURCES} 
${LAST_GCC_PROGRAM_SOURCE} 
   )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils
)

sack_add_library(${PROJECT}.plugin. SHARED 
${FIRST_GCC_LIBRARY_SOURCE} 
${BASE_SOURCES}
${LAST_GCC_LIBRARY_SOURCE} 
 )
SET_TARGET_PROPERTIES(${PROJECT}.plugin. PROPERTIES
                  FOLDER utils
                  COMPILE_DEFINITIONS EDITOPTION_PLUGIN
                  SUFFIX ""
                  PREFIX ""
)
target_link_libraries( ${PROJECT}.plugin. ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT}.plugin. )
