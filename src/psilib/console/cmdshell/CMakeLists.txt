
set( PROJECT psi_cmdshell )

ADD_PROGRAM( ${PROJECT} WIN32 cmdshell.c  )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES 
                  FOLDER tests/PSI
    BUILD_RPATH "\$ORIGIN/lib"
)
install_default_dest( ${PROJECT} )

