
if( BUILD_TESTS )
  set( project test.clock )
  set(BASE_SOURCES
      testclock.c
  )

  add_program(${project} WIN32 ${BASE_SOURCES} )
  SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER tests/PSI
  )
  install_default_dest( ${project} )
else( BUILD_TESTS )

  set( project analog_clock )

endif( BUILD_TESTS )


install( FILES Clock.png clock_face_1.png DESTINATION bin/images )
install( FILES ${project}.interface.conf DESTINATION bin )
