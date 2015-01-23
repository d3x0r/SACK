
include_directories( ../../zlib-1.2.7 ../../libpng-1.6.6 )


PROJECT( image_colorizer )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    image_colorizer.c
${LAST_GCC_PROGRAM_SOURCE}
)

sack_add_executable( ${PROJECT_NAME} ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

PROJECT( file_image_colorizer )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    file_image_colorizer.c
${LAST_GCC_PROGRAM_SOURCE}
)

sack_add_executable( ${PROJECT_NAME} ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

