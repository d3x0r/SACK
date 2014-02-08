
SET( TEST_IMAGES background.jpg daub.png playagain.jpg playing.jpg
sky.jpg slot_strip.2.jpg slot_strip.jpg )
INSTALL( 
  FILES
    ${TEST_IMAGES}
  DESTINATION
    ${DATA_INSTALL_PREFIX}/images
)

#-------------------------------------------------------------------

SET( PROJECT test_display1 )
SET( PROJECT_SOURCES test )

ADD_EXECUTABLE( ${PROJECT} ${FIRST_GCC_PROGRAM_SOURCE} ${PROJECT_SOURCES} ${LAST_GCC_PROGRAM_SOURCE} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT}" 
                  FOLDER tests
)
TARGET_LINK_LIBRARIES( ${PROJECT} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT} )

#-------------------------------------------------------------------

SET( PROJECT test_display2 )
SET( PROJECT_SOURCES test2 )

ADD_EXECUTABLE( ${PROJECT} ${FIRST_GCC_PROGRAM_SOURCE} ${PROJECT_SOURCES} ${LAST_GCC_PROGRAM_SOURCE} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT}" 
                  FOLDER tests
)
TARGET_LINK_LIBRARIES( ${PROJECT} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT} )


#-------------------------------------------------------------------

SET( PROJECT test_display3 )
SET( PROJECT_SOURCES test3 )

ADD_EXECUTABLE( ${PROJECT} ${FIRST_GCC_PROGRAM_SOURCE} ${PROJECT_SOURCES} ${LAST_GCC_PROGRAM_SOURCE} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT}" 
                  FOLDER tests
)
TARGET_LINK_LIBRARIES( ${PROJECT} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT} )

#-------------------------------------------------------------------

SET( PROJECT test_display4 )
SET( PROJECT_SOURCES test4 )

ADD_EXECUTABLE( ${PROJECT} ${FIRST_GCC_PROGRAM_SOURCE} ${PROJECT_SOURCES} ${LAST_GCC_PROGRAM_SOURCE} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT}" 
                  FOLDER tests
)
TARGET_LINK_LIBRARIES( ${PROJECT} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT} )


#-------------------------------------------------------------------

SET( PROJECT test_slot_game )
SET( PROJECT_SOURCES slotgame.c )

ADD_EXECUTABLE( ${PROJECT} ${FIRST_GCC_PROGRAM_SOURCE} ${PROJECT_SOURCES} ${LAST_GCC_PROGRAM_SOURCE} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT}" 
                  FOLDER tests
)
TARGET_LINK_LIBRARIES( ${PROJECT} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT} )

#-------------------------------------------------------------------

SET( PROJECT test_slot_game2 )
SET( PROJECT_SOURCES slotgame2.c )

ADD_EXECUTABLE( ${PROJECT} ${FIRST_GCC_PROGRAM_SOURCE} ${PROJECT_SOURCES} ${LAST_GCC_PROGRAM_SOURCE} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT}" 
                  FOLDER tests
)
TARGET_LINK_LIBRARIES( ${PROJECT} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT} )

#-------------------------------------------------------------------

SET( PROJECT test_blur )
SET( PROJECT_SOURCES testblur.c )

ADD_EXECUTABLE( ${PROJECT} ${FIRST_GCC_PROGRAM_SOURCE} ${PROJECT_SOURCES} ${LAST_GCC_PROGRAM_SOURCE} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT}" 
                  FOLDER tests
)
TARGET_LINK_LIBRARIES( ${PROJECT} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT} )

