cmake_minimum_required(VERSION 2.8)



add_library_force_source( glew SHARED src/glew.c  )
SET_TARGET_PROPERTIES(glew PROPERTIES COMPILE_DEFINITIONS GLEW_BUILD )
if( __LINUX__ )
target_link_libraries( glew GL )
else( __LINUX__ )
target_link_libraries( glew opengl32 )
endif( __LINUX__ )
install_mode_dest( glew )
add_executable_force_source( glewinfo src/glewinfo.c ) 

if( __LINUX__ )
target_link_libraries( glewinfo glew GL GLU )
add_executable_force_source( glew-visualinfo src/visualinfo.c)
target_link_libraries( glew-visualinfo glew GL GLU )
else( __LINUX__ )
target_link_libraries( glewinfo glew opengl32 glu32 )
add_executable_force_source( glew-visualinfo src/visualinfo.c)
target_link_libraries( glew-visualinfo glew opengl32 glu32 )
endif( __LINUX__ )

install_mode_dest( glew glewinfo glew-visualinfo )



