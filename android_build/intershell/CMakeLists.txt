cmake_minimum_required(VERSION 2.8)

set( Project SACK.InterShell )

if( NOT INTERSHELL_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(INTERSHELL_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\InterShell;Install_Dir]" ABSOLUTE CACHE)
else( NOT INTERSHELL_SDK_ROOT_PATH )
set(INTERSHELL_SDK_ROOT_PATH ${INTERSHELL_SDK_ROOT_PATH} CACHE STRING "SACK InterShell SDK Root" )
endif( NOT INTERSHELL_SDK_ROOT_PATH )


#PROJECT( InterShell.Build )

macro( ainstall a b c d )
	 FILE( GLOB files ${a}/${b} )
         
         foreach( file ${files} )
  	STRING( REPLACE "${a}" "" fileout ${file} )
         	
    EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${file} ${d}${fileout} )
    endforeach( file )
endmacro( ainstall )

macro( dinstall a b c d )
	 FILE( GLOB_RECURSE files  ${a}/${b} )
         
         foreach( file ${files} )
  	STRING( REPLACE "${a}" "" fileout ${file} )
         	
    EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${file} ${d}${fileout} )
    endforeach( file )
endmacro( dinstall )

macro( install_resources target )
  FILE( GLOB_RECURSE RESOURCES RELATIVE ${INTERSHELL_SDK_ROOT_PATH}/Resources ${INTERSHELL_SDK_ROOT_PATH}/Resources/* )
  foreach( resource ${RESOURCES} )
  	STRING( REPLACE "/" "." resout ${resource} )
        EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${INTERSHELL_SDK_ROOT_PATH}/Resources/${resource} package/assets/${resout} )
  endforeach( resource )
  
endmacro( install_resources )

#EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${PROJECT_SOURCE_DIR}/${VECTLIB_SOURCES} ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_double.cpp )


install_resources( package/assets )

ainstall( ${INTERSHELL_SDK_ROOT_PATH}/lib * DESTINATION package/libs/armeabi )

dinstall(  ${CMAKE_SOURCE_DIR}/res * DESTINATION package/res )
ainstall(  ${CMAKE_SOURCE_DIR} sql.config DESTINATION package/assets )
ainstall(  ${CMAKE_SOURCE_DIR} libEditOptions.interface.conf DESTINATION package/assets )
ainstall(  ${CMAKE_SOURCE_DIR} libInterShell_console.interface.conf DESTINATION package/assets )
ainstall(  ${CMAKE_SOURCE_DIR} libInterShell_console.config DESTINATION package/assets )
ainstall(  ${CMAKE_SOURCE_DIR} build.xml DESTINATION package )
ainstall(  ${CMAKE_SOURCE_DIR} AndroidManifest.xml DESTINATION package )
ainstall(  ${CMAKE_SOURCE_DIR} strings.xml DESTINATION package/res/values )


EXECUTE_PROCESS(COMMAND cmake -E remove_directory package/src )
message( "EXECUTE_PROCESS(COMMAND cmake -E remove_directory${P_SOURCE_DIR}/package/src )")

set( application InterShell )
set( activity EditOptions )
CONFIGURE_FILE( ${CMAKE_SOURCE_DIR}/../MyNativeActivity.cm package/src/org/d3x0r/${application}/${activity}/${activity}.java )

set( activity InterShell_console )
CONFIGURE_FILE( ${CMAKE_SOURCE_DIR}/../MyNativeActivity.cm package/src/org/d3x0r/${application}/${activity}/${activity}.java )




EXECUTE_PROCESS( 
       COMMAND android.bat update project --target "android-14" --path .
       WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/package )
EXECUTE_PROCESS( 
       COMMAND ant.bat debug 
       WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/package )


    add_custom_target( install 
    			COMMAND echo adb install command...
                     COMMAND adb install package/bin/${Project}-debug.apk
 )


