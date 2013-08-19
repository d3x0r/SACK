
include_directories( ../../zlib-1.2.7 ../../libpng-1.5.12 )


PROJECT( image_colorizer )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    image_colorizer.c
    imagepng.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable( ${PROJECT_NAME} ${BASE_SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	COMPILE_DEFINITIONS  "TARGET_LABEL=${TARGET_LABEL}" 
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

PROJECT( file_image_colorizer )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    file_image_colorizer.c
    imagepng.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable( ${PROJECT_NAME} ${BASE_SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	COMPILE_DEFINITIONS  "TARGET_LABEL=${TARGET_LABEL}" 
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

