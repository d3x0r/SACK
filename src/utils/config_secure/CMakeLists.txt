project( config_encrypt )

set(BASE_SOURCES
    config_encrypt.c
)

sack_add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER utils )
#target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

#add_subdirectory( portable )
