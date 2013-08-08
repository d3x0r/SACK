
cmake_minimum_required(VERSION 2.8)



PROJECT( flatland_editor )

include_directories( "../../include"  "../../../include")

ADD_EXECUTABLE( ${PROJECT_NAME} WIN32 
  ${FIRST_GCC_PROGRAM_SOURCE}
        flatland.cpp 
        displaydlg.cpp 
        displayfile.cpp 
        drawname.cpp 
        drawtexture.cpp 
        sectdlg.cpp
  ${LAST_GCC_PROGRAM_SOURCE}
)

TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${SACK_LIBRARIES} )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} -DUSE_WORLDSCAPE_INTERFACE"
)

if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
	install( TARGETS ${PROJECT_NAME}
	        RUNTIME DESTINATION bin                     
        	LIBRARY DESTINATION bin
	        ARCHIVE DESTINATION lib )
else( WIN32 )
	install( TARGETS ${PROJECT_NAME}
	        RUNTIME DESTINATION bin 
        	LIBRARY DESTINATION lib
	        ARCHIVE DESTINATION lib )
endif( WIN32 )


