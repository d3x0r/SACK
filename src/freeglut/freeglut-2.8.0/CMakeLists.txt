cmake_minimum_required(VERSION 2.8)

PROJECT( FreeGlut )

INCLUDE_DIRECTORIES( include )

if( WIN32 )
	ADD_DEFINITIONS( -D_WIN32 )
	ADD_DEFINITIONS( -DFREEGLUT_EXPORTS )
endif( WIN32 )

SET( GLUT_SOURCES  
	src/freeglut_callbacks 
	src/freeglut_cursor 
	src/freeglut_display 
	src/freeglut_ext 
	src/freeglut_font 
	src/freeglut_font_data 
	src/freeglut_gamemode 
	src/freeglut_geometry 
	src/freeglut_glutfont_definitions 
	src/freeglut_init 
	src/freeglut_input_devices 
	src/freeglut_joystick 
	src/freeglut_main 
	src/freeglut_menu 
	src/freeglut_misc 
	src/freeglut_overlay 
	src/freeglut_spaceball 
	src/freeglut_state 
	src/freeglut_stroke_mono_roman 
	src/freeglut_stroke_roman 
	src/freeglut_structure 
	src/freeglut_teapot 
	src/freeglut_videoresize 
	src/freeglut_window
)

add_library( freeglut SHARED ${GLUT_SOURCES} )
