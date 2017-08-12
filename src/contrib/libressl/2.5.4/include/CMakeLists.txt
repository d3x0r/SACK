if( NOT DO_NOT_INSTALL )
install(DIRECTORY .
        DESTINATION include
        PATTERN "CMakeLists.txt" EXCLUDE
        PATTERN "compat" EXCLUDE
        PATTERN "Makefile*" EXCLUDE)
endif( NOT DO_NOT_INSTALL )
