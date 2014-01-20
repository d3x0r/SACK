PROJECT( proxy )

sack_add_executable( ${PROJECT_NAME} ${FIRST_GCC_PROGRAM_SOURCE} proxy.c ${LAST_GCC_PROGRAM_SOURCE} )
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} ${WIN_SYS_LIBS} )

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES  FOLDER "Network examples" )
install_default_dest( ${PROJECT_NAME} )

add_subdirectory( portable )

