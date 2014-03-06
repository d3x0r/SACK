cmake_minimum_required(VERSION 2.8)

if( MSVC )
ADD_DEFINITIONS( -DWIN32 )
endif( MSVC )



# glew library
add_library_force_source( glew SHARED src/glew.c  )
SET_TARGET_PROPERTIES(glew PROPERTIES COMPILE_DEFINITIONS "GLEW_BUILD" )
if( UNIX )
target_link_libraries( glew GL )
else( UNIX )
target_link_libraries( glew opengl32 )
endif( UNIX )
install_mode_dest( glew )

# glewinfo
add_executable_force_source( glewinfo src/glewinfo.c ) 

# glew visualinfo
if( UNIX )
   target_link_libraries( glewinfo glew GL GLU )
   add_executable_force_source( glew-visualinfo src/visualinfo.c)
   target_link_libraries( glew-visualinfo glew GL GLU )
else( UNIX )
   target_link_libraries( glewinfo glew opengl32 glu32 )
   add_executable_force_source( glew-visualinfo src/visualinfo.c)
   target_link_libraries( glew-visualinfo glew opengl32 glu32 )
endif( UNIX )

install_mode_dest( glew glewinfo glew-visualinfo )



