

project( example.render.plugin )


set(SOURCES 
${FIRST_GCC_LIBRARY_SOURCE}
   example
${LAST_GCC_LIBRARY_SOURCE}
)


add_library_force_source( ${PROJECT_NAME} SHARED ${SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  FOLDER "Message Service"
)

target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
if( __ANDROID__ )

else( __ANDROID__ )
  if( UNIX )
    target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} GL GLU glew Xxf86vm )
  else( UNIX )
    target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} opengl32 glu32 )
  endif( UNIX )
endif( __ANDROID__ )

install_mode_dest( ${PROJECT_NAME} )

