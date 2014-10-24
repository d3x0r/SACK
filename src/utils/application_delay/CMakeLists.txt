set( PROJECT application_delay.module )

set(BASE_SOURCES
    application_delay.c
)

sack_add_library( ${PROJECT} SHARED ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils
)

if( NOT __ANDROID__ )
  SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
  )
endif( NOT __ANDROID__ )

target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT} )


