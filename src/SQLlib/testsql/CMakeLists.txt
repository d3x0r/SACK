
project( pssqlcmd )

set(BASE_SOURCES
    sqlcmd
)


sack_add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} ${SACK_PSI_LIBRARY} )
install_default_dest( ${PROJECT_NAME} )


add_subdirectory( portable )



