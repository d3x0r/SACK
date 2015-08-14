
set( PROJECT EditTranslations )

set(BASE_SOURCES
    edit_translations.c
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
                  OUTPUT_NAME ${PROJECT}.plugin
)
endif( NOT MSVC )

if( NOT __ANDROID__ )
   SET_TARGET_PROPERTIES(${PROJECT}.plugin. PROPERTIES
                  SUFFIX ""
                  PREFIX ""
)
endif( NOT __ANDROID__ )
SET_PROPERTY( TARGET ${PROJECT}.plugin. APPEND PROPERTY
                  COMPILE_DEFINITIONS EDITTRANSLATION_PLUGIN
)

target_link_libraries( ${PROJECT}.plugin. ${SACK_LIBRARIES} )

install_default_dest( ${PROJECT}.plugin. )
