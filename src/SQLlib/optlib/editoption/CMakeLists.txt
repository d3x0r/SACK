
set( PROJECT EditOptions )

set(BASE_SOURCES
    editopt.c
)

add_program(${PROJECT} WIN32  ${BASE_SOURCES}  )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils
)

sack_add_library(${PROJECT}.plugin. SHARED  ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT}.plugin. PROPERTIES
                  FOLDER utils
)
if( NOT MSVC )
SET_TARGET_PROPERTIES(${PROJECT}.plugin. PROPERTIES
                  OUTPUT_NAME EditOptions.plugin
)
endif( NOT MSVC )

if( NOT __ANDROID__ )
   SET_TARGET_PROPERTIES(${PROJECT}.plugin. PROPERTIES
                  SUFFIX ""
                  PREFIX ""
)
endif( NOT __ANDROID__ )
SET_PROPERTY( TARGET ${PROJECT}.plugin. APPEND PROPERTY
                  COMPILE_DEFINITIONS EDITOPTION_PLUGIN
)

target_link_libraries( ${PROJECT}.plugin. ${SACK_LIBRARIES} )

install_literal_product( ${PROJECT}.plugin. plugins )
