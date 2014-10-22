
set_property(GLOBAL PROPERTY USE_FOLDERS On)
set( CMAKE_INSTALL_MESSAGE "LAZY" )

cmake_minimum_required(VERSION 2.8)

PROJECT( blackvoxel )

set( SDL_SDK_ROOT_PATH M:/SDL2-2.0.3/build/output )
set(SDL_SDK_ROOT_PATH ${SDL_SDK_ROOT_PATH} CACHE STRING "SDL SDK Root" )

if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )

include( ${SACK_SDK_ROOT_PATH}/CMakePackage )
INSTALL_SACK( bin )
include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

if( 0 )
if( NOT SACK_BINARY_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_BINARY_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\Sack Binaries;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_BINARY_SDK_ROOT_PATH )
set(SACK_BINARY_SDK_ROOT_PATH ${SACK_BINARY_SDK_ROOT_PATH} CACHE STRING "INTERSHELL SDK Root" )
endif( NOT SACK_BINARY_SDK_ROOT_PATH )

include( ${SACK_BINARY_SDK_ROOT_PATH}/CMakePackage )

if( NOT INTERSHELL_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(INTERSHELL_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\INTERSHELL;Install_Dir]" ABSOLUTE CACHE)
else( NOT INTERSHELL_SDK_ROOT_PATH )
set(INTERSHELL_SDK_ROOT_PATH ${INTERSHELL_SDK_ROOT_PATH} CACHE STRING "INTERSHELL SDK Root" )
endif( NOT INTERSHELL_SDK_ROOT_PATH )

include( ${INTERSHELL_SDK_ROOT_PATH}/CMakePackage )
INSTALL_INTERSHELL( . )
include_directories( ${INTERSHELL_INCLUDE_DIR} )
LINK_DIRECTORIES( ${INTERSHELL_LIBRARY_DIR} )

if( NOT DEKWARE_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(DEKWARE_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\DEKWARE;Install_Dir]" ABSOLUTE CACHE)
else( NOT DEKWARE_SDK_ROOT_PATH )
set(DEKWARE_SDK_ROOT_PATH ${DEKWARE_SDK_ROOT_PATH} CACHE STRING "DEKWARE SDK Root" )
endif( NOT DEKWARE_SDK_ROOT_PATH )

include( ${DEKWARE_SDK_ROOT_PATH}/CMakePackage )
INSTALL_DEKWARE( . . )
include_directories( ${DEKWARE_INCLUDE_DIR} )
LINK_DIRECTORIES( ${DEKWARE_LIBRARY_DIR} )

endif( 0 )


set( SOURCES 
src/bmploader.cpp
src/main.cpp
src/Z3DBlit.cpp
src/ZActorPhysics.cpp
src/ZActor_Player.cpp
src/ZEventManager.cpp
src/ZGame.cpp
src/ZGameStat.cpp
src/ZGameWindow_Advertising.cpp
src/ZGameWindow_DisplayInfos.cpp
src/ZGameWindow_Inventory.cpp
src/ZGameWindow_Programmable.cpp
src/ZGameWindow_ProgressBar.cpp
src/ZGameWindow_Sequencer.cpp
src/ZGameWindow_Storage.cpp
src/ZGameWindow_UserTextureTransformer.cpp
src/ZGameWindow_VoxelTypeBar.cpp
src/ZGame_Events.cpp
src/ZGui.cpp
src/ZGui_CheckBox.cpp
src/ZGui_CyclingChoiceBox.cpp
src/ZGui_FontFrame.cpp
src/ZGui_InventoryBox.cpp
src/ZGui_KeyChooser.cpp
src/ZGui_NumericChoiceBox.cpp
src/ZGui_PanelTextButton.cpp
src/ZGui_ProgressBar.cpp
src/ZGui_TileFrame.cpp
src/ZGui_VoxelViewBox.cpp
src/ZHighPerfTimer.cpp
src/ZInclusionProbabilizer.cpp
src/ZInventory.cpp
src/ZLog.cpp
src/ZOs_Specific_Various.cpp
src/ZRender_Basic.cpp
src/ZScreen_ChooseOption.cpp
src/ZScreen_Loading.cpp
src/ZScreen_Main.cpp
src/ZScreen_Message.cpp
src/ZScreen_Options_Display.cpp
src/ZScreen_Options_Gameplay.cpp
src/ZScreen_Options_Keymap.cpp
src/ZScreen_Options_Sound.cpp
src/ZScreen_Saving.cpp
src/ZScreen_SlotSelection.cpp
src/ZScripting_Squirrel3.cpp
src/ZSectorSphere.cpp
src/ZSectorStreamLoader.cpp
src/ZSettings_Hardware.cpp
src/ZSound.cpp
src/ZTextureManager.cpp
src/ZTileSets.cpp
src/ZTools.cpp
src/ZTool_Constructor.cpp
src/ZVoxelExtension_FabMachine.cpp
src/ZVoxelExtension_MiningRobot_xr1.cpp
src/ZvoxelExtension_Programmable.cpp
src/ZVoxelExtension_Sequencer.cpp
src/ZVoxelExtension_Storage.cpp
src/ZVoxelExtension_UserTextureTransformer.cpp
src/ZVoxelGfx_Tree.cpp
src/ZVoxelGfx_Tree_Giant.cpp
src/ZVoxelGfx_Tree_Straight.cpp
src/ZVoxelProcessor.cpp
src/ZVoxelReactor.cpp
src/ZVoxelSector.cpp
src/ZVoxelType.cpp
src/ZVoxelTypeManager.cpp
src/ZVoxelType_BlastFurnace.cpp
src/ZVoxelType_Concrete.cpp
src/ZVoxelType_Egmy_T1.cpp
src/ZVoxelType_FabMachine.cpp
src/ZVoxelType_FusionElement.cpp
src/ZVoxelType_IndicatorLight.cpp
src/ZVoxelType_LightTransmitter.cpp
src/ZVoxelType_MiningRobot_xr1.cpp
src/ZVoxelType_Mover.cpp
src/ZVoxelType_Picker.cpp
src/ZVoxelType_PlaneZ1.cpp
src/ZVoxelType_Programmable.cpp
src/ZVoxelType_Sequencer.cpp
src/ZVoxelType_SimpleButton.cpp
src/ZVoxelType_Storage.cpp
src/ZVoxelType_UserTextureTransformer.cpp
src/ZVoxelType_YellowGas.cpp
src/ZWorld.cpp
src/ZWorldConvert.cpp
src/ZWorldGenesis.cpp
src/ZWorldInfo.cpp
src/ZWorld_Stat.cpp

#src/sc_Squirrel3/sqstdlib/sqstdaux.cpp
#src/sc_Squirrel3/sqstdlib/sqstdblob.cpp
#src/sc_Squirrel3/sqstdlib/sqstdio.cpp
#src/sc_Squirrel3/sqstdlib/sqstdmath.cpp
#src/sc_Squirrel3/sqstdlib/sqstdrex.cpp
#src/sc_Squirrel3/sqstdlib/sqstdstream.cpp
#src/sc_Squirrel3/sqstdlib/sqstdstring.cpp
#src/sc_Squirrel3/sqstdlib/sqstdsystem.cpp
#src/sc_Squirrel3/squirrel/sqapi.cpp
#src/sc_Squirrel3/squirrel/sqbaselib.cpp
#src/sc_Squirrel3/squirrel/sqclass.cpp
#src/sc_Squirrel3/squirrel/sqcompiler.cpp
#src/sc_Squirrel3/squirrel/sqdebug.cpp
#src/sc_Squirrel3/squirrel/sqfuncstate.cpp
#src/sc_Squirrel3/squirrel/sqlexer.cpp
#src/sc_Squirrel3/squirrel/sqmem.cpp
#src/sc_Squirrel3/squirrel/sqobject.cpp
#src/sc_Squirrel3/squirrel/sqstate.cpp
#src/sc_Squirrel3/squirrel/sqtable.cpp
#src/sc_Squirrel3/squirrel/sqvm.cpp
src/z/ZGlobal_Settings.cpp
src/z/ZType_ZMatrix.cpp
src/z/ZBitmapImage.cpp
src/z/ZFastRandom.cpp
src/z/ZGenericCanva.cpp
src/z/ZGenericCanva_2.cpp
src/z/ZLinearInterpolator.cpp
src/z/ZMemPool.cpp
src/z/ZMemPool_Optimized.cpp
src/z/ZObject_Array.cpp
src/z/ZObject_Lists.cpp
src/z/ZSimpleFile.cpp
src/z/ZStream_File.cpp
src/z/ZStream_SpecialRamStream.cpp
src/z/ZString.cpp
src/z/ZBitmapImage.cpp
src/z/ZFastRandom.cpp
src/z/ZGenericCanva.cpp
src/z/ZGenericCanva_2.cpp
src/z/ZLinearInterpolator.cpp
src/z/ZMemPool.cpp
src/z/ZMemPool_Optimized.cpp
src/z/ZObject_Array.cpp
src/z/ZObject_Lists.cpp
src/z/ZSimpleFile.cpp
src/z/ZStream_File.cpp
src/z/ZStream_SpecialRamStream.cpp
src/z/ZString.cpp
)

set( HEADERS 
src/ACompileSettings.h
src/bmploader.h
src/PolySpace.h
src/Z3DBlit.h
src/ZActorPhysics.h
src/ZActor_Player.h
src/ZCamera.h
src/ZEgmyScatter.h
src/ZEgmyTargetManager.h
src/ZEjectedCode.h
src/ZEventManager.h
src/ZFabMachineInfos.h
src/ZGame.h
src/ZGameEventSequencer.h
src/ZGameStat.h
src/ZGameWindow_Advertising.h
src/ZGameWindow_DisplayInfos.h
src/ZGameWindow_Inventory.h
src/ZGameWindow_Programmable.h
src/ZGameWindow_ProgressBar.h
src/ZGameWindow_Sequencer.h
src/ZGameWindow_Storage.h
src/ZGameWindow_UserTextureTransformer.h
src/ZGameWindow_VoxelTypeBar.h
src/ZGame_Events.h
src/ZGui.h
src/ZGui_CheckBox.h
src/ZGui_CyclingChoiceBox.h
src/ZGui_FontFrame.h
src/ZGui_InventoryBox.h
src/ZGui_KeyChooser.h
src/ZGui_NumericChoiceBox.h
src/ZGui_PanelTextButton.h
src/ZGui_ProgressBar.h
src/ZGui_TileFrame.h
src/ZGui_VoxelViewBox.h
src/ZHighPerfTimer.h
src/ZInclusionProbabilizer.h
src/ZInventory.h
src/ZLog.h
src/ZOs_Specific_Various.h
src/ZPlayer.h
src/ZPointList.h
src/ZRandom_MakeGoodNumbers.h
src/ZRender_Basic.h
src/ZRender_Sorter.h
src/ZScreen_ChooseOption.h
src/ZScreen_Loading.h
src/ZScreen_Main.h
src/ZScreen_Message.h
src/ZScreen_Options_Display.h
src/ZScreen_Options_Gameplay.h
src/ZScreen_Options_Keymap.h
src/ZScreen_Options_Sound.h
src/ZScreen_Saving.h
src/ZScreen_SlotSelection.h
src/ZScripting.h
src/ZScripting_Squirrel3.h
src/ZSectorLoader.h
src/ZSectorRequestRingList.h
src/ZSectorRingList.h
src/ZSectorSphere.h
src/ZSectorStreamLoader.h
src/ZSectorTagHash.h
src/ZSector_ModifTracker.h
src/ZSettings_Hardware.h
src/ZSound.h
src/ZSpecial_RadiusZoning.h
src/ZTextureManager.h
src/ZTileSets.h
src/ZTools.h
src/ZTool_Constructor.h
src/ZVoxelExtension.h
src/ZVoxelExtensionType_VoxelFluid.h
src/ZVoxelExtension_BlastFurnace.h
src/ZVoxelExtension_Egmy_T1.h
src/ZVoxelExtension_FabMachine.h
src/ZVoxelExtension_FusionElement.h
src/ZVoxelExtension_MiningRobot_xr1.h
src/ZVoxelExtension_PlaneZ1.h
src/ZVoxelExtension_Programmable.h
src/ZVoxelExtension_Sequencer.h
src/ZVoxelExtension_Storage.h
src/ZVoxelExtension_UserTextureTransformer.h
src/ZVoxelGfx_Tree.h
src/ZVoxelGfx_Tree_Giant.h
src/ZVoxelGfx_Tree_Straight.h
src/ZVoxelProcessor.h
src/ZVoxelReaction.h
src/ZVoxelReactor.h
src/ZVoxelSector.h
src/ZVoxelType.h
src/ZVoxelTypeManager.h
src/ZVoxelType_BlastFurnace.h
src/ZVoxelType_Concrete.h
src/ZVoxelType_Egmy_T1.h
src/ZVoxelType_FabMachine.h
src/ZVoxelType_FusionElement.h
src/ZVoxelType_IndicatorLight.h
src/ZVoxelType_LightTransmitter.h
src/ZVoxelType_MiningRobot_xr1.h
src/ZVoxelType_Mover.h
src/ZVoxelType_Picker.h
src/ZVoxelType_PlaneZ1.h
src/ZVoxelType_Programmable.h
src/ZVoxelType_RTFM.h
src/ZVoxelType_Sequencer.h
src/ZVoxelType_SimpleButton.h
src/ZVoxelType_Storage.h
src/ZVoxelType_UserTextureTransformer.h
src/ZVoxelType_Void.h
src/ZVoxelType_YellowGas.h
src/ZWorld.h
src/ZWorldConvert.h
src/ZWorldGenesis.h
src/ZWorldInfo.h
src/ZWorld_Stat.h
src/z/ZBitmapImage.h
src/z/ZFastBit_Array.h
src/z/ZFastRandom.h
src/z/ZGeneralObject.h
src/z/ZGenericCanva.h
src/z/ZGenericCanva_2.h
src/z/ZGenericList.h
src/z/ZGenericTable.h
src/z/ZGlobal_Settings.h
src/z/ZLinearInterpolator.h
src/z/ZMemPool.h
src/z/ZMemPool_Optimized.h
src/z/ZObject.h
src/z/ZObject_Array.h
src/z/ZObject_Lists.h
src/z/ZSimpleFile.h
src/z/ZStreams.h
src/z/ZStream_File.h
src/z/ZStream_SpecialRamStream.h
src/z/ZString.h
src/z/ZTypes.h
src/z/ZType_ZMatrix.h
src/z/ZType_ZPolar3d.h
src/z/ZType_ZPolar3d_CrossFunc.h
src/z/ZType_ZQuaternion.h
src/z/ZType_ZVector3d.h
src/z/ZType_ZVector3d_CrossFunc.h
src/z/ZType_ZVector3L.h
src/z/ZType_ZVector3L_CrossFunc.h
src/z/ZVar.h )

SOURCE_GROUP(Headers FILES ${HEADERS})
OPTION( COMPILEOPTION_ALLOWSAVEPLAYERSTATE "Allow Save Player State" ON )
if( COMPILEOPTION_ALLOWSAVEPLAYERSTATE )
add_definitions( -DCOMPILEOPTION_ALLOWSAVEPLAYERSTATE=1 )
endif( COMPILEOPTION_ALLOWSAVEPLAYERSTATE )

include_directories( ${SDL_SDK_ROOT_PATH}/include )
include_directories( src/sc_Squirrel3/include )
LINK_DIRECTORIES( ${SDL_SDK_ROOT_PATH}/lib )
add_executable( ${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} )
target_link_libraries( ${PROJECT_NAME} M:/SDL2-2.0.3/build/output/lib/SDL2.lib squirrel sqstdlib )

install_default_dest( ${PROJECT_NAME} )

target_link_libraries( ${PROJECT_NAME} ${SACK_PLATFORM_LIBRARIES} ${SACK_LIBRARIES} glew imm32 version glu32 )

add_subdirectory( src/sc_Squirrel3 )
