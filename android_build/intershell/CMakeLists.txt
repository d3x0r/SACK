cmake_minimum_required(VERSION 2.8)


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
         	
    message( "EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${file} ${d}${fileout} )" )
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

macro( install_code target )
	ainstall( ${INTERSHELL_SDK_ROOT_PATH}/lib * DESTINATION ${target} )
endmacro( install_code )

#EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${PROJECT_SOURCE_DIR}/${VECTLIB_SOURCES} ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_double.cpp )


install_resources( package/assets )
install_code( package/libs/armeabi )

dinstall(  ${CMAKE_SOURCE_DIR}/res * DESTINATION package/res )
ainstall(  ${CMAKE_SOURCE_DIR} build.xml DESTINATION package )
ainstall(  ${CMAKE_SOURCE_DIR} AndroidManifest.xml DESTINATION package )
ainstall(  ${CMAKE_SOURCE_DIR} strings.xml DESTINATION package/res/values )

EXECUTE_PROCESS( 
       COMMAND android.bat update project --target "android-14" --path .
       WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/package ERROR_VARIABLE error RESULT_VARIABLE res  )
message( "execute was ${error} or ${res}" )
EXECUTE_PROCESS( 
       COMMAND ant.bat debug 
       WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/package ERROR_VARIABLE error RESULT_VARIABLE res  )
message( "execute was ${error} or ${res}" )

