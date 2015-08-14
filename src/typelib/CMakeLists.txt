

set( project test.space.web )


set(BASE_SOURCES
    spaceweb.c
)

ADD_PROGRAM(${project} WIN32 ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${project} PROPERTIES
           FOLDER tests
)
SET_PROPERTY(TARGET ${project} APPEND PROPERTY
           COMPILE_DEFINITIONS  "MAKE_RCOORD_SINGLE"
)
my_target_link_libraries( ${project} ${SACK_PLATFORM_LIBRARIES} )

install_default_dest( ${project} )





set( project test.space.web.voronoi )


set(BASE_SOURCES
    spaceweb_voronoi.c
)

ADD_PROGRAM(${project} WIN32 ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${project} PROPERTIES
           FOLDER tests
)
SET_PROPERTY(TARGET ${project} APPEND PROPERTY
           COMPILE_DEFINITIONS  "MAKE_RCOORD_SINGLE"
)
my_target_link_libraries( ${project} ${SACK_PLATFORM_LIBRARIES} )

install_default_dest( ${project} )



