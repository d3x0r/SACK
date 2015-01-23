
cmake_minimum_required(VERSION 2.8)

project( test_deadstart )


include_directories( 
  M:/sack/include
)

OPTION( __WINDOWS__ "use widnows ctrlc signal handling - else use signals" 1 )

if( __WINDOWS__ )
message("Blah") 
add_definitions( -D__WINDOWS__) 
endif()

add_executable(test_deadstart 
../src/deadstart/deadstart_list
dead1 dead2 dead3 deadmain 
#../src/deadstart/deadstop 
#../src/deadstart/deadstart_core 
../src/deadstart/deadstart_end 
)



#../src/deadstart/deadstart_end dead1 dead2 dead3 deadmain ../src/deadstart/deadstop ../src/deadstart/deadstart ../src/deadstart/deadstart_list
