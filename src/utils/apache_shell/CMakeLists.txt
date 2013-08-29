
set( PROJECT apache_loader )
include_directories( c:/wamp/bin/apache/apache2.4.4/include )

add_library( apache_loader SHARED 
                ${FIRST_GCC_LIBRARY_SOURCE}
                apache_shell.c
                ${LAST_GCC_LIBRARY_SOURCE}
           )

