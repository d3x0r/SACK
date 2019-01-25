cmake_minimum_required(VERSION 2.8)
if( POLICY CMP0037 )
	cmake_policy(SET CMP0037 OLD)
endif( POLICY CMP0037 )

# Project becomes the package and application name
# activities are named later.

set( Project Dekware )
PROJECT( ${Project} NONE )

if( NOT DEKWARE_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(DEKWARE_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\Dekware;Install_Dir]" ABSOLUTE CACHE)
else( NOT DEKWARE_SDK_ROOT_PATH )
set(DEKWARE_SDK_ROOT_PATH ${DEKWARE_SDK_ROOT_PATH} CACHE STRING "SACK Dekware SDK Root" )
endif( NOT DEKWARE_SDK_ROOT_PATH )

add_custom_target( update
       WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

macro( ainstall a b c d )
	 FILE( GLOB files ${a}/${b} )
         foreach( file ${files} )
  	STRING( REPLACE "${a}" "" fileout ${file} )
         	
     	if( NOT IS_DIRECTORY ${file} )
		add_custom_command( TARGET update PRE_BUILD  COMMAND cmake -E copy_if_different ${file} ${d}${fileout} )
     	endif( NOT IS_DIRECTORY ${file} )
    endforeach( file )
endmacro( ainstall )

macro( dinstall a b c d )
	 FILE( GLOB_RECURSE files  ${a}/${b} )
         
         foreach( file ${files} )
  	STRING( REPLACE "${a}" "" fileout ${file} )
         	
    add_custom_command( TARGET update PRE_BUILD  COMMAND cmake -E copy_if_different ${file} ${d}${fileout} )
    endforeach( file )
endmacro( dinstall )

macro( install_scripts target )
  FILE( GLOB_RECURSE RESOURCES RELATIVE ${DEKWARE_SDK_ROOT_PATH}/bin/scripts ${DEKWARE_SDK_ROOT_PATH}/bin/scripts/* )
  foreach( resource ${RESOURCES} )
  	STRING( REPLACE "/" "." resout ${resource} )
        add_custom_command( TARGET update PRE_BUILD COMMAND cmake -E copy_if_different ${DEKWARE_SDK_ROOT_PATH}/bin/scripts/${resource} package/assets/${resout} )
  endforeach( resource )
endmacro( install_scripts )

#EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${PROJECT_SOURCE_DIR}/${VECTLIB_SOURCES} ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_double.cpp )


install_scripts( package/assets )

ainstall( ${DEKWARE_SDK_ROOT_PATH}/lib lib*.so DESTINATION package/libs/armeabi )
ainstall( ${DEKWARE_SDK_ROOT_PATH}/bin lib*.so DESTINATION package/libs/armeabi )
ainstall( ${DEKWARE_SDK_ROOT_PATH}/bin/plugins lib*.so DESTINATION package/libs/armeabi )

dinstall(  ${CMAKE_SOURCE_DIR}/res * DESTINATION package/res )
ainstall(  ${CMAKE_SOURCE_DIR} sql.config DESTINATION package/assets )
ainstall(  ${CMAKE_SOURCE_DIR} *.conf DESTINATION package/assets )
ainstall(  ${CMAKE_SOURCE_DIR} *.config DESTINATION package/assets )
configure_file( ${CMAKE_SOURCE_DIR}/build.xml package/build.xml @ONLY )
FILE( READ ${CMAKE_SOURCE_DIR}/AndroidManifest.head.xml manifest_head_file )
STRING( CONFIGURE "${manifest_head_file}" manifest_head )
FILE( READ ${CMAKE_SOURCE_DIR}/AndroidManifest.activity.xml manifest_activity )
FILE( READ ${CMAKE_SOURCE_DIR}/AndroidManifest.tail.xml manifest_tail )
ainstall(  ${CMAKE_SOURCE_DIR} strings.xml DESTINATION package/res/values )


EXECUTE_PROCESS(COMMAND cmake -E remove_directory package/src )
message( "EXECUTE_PROCESS(COMMAND cmake -E remove_directory${P_SOURCE_DIR}/package/src )")

EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_SOURCE_DIR}/../NativeStaticLib.java package/src/org/d3x0r/sack/NativeStaticLib.java )

set( application ${Project} )

set( activity EditOptions )
set( library EditOptions )
set( stringid string/app_name2 )
CONFIGURE_FILE( ${CMAKE_SOURCE_DIR}/../MyNativeActivity.cm package/src/org/d3x0r/${application}/${activity}/${activity}.java )
STRING( CONFIGURE "${manifest_activity}" activity_out )
set( manifest_head "${manifest_head}\n${activity_out}" )

set( activity Dekware )
set( library Dekware )
set( stringid string/app_name1 )
STRING( REPLACE "." "/" app_path ${application} )
FILE( WRITE ${CMAKE_BINARY_DIR}/SupidConfigure.cmake "configure_file( ${CMAKE_SOURCE_DIR}/../MyNativeActivity.cm package/src/org/d3x0r/${app_path}/${activity}/${activity}.java )\n" )
FILE( APPEND ${CMAKE_BINARY_DIR}/SupidConfigure.cmake "EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_SOURCE_DIR}/../NativeStaticLib.java package/src/org/d3x0r/sack/NativeStaticLib.java )\n" )
#CONFIGURE_FILE( ${CMAKE_SOURCE_DIR}/../MyNativeActivity.cm package/src/org/d3x0r/${application}/${activity}/${activity}.java )
STRING( CONFIGURE "${manifest_activity}" activity_out )
set( manifest_head "${manifest_head}\n${activity_out}" )

set( manifest_head "${manifest_head}\n${manifest_tail}" )

FILE( WRITE ${CMAKE_BINARY_DIR}/package/AndroidManifest.xml "${manifest_head}" )


add_custom_command( TARGET update PRE_BUILD COMMAND  cmake -Dapplication=${Project} -Dactivity=${activity} -P ${CMAKE_BINARY_DIR}/SupidConfigure.cmake )


if( CMAKE_BUILD_TYPE STREQUAL "release" )
  set( MORE_COMMANDS 
       COMMAND rm -f bin/${Project}-${CMAKE_BUILD_TYPE}.apk 
       COMMAND jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore ${CMAKE_CURRENT_SOURCE_DIR}/../dekware/my-release-key.keystore bin/${Project}-${CMAKE_BUILD_TYPE}-unsigned.apk alias_name
       COMMAND zipalign -v 4 bin/${Project}-${CMAKE_BUILD_TYPE}-unsigned.apk bin/${Project}-${CMAKE_BUILD_TYPE}.apk
     )
endif( CMAKE_BUILD_TYPE STREQUAL "release" )

    add_custom_target( package
       COMMAND android.bat update project --target "android-14" --path .
       COMMAND ant.bat ${CMAKE_BUILD_TYPE}
       ${MORE_COMMANDS}
       WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/package )


    add_custom_target( install 
    			COMMAND echo adb install command...
                     COMMAND adb install package/bin/${Project}-${CMAKE_BUILD_TYPE}.apk
 )
    add_custom_target( uninstall 
    			COMMAND echo adb uninstall command...
                     COMMAND adb uninstall org.d3x0r.sack.${Project}
 )



#C:\general\android\android-sdk\build-tools\27.0.3\ 
#
#. aapt package -f -m -J <input> -M AndroidManifest.xml -S <output> -I 
#android.jar
#2. javac *.java and output to obj
#3. dx --dex --output=classes.dex obj
#4. aapt package -f -m -F app.unaligned.apk -M AndroidManifest.xml -S 
#<output> -I android.jar
#5. apksigner sign --ks debug.keystore --ks-pass "blabla"
#6. zipalign -f 4 app.unaligned.apk app.apk
