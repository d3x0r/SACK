/*
 * This file is part of Blackvoxel.
 *
 * Copyright 2010-2014 Laurent Thiebaut & Olivia Merle
 *
 * Blackvoxel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Blackvoxel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
 * ZActor_Player.cpp
 *
 *  Created on: 20 oct. 2011
 *      Author: laurent
 */

#include "ZActor_Player.h"
#include "ZGame.h"

ZActor_Player::ZActor_Player()
{
  ActorMode = 0;
  IsInLiquid     = false;
  IsFootInLiquid = false;
  IsHeadInLiquid = false;
  IsWalking = false;
  PlayerDensity = 1.040;
  LocationDensity = 0.0;
  PlayerSurface.x = 1.0;
  PlayerSurface.z = 1.0;
  PlayerSurface.y = 0.01;
  Speed_Walk = 20.0;
  PlaneSpeed = 0.0;
  PlaneCommandResponsiveness = 0.0;
  PlaneEngineThrust = 0.0;
  PlaneMinThrust = 5500.0;
  PlaneMaxThrust = 60000.0;
  PlaneEngineOn = false;
  PlaneTakenOff = false;
  PlaneLandedCounter = 0.0;
  PlaneToohighAlt = false;
  PlaneFreeFallCounter = 0.0;
  PlaneWaitForRectorStartSound = false;
  PlaneReactorSoundHandle = 0;
  WalkSoundHandle = 0;
  Test_T1 = 0;
  Riding_Voxel = 0;
  Riding_VoxelInfo = 0;
  Riding_IsRiding = 0;
  LastHelpTime = 1000000.0;
  LastHelpVoxel.x = 0;
  LastHelpVoxel.y = 0;
  LastHelpVoxel.z = 0;

  Inventory = new ZInventory();
  Init();
}

ZActor_Player::~ZActor_Player()
{
  if (Inventory) {delete Inventory; Inventory = 0;}
}

void ZActor_Player::Init(bool Death)
{

  // Player is alive and have some amount of lifepoints.

  IsDead = false;
  LifePoints = 1000.0;

  // Initial position and view direction
  ViewDirection.translate( 425, 0, 1975 );
  //ViewDirection.origin().x = 425.0; ViewDirection.origin().y = 0.0; ViewDirection.origin().z = 1975.0;
  ViewDirection.RotateYaw( 180 );
	ViewDirection.RotateRoll( -ViewDirection.roll() ); // just double make sure we're standing upright
	//ViewDirection.pitch = 0.0; ViewDirection.roll = 0.0; ViewDirection.yaw = 0.0;
  Velocity = 0.0;
  Deplacement = 0.0;

  // Camera settings.

  EyesPosition.x = 0; EyesPosition.y = 256 * 1.75 ; EyesPosition.z = 0.0; // Old eye y = 450

  Camera.orientation = ViewDirection;
  Camera.orientation.translate( EyesPosition.x, EyesPosition.y, EyesPosition.z );
  //Camera.x = ViewDirection.origin().x + EyesPosition.x; Camera.y = ViewDirection.origin().y + EyesPosition.y; Camera.z = ViewDirection.origin().z + EyesPosition.z;
  //Camera.Pitch = ViewDirection.pitch; Camera.Roll  = ViewDirection.roll; Camera.Yaw   = ViewDirection.yaw;
  Camera.ColoredVision.Activate = false;

  if (Death)
  {
    if (Riding_IsRiding)
    {
      if (GameEnv->VoxelTypeManager.VoxelTable[ Riding_Voxel ]->Is_HasAllocatedMemoryExtension) delete ((ZVoxelExtension * )Riding_VoxelInfo);
      Riding_IsRiding = false;
      Riding_VoxelInfo = 0;
      Riding_Voxel = 0;
    }
  }

  PlaneSpeed = 0.0;
  PlaneCommandResponsiveness = 0.0;
  PlaneEngineThrust = 0.0;
  PlaneEngineOn = false;
  PlaneTakenOff = false;
  PlaneLandedCounter = 0.0;
  PlaneToohighAlt = false;
  PlaneFreeFallCounter = 0.0;

  // Actor mode

  ActorMode = 0;

  // Inventory
  if ((!Death) || COMPILEOPTION_DONTEMPTYINVENTORYONDEATH!=1)
  {
    SetInitialInventory(Death);
  }

  // Time
  if (!Death) Time_TotalGameTime = 0;
  Time_ElapsedTimeSinceLastRespawn = 0;
}

void ZActor_Player::SetInitialInventory(bool Death)
{
  Inventory->Clear();

#if COMPILEOPTION_INVENTORY_DEMOCONTENT == 1

  // Demo Version inventory content

  Inventory->SetActualItemSlotNum(3);

  Inventory->SetSlot(ZInventory::Tools_StartSlot+0, 77,1);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+0 ,107,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+1 , 75,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+2 , 49,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+3 ,  1,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+4 ,  4,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+5 ,  3,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+6 , 22,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+7 , 63,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+8 , 11,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+9 , 30,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+10 , 37,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+11 , 47,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+12 , 26,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+13 , 74,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+14 ,109,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+15 ,110,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+16 ,111,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+17 ,112,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+18 , 27,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+19 , 52,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+20 , 87,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+21 , 92,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+22 , 88,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+23, 99,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+24 ,100,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+25 ,101,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+26 ,102,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+27, 96,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+28 ,108,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+29 , 90,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+30 , 94,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+31 , 97,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+32 , 98,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+33 ,103,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+34 ,104,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+35 ,105,4096);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+36 ,106,4096);
#else

  // Full version inventory content.

  Inventory->SetSlot(ZInventory::Tools_StartSlot+0, 42,1);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+0,107,1);
  Inventory->SetSlot(ZInventory::Inventory_StartSlot+1, 75,1);

#endif
}

void ZActor_Player::SetPosition( ZVector3d &NewLocation )
{
	ViewDirection.translate( NewLocation );

  Camera.orientation = ViewDirection;//.orientation;
  Camera.orientation.translate_rel( EyesPosition.x, EyesPosition.y,EyesPosition.z );
  //Camera.orientation.RotateRoll( -Camera.orientation.roll() );
  //Camera.orientation.translate( ViewDirection.origin().x + EyesPosition.x, ViewDirection.origin().y + EyesPosition.y,ViewDirection.origin().z+ EyesPosition.z );
  //Camera.x = ViewDirection.origin().x + EyesPosition.x;
  //Camera.y = ViewDirection.origin().y + EyesPosition.y;
  //Camera.z = ViewDirection.origin().z + EyesPosition.z;
  // printf("Camera.y: %lf\n", Camera.y);
}

void ZActor_Player::Action_MouseMove(Long Delta_x, Long Delta_y)
{
  if (ActorMode == 0 || ActorMode == 3)
  {
    double MouseRatio;

    MouseRatio = GameEnv->Settings_Hardware->Setting_MouseFactor;

	ViewDirection.Rotate( -Delta_x/(3*MouseRatio), -Delta_y/(3*MouseRatio), 0 );
	/*
    ViewDirection.yaw+=Delta_x/(3*MouseRatio);
    if (ViewDirection.yaw >= 360.0) ViewDirection.yaw -= 360.0;
    if (ViewDirection.yaw <0 ) ViewDirection.yaw += 360.0;

    ViewDirection.pitch-=Delta_y/(3*MouseRatio);
    if (ViewDirection.pitch >= 360.0) ViewDirection.pitch -= 360.0;
    if (ViewDirection.pitch <0 ) ViewDirection.pitch += 360.0;
    if (ViewDirection.pitch >= 360.0) ViewDirection.pitch -= 360.0;
    if (ViewDirection.pitch <0 ) ViewDirection.pitch += 360.0;
    ViewDirection.roll = 0.0;

    // Pitch clip
    if (ViewDirection.pitch >=90.0 && ViewDirection.pitch < 180.0) ViewDirection.pitch = 90.0;
    if (ViewDirection.pitch <270.0 && ViewDirection.pitch >=180.0) ViewDirection.pitch = 270.0;
    */
//    printf("Pitch: %lf\n", ViewDirection.pitch);

	//Camera.orientation = ViewDirection;
	/*
    Camera.Yaw = ViewDirection.yaw;
    Camera.Pitch = ViewDirection.pitch;
    Camera.Roll = ViewDirection.roll;
	*/
  }

  if (ActorMode == 1)
  {
    double MouseRatio;

    MouseRatio = GameEnv->Settings_Hardware->Setting_MouseFactor;

	/*
    ViewDirection.roll+=Delta_x/(6*MouseRatio);
    if (ViewDirection.roll >=90.0 && ViewDirection.roll < 180.0) ViewDirection.roll = 90;
    if (ViewDirection.roll <270.0 && ViewDirection.roll >=180.0) ViewDirection.roll = 270;
    if (ViewDirection.roll >= 360.0) ViewDirection.roll -= 360.0;
    if (ViewDirection.roll <0.0 ) ViewDirection.roll += 360.0;
	*/
	ViewDirection.Rotate( Delta_y/(6*MouseRatio) //* sin(ViewDirection.roll/57.295779513)
					, Delta_y/(6*MouseRatio) //* cos(ViewDirection.roll/57.295779513)
					, Delta_x/(6*MouseRatio) );

// Clip limit




	/*
    ViewDirection.pitch+=Delta_y/(6*MouseRatio) * cos(ViewDirection.roll/57.295779513);
    ViewDirection.yaw  +=Delta_y/(6*MouseRatio) * sin(ViewDirection.roll/57.295779513);
    if (ViewDirection.pitch >= 360.0) ViewDirection.pitch -= 360.0;
    if (ViewDirection.pitch <0.0 ) ViewDirection.pitch += 360.0;
    if (ViewDirection.yaw >= 360.0) ViewDirection.yaw -= 360.0;
    if (ViewDirection.yaw <0.0 ) ViewDirection.yaw += 360.0;
	*/

    // ViewDirection.yaw = 0.0;


	//Camera.orientation = ViewDirection;
  }



  if (ActorMode == 2)
  {
    double MouseRatio;

    if (IsDead) return;


    MouseRatio = GameEnv->Settings_Hardware->Setting_MouseFactor;

    if (!IsOnGround) 
		ViewDirection.RotateRoll( Delta_x/(6*MouseRatio)*PlaneCommandResponsiveness );
		//ViewDirection.roll+=Delta_x/(6*MouseRatio)*PlaneCommandResponsiveness;
    //
	ViewDirection.RotatePitch( Delta_y/(6*MouseRatio) * PlaneCommandResponsiveness );

    if (IsOnGround) 
	{
		if (PlaneSpeed > 500.0) 
			ViewDirection.RotateYaw( Delta_x/(64.0*MouseRatio) );
	}
	/*
    ViewDirection.pitch+=Delta_y/(6*MouseRatio) * cos(ViewDirection.roll/57.295779513) * PlaneCommandResponsiveness;
    if (IsOnGround) { if (PlaneSpeed > 500.0) ViewDirection.yaw += Delta_x/(64.0*MouseRatio);}
    else              ViewDirection.yaw  +=Delta_y/(6*MouseRatio) * sin(ViewDirection.roll/57.295779513) * PlaneCommandResponsiveness;
    if (ViewDirection.pitch >= 360.0) ViewDirection.pitch -= 360.0;
    if (ViewDirection.pitch <0.0 ) ViewDirection.pitch += 360.0;
    if (ViewDirection.yaw >= 360.0) ViewDirection.yaw -= 360.0;
    if (ViewDirection.yaw <0.0 ) ViewDirection.yaw += 360.0;
    if (ViewDirection.roll >= 360.0) ViewDirection.roll -= 360.0;
    if (ViewDirection.roll <0.0 ) ViewDirection.roll += 360.0;

    // Angle Clip limit

    if (ViewDirection.roll >=90.0 && ViewDirection.roll < 180.0) ViewDirection.roll = 90.0;
    if (ViewDirection.roll <270.0 && ViewDirection.roll >=180.0) ViewDirection.roll = 270.0;
    if (ViewDirection.pitch >=90.0 && ViewDirection.pitch < 180.0) ViewDirection.pitch = 90.0;
    if (ViewDirection.pitch <270.0 && ViewDirection.pitch >=180.0) ViewDirection.pitch = 270.0;
	*/

    // ViewDirection.yaw = 0.0;
  }

  Camera.orientation = ViewDirection;
  Camera.orientation.origin() += EyesPosition;
  
}

void ZActor_Player::Action_MouseButtonClick(ULong Button)
{
  UShort ToolNum;
  ZTool * Tool;
  if (IsDead) return;

  if( Button == 3 && GameEnv->Game_Events->Is_KeyPressed( SDLK_LSHIFT&0xFF, 0 ) )
  {
	  VoxelSelectDistance++;
	  return;
  }
  if( Button == 4 && GameEnv->Game_Events->Is_KeyPressed( SDLK_LSHIFT&0xFF, 0 ) )
  {
	  VoxelSelectDistance--;
	  return;
  }

  ToolNum = Inventory->GetSlotRef(ZInventory::Tools_StartSlot)->VoxelType;
  Tool = PhysicsEngine->GetToolManager()->GetTool(ToolNum);

  if (Tool)
  {
    Tool->Tool_MouseButtonClick(Button);
  }

  /*
  MouseButtonMatrix[Button] = 1;
  if (Button==0) PhysicsEngine->World->SetVoxel_WithCullingUpdate(PointedVoxel.PredPointedVoxel.x, PointedVoxel.PredPointedVoxel.y, PointedVoxel.PredPointedVoxel.z, BuildingMaterial);
  if (Button!=0) PhysicsEngine->World->SetVoxel_WithCullingUpdate(PointedVoxel.PointedVoxel.x, PointedVoxel.PointedVoxel.y, PointedVoxel.PointedVoxel.z,0);
  */
}

void ZActor_Player::Action_MouseButtonRelease( ULong Button)
{
  UShort ToolNum;
  ZTool * Tool;

  ToolNum = Inventory->GetSlotRef(ZInventory::Tools_StartSlot)->VoxelType;
  Tool = PhysicsEngine->GetToolManager()->GetTool(ToolNum);

  if (Tool)
  {
    Tool->Tool_MouseButtonRelease(Button);
  }
}

bool ZActor_Player::Action_StillEvents( bool * MouseMatrix, UByte * KeyboardMatrix)
{
  UShort ToolNum;
  ZTool * Tool;

  double FrameTime = GameEnv->Time_GameLoop;

  ToolNum = Inventory->GetSlotRef(ZInventory::Tools_StartSlot)->VoxelType;
  Tool = PhysicsEngine->GetToolManager()->GetTool(ToolNum);

  if (Tool)
  {
    Tool->Tool_StillEvents(FrameTime,MouseMatrix,KeyboardMatrix);
  }

  // Displacement in liquid.

  //if (IsInLiquid) printf("Mouille\n");

  if (ActorMode == 0 && IsInLiquid && KeyboardMatrix[GameEnv->Settings_Hardware->Setting_Key_Jump])
  {
    Velocity.y += 5.0*FrameTime;
  }

  return(true);
}

void ZActor_Player::Event_Collision(double RelativeVelocity )
{
  // 2 Cubes = -2251
  // 3 Cubes = -2753
  // 4 Cubes = -3169
  // 5 Cubes = -3586
  // 6 Cubes = -3919
  // 7 Cubes = -4253
  // 8 Cubes = -4503
  switch (ActorMode)
  {
    case 0:
      if (fabs(RelativeVelocity) > 5100.0)
      {
        // printf("Collision : %lf\n",RelativeVelocity);
        GameEnv->Sound->PlaySound(1);
        printf("Player is dead : Fatal Fall\n");
        #if COMPILEOPTION_FALLAREFATALS==1
        Event_DeadlyFall();
        #endif
      }
      break;
    case 2:
      if (this->PlaneTakenOff && fabs(RelativeVelocity) > 60.0)
      {
        // printf("Velocity:%lf CycleTime%lf\n",RelativeVelocity, GameEnv->Time_GameLoop);
        Event_PlaneCrash();
        GameEnv->Sound->PlaySound(1);
      }
      break;
  }
}
/*
bool ZActor_Player::Save( ZStream_SpecialRamStream * OutStream )
{
  ULong *Size,StartLen,EndLen;
  OutStream->PutString("BLKPLAYR");
  OutStream->Put( (UShort)1); // Version
  OutStream->Put( (UShort)1); // Compatibility Class;
  Size = OutStream->GetPointer_ULong();
  OutStream->Put((ULong)0xA600DBED); // Size of the chunk, will be written later.
  StartLen = OutStream->GetActualBufferLen();

  OutStream->Put( ViewDirection.origin().x ); OutStream->Put( ViewDirection.origin().y ); OutStream->Put( ViewDirection.origin().z );
  OutStream->Put( Velocity.x ); OutStream->Put( Velocity.y ); OutStream->Put( Velocity.z );
  OutStream->Put( Deplacement.x ); OutStream->Put( Deplacement.y ); OutStream->Put( Deplacement.z );
  OutStream->Put( ViewDirection.pitch ); OutStream->Put( ViewDirection.roll ); OutStream->Put( ViewDirection.yaw );
  OutStream->Put( Camera.x ); OutStream->Put( Camera.y ); OutStream->Put( Camera.z );
  OutStream->Put( Camera.Pitch ); OutStream->Put( Camera.Roll ); OutStream->Put( Camera.Yaw );

  OutStream->Put( (ULong)LifePoints);
  Inventory->Save(OutStream);

  EndLen = OutStream->GetActualBufferLen();
  *Size = EndLen - StartLen;

  return(true);
}
*/

bool ZActor_Player::Save( ZStream_SpecialRamStream * OutStream )
{
  ULong *Size,StartLen,*ExtensionSize,EndLen, StartExtensionLen;
  OutStream->PutString("BLKPLYR4");
  OutStream->Put( (UShort)3); // Version
  OutStream->Put( (UShort)3); // Compatibility Class;
  Size = OutStream->GetPointer_ULong();
  OutStream->Put((ULong)0xA600DBED); // Size of the chunk, will be written later.
  StartLen = OutStream->GetActualBufferLen();

  ZVector3d position = ViewDirection.origin() * 256.0/GlobalSettings.VoxelBlockSize;

  OutStream->Put( position.x ); OutStream->Put( position.y ); OutStream->Put( position.z );
  OutStream->Put( Velocity.x ); OutStream->Put( Velocity.y ); OutStream->Put( Velocity.z );
  //OutStream->Put( Deplacement.x ); OutStream->Put( Deplacement.y ); OutStream->Put( Deplacement.z );
  OutStream->Put( ViewDirection.quat()[0] ); OutStream->Put( ViewDirection.quat()[1] ); OutStream->Put( ViewDirection.quat()[2] ); OutStream->Put( ViewDirection.quat()[3] );
  //OutStream->Put( ViewDirection.pitch() ); OutStream->Put( ViewDirection.roll ); OutStream->Put( ViewDirection.yaw );
  //OutStream->Put( Camera.x() ); OutStream->Put( Camera.y() ); OutStream->Put( Camera.z() );
  //OutStream->Put( Camera.orientation.quat()[0] ); OutStream->Put( Camera.orientation.quat()[1] ); OutStream->Put( Camera.orientation.quat()[2] ); OutStream->Put( Camera.orientation.quat()[3] );
  //OutStream->Put( Camera.Pitch ); OutStream->Put( Camera.Roll ); OutStream->Put( Camera.Yaw );
  OutStream->Put( (ULong)LifePoints);

  // printf("Offset : %ld\n",OutStream->GetOffset());

  Inventory->Save(OutStream);

  // New for version 2

  OutStream->Put(ActorMode);
  OutStream->Put(IsInLiquid);
  OutStream->Put(IsFootInLiquid);
  OutStream->Put(IsHeadInLiquid);
  OutStream->Put(LocationDensity);

  OutStream->Put(Riding_IsRiding);
  OutStream->Put(Riding_Voxel);
  OutStream->Put((ULong)Riding_VoxelInfo);
  OutStream->Put(PlaneSpeed);
  OutStream->Put(PlaneCommandResponsiveness);
  OutStream->Put(PlaneEngineThrust);
  OutStream->Put(PlaneEngineOn);
  OutStream->Put(PlaneTakenOff);
  OutStream->Put(PlaneLandedCounter);
  OutStream->Put(PlaneToohighAlt);
  OutStream->Put(Time_TotalGameTime);
  OutStream->Put(Time_ElapsedTimeSinceLastRespawn);
  OutStream->PutString("RIDEXTEN");
  ExtensionSize = OutStream->GetPointer_ULong();
  OutStream->Put((ULong) 0xA600DBED );
  StartExtensionLen = OutStream->GetActualBufferLen();
  if (GameEnv->VoxelTypeManager.VoxelTable[Riding_Voxel]->Is_HasAllocatedMemoryExtension)
  {
    OutStream->Put((bool)true);
    ((ZVoxelExtension * )Riding_VoxelInfo)->Save(OutStream);
  }
  else OutStream->Put((bool)false);
  *ExtensionSize = OutStream->GetActualBufferLen() - StartExtensionLen;

  EndLen = OutStream->GetActualBufferLen();
  *Size = EndLen - StartLen;

  return(true);
}

bool ZActor_Player::Load( ZStream_SpecialRamStream * InStream)
{
  ZString Section_Name;
  UShort   Section_Version;
  UShort   Section_CompatibilityClass;
  ULong   Section_Size, Tmp_UL;
  bool Ok;
  Section_Name.SetLen(8);

  Ok = true;
  Ok &= InStream->GetStringFixedLen(Section_Name.String,8);
  Ok &= InStream->Get(Section_Version);
  Ok &= InStream->Get(Section_CompatibilityClass);
  Ok &= InStream->Get(Section_Size);

  if (! Ok) return(false);

  // Implemented the version 2 format because version 1 was buggy and compatibility can't be assured without changing the section name.
  if (Section_Name == "BLKPLYR2")
  {
    Ok &= InStream->Get( ViewDirection.origin().x ); InStream->Get( ViewDirection.origin().y ); InStream->Get( ViewDirection.origin().z );
    Ok &= InStream->Get( Velocity.x ); InStream->Get( Velocity.y ); InStream->Get( Velocity.z );
    Ok &= InStream->Get( Deplacement.x ); InStream->Get( Deplacement.y ); InStream->Get( Deplacement.z );
	//double v[3];
    //Ok &= InStream->Get( v[0] ); InStream->Get( ViewDirection.roll ); InStream->Get( ViewDirection.yaw );
	//ViewDirection.RotatePitch
    
	double v[3];
    Ok &= InStream->Get( v[0] ); InStream->Get( v[1] ); InStream->Get( v[2] );
	//Camera.orientation.rotate( v[0], v[1], v[2] );
	ViewDirection.RotateAbsolute( v[0], v[1], v[2] );
    //Ok &= InStream->Get( Camera.x() ); InStream->Get( Camera.y() ); InStream->Get( Camera.z() );
    Ok &= InStream->Get( v[0] ); InStream->Get( v[1] ); InStream->Get( v[2] );
	Camera.orientation.translate( v[0], v[1], v[2] );
	double q[4];
    Ok &= InStream->Get( q[0] ); InStream->Get( q[1] ); InStream->Get( q[2] ); 
	//InStream->Get( q[3] );
	Camera.orientation.RotateAbsolute( q[0], q[1], q[2] );
	//Camera.orientation.quat( q );
	//ViewDirection.quat( q );
    //Ok &= InStream->Get( Camera.Pitch ); InStream->Get( Camera.Roll ); InStream->Get( Camera.Yaw );
    Ok &= InStream->Get( Tmp_UL ); LifePoints = Tmp_UL; // Corrected
    if (!Ok) return(false);

    if (!Inventory->Load(InStream)) return(false);

    // New for version 2

    ULong ExtensionBlocSize;
    bool IsExtensionToLoad;

    Ok &= InStream->Get(ActorMode);
    Ok &= InStream->Get(IsInLiquid);
    Ok &= InStream->Get(IsFootInLiquid);
    Ok &= InStream->Get(IsHeadInLiquid);
    Ok &= InStream->Get(LocationDensity);

    Ok &= InStream->Get(Riding_IsRiding);
    Ok &= InStream->Get(Riding_Voxel);
    Ok &= InStream->Get(Tmp_UL); Riding_VoxelInfo = Tmp_UL; // Corrected
    Ok &= InStream->Get(PlaneSpeed);
    Ok &= InStream->Get(PlaneCommandResponsiveness);
    Ok &= InStream->Get(PlaneEngineThrust);
    Ok &= InStream->Get(PlaneEngineOn);
    Ok &= InStream->Get(PlaneTakenOff);
    Ok &= InStream->Get(PlaneLandedCounter);
    Ok &= InStream->Get(PlaneToohighAlt);

    Ok &= InStream->Get(Time_TotalGameTime); // New for V3
    Ok &= InStream->Get(Time_ElapsedTimeSinceLastRespawn); // New for V3

    Ok &= InStream->GetStringFixedLen(Section_Name.String,8);
    if (!(Ok && Section_Name == "RIDEXTEN" )) return(false);

    Ok &= InStream->Get(ExtensionBlocSize);

    Ok &= InStream->Get(IsExtensionToLoad);
    if (!Ok) return(false);
    if (GameEnv->VoxelTypeManager.VoxelTable[Riding_Voxel]->Is_HasAllocatedMemoryExtension)
    {
      Riding_VoxelInfo = (ZMemSize)GameEnv->VoxelTypeManager.VoxelTable[Riding_Voxel]->CreateVoxelExtension(); if (Riding_VoxelInfo == 0 ) return(false);
      if (IsExtensionToLoad)
      {
        ((ZVoxelExtension * )Riding_VoxelInfo)->Load(InStream);
      }
    }
    return(true);
  }

  if (Section_Name == "BLKPLYR3")
  {
    Ok &= InStream->Get( ViewDirection.origin().x ); InStream->Get( ViewDirection.origin().y ); InStream->Get( ViewDirection.origin().z );
	ViewDirection.origin() *= GlobalSettings.VoxelBlockSize/256.0;
    Ok &= InStream->Get( Velocity.x ); InStream->Get( Velocity.y ); InStream->Get( Velocity.z );
    Ok &= InStream->Get( Deplacement.x ); InStream->Get( Deplacement.y ); InStream->Get( Deplacement.z );
	//double v[3];
    //Ok &= InStream->Get( v[0] ); InStream->Get( ViewDirection.roll ); InStream->Get( ViewDirection.yaw );
	//ViewDirection.RotatePitch
	    
	double v[4];
    Ok &= InStream->Get( v[0] ); InStream->Get( v[1] ); InStream->Get( v[2] ); InStream->Get( v[3] );
	//Camera.orientation.rotate( v[0], v[1], v[2] );
	ViewDirection.quat( v );
    //Ok &= InStream->Get( Camera.x() ); InStream->Get( Camera.y() ); InStream->Get( Camera.z() );
    //Ok &= InStream->Get( v[0] ); InStream->Get( v[1] ); InStream->Get( v[2] );
	//Camera.orientation.translate( v[0], v[1], v[2] );
	//double q[4];
    //Ok &= InStream->Get( q[0] ); InStream->Get( q[1] ); InStream->Get( q[2] ); 
	//InStream->Get( q[3] );
	//Camera.orientation.RotateAbsolute( q[0], q[1], q[2] );
	//Camera.orientation.quat( q );
	//ViewDirection.quat( q );
	SetPosition( ViewDirection.origin() );
    //Ok &= InStream->Get( Camera.Pitch ); InStream->Get( Camera.Roll ); InStream->Get( Camera.Yaw );
    Ok &= InStream->Get( Tmp_UL ); LifePoints = Tmp_UL; // Corrected
    if (!Ok) return(false);

    if (!Inventory->Load(InStream)) return(false);

    // New for version 2

    ULong ExtensionBlocSize;
    bool IsExtensionToLoad;

    Ok &= InStream->Get(ActorMode);
    Ok &= InStream->Get(IsInLiquid);
    Ok &= InStream->Get(IsFootInLiquid);
    Ok &= InStream->Get(IsHeadInLiquid);
    Ok &= InStream->Get(LocationDensity);

    Ok &= InStream->Get(Riding_IsRiding);
    Ok &= InStream->Get(Riding_Voxel);
    Ok &= InStream->Get(Tmp_UL); Riding_VoxelInfo = Tmp_UL; // Corrected
    Ok &= InStream->Get(PlaneSpeed);
    Ok &= InStream->Get(PlaneCommandResponsiveness);
    Ok &= InStream->Get(PlaneEngineThrust);
    Ok &= InStream->Get(PlaneEngineOn);
    Ok &= InStream->Get(PlaneTakenOff);
    Ok &= InStream->Get(PlaneLandedCounter);
    Ok &= InStream->Get(PlaneToohighAlt);

    Ok &= InStream->Get(Time_TotalGameTime); // New for V3
    Ok &= InStream->Get(Time_ElapsedTimeSinceLastRespawn); // New for V3

    Ok &= InStream->GetStringFixedLen(Section_Name.String,8);
    if (!(Ok && Section_Name == "RIDEXTEN" )) return(false);

    Ok &= InStream->Get(ExtensionBlocSize);

    Ok &= InStream->Get(IsExtensionToLoad);
    if (!Ok) return(false);
    if (GameEnv->VoxelTypeManager.VoxelTable[Riding_Voxel]->Is_HasAllocatedMemoryExtension)
    {
      Riding_VoxelInfo = (ZMemSize)GameEnv->VoxelTypeManager.VoxelTable[Riding_Voxel]->CreateVoxelExtension(); if (Riding_VoxelInfo == 0 ) return(false);
      if (IsExtensionToLoad)
      {
        ((ZVoxelExtension * )Riding_VoxelInfo)->Load(InStream);
      }
    }
    return(true);
  }

  if (Section_Name == "BLKPLYR4")
  {
    Ok &= InStream->Get( ViewDirection.origin().x ); InStream->Get( ViewDirection.origin().y ); InStream->Get( ViewDirection.origin().z );
	ViewDirection.origin() *= GlobalSettings.VoxelBlockSize/256.0;
    Ok &= InStream->Get( Velocity.x ); InStream->Get( Velocity.y ); InStream->Get( Velocity.z );
    //Ok &= InStream->Get( Deplacement.x ); InStream->Get( Deplacement.y ); InStream->Get( Deplacement.z );
	//double v[3];
    //Ok &= InStream->Get( v[0] ); InStream->Get( ViewDirection.roll ); InStream->Get( ViewDirection.yaw );
	//ViewDirection.RotatePitch
	    
	double v[4];
    Ok &= InStream->Get( v[0] ); InStream->Get( v[1] ); InStream->Get( v[2] ); InStream->Get( v[3] );
	//Camera.orientation.rotate( v[0], v[1], v[2] );
	ViewDirection.quat( v );
    //Ok &= InStream->Get( Camera.x() ); InStream->Get( Camera.y() ); InStream->Get( Camera.z() );
    //Ok &= InStream->Get( v[0] ); InStream->Get( v[1] ); InStream->Get( v[2] );
	//Camera.orientation.translate( v[0], v[1], v[2] );
	//double q[4];
    //Ok &= InStream->Get( q[0] ); InStream->Get( q[1] ); InStream->Get( q[2] ); 
	//InStream->Get( q[3] );
	//Camera.orientation.RotateAbsolute( q[0], q[1], q[2] );
	//Camera.orientation.quat( q );
	//ViewDirection.quat( q );
	SetPosition( ViewDirection.origin() );
    //Ok &= InStream->Get( Camera.Pitch ); InStream->Get( Camera.Roll ); InStream->Get( Camera.Yaw );
    Ok &= InStream->Get( Tmp_UL ); LifePoints = Tmp_UL; // Corrected
    if (!Ok) return(false);

    if (!Inventory->Load(InStream)) return(false);

    // New for version 2

    ULong ExtensionBlocSize;
    bool IsExtensionToLoad;

    Ok &= InStream->Get(ActorMode);
    Ok &= InStream->Get(IsInLiquid);
    Ok &= InStream->Get(IsFootInLiquid);
    Ok &= InStream->Get(IsHeadInLiquid);
    Ok &= InStream->Get(LocationDensity);

    Ok &= InStream->Get(Riding_IsRiding);
    Ok &= InStream->Get(Riding_Voxel);
    Ok &= InStream->Get(Tmp_UL); Riding_VoxelInfo = Tmp_UL; // Corrected
    Ok &= InStream->Get(PlaneSpeed);
    Ok &= InStream->Get(PlaneCommandResponsiveness);
    Ok &= InStream->Get(PlaneEngineThrust);
    Ok &= InStream->Get(PlaneEngineOn);
    Ok &= InStream->Get(PlaneTakenOff);
    Ok &= InStream->Get(PlaneLandedCounter);
    Ok &= InStream->Get(PlaneToohighAlt);

    Ok &= InStream->Get(Time_TotalGameTime); // New for V3
    Ok &= InStream->Get(Time_ElapsedTimeSinceLastRespawn); // New for V3

    Ok &= InStream->GetStringFixedLen(Section_Name.String,8);
    if (!(Ok && Section_Name == "RIDEXTEN" )) return(false);

    Ok &= InStream->Get(ExtensionBlocSize);

    Ok &= InStream->Get(IsExtensionToLoad);
    if (!Ok) return(false);
    if (GameEnv->VoxelTypeManager.VoxelTable[Riding_Voxel]->Is_HasAllocatedMemoryExtension)
    {
      Riding_VoxelInfo = (ZMemSize)GameEnv->VoxelTypeManager.VoxelTable[Riding_Voxel]->CreateVoxelExtension(); if (Riding_VoxelInfo == 0 ) return(false);
      if (IsExtensionToLoad)
      {
        ((ZVoxelExtension * )Riding_VoxelInfo)->Load(InStream);
      }
    }
    return(true);
  }

  if (Section_Name == "BLKPLAYR")
  {
    Ok &= InStream->Get( ViewDirection.origin().x ); InStream->Get( ViewDirection.origin().y ); InStream->Get( ViewDirection.origin().z );
    Ok &= InStream->Get( Velocity.x ); InStream->Get( Velocity.y ); InStream->Get( Velocity.z );
    Ok &= InStream->Get( Deplacement.x ); InStream->Get( Deplacement.y ); InStream->Get( Deplacement.z );
    //Ok &= InStream->Get( ViewDirection.pitch ); InStream->Get( ViewDirection.roll ); InStream->Get( ViewDirection.yaw );
	double v[3];
    Ok &= InStream->Get( v[0] ); InStream->Get( v[1] ); InStream->Get( v[2] );
	Camera.orientation.translate( v );
	double q[4];
    Ok &= InStream->Get( q[0] ); InStream->Get( q[1] ); InStream->Get( q[2] ); InStream->Get( q[3] );
	Camera.orientation.quat( q );
	ViewDirection.quat( q );
    //Ok &= InStream->Get( Camera.Pitch ); InStream->Get( Camera.Roll ); InStream->Get( Camera.Yaw );
    Ok &= InStream->Get( Tmp_UL ); LifePoints = Tmp_UL; // Corrected

    if (!Ok) return(false);

    if (!Inventory->Load(InStream)) return(false);

    // New for version 2

    ULong ExtensionBlocSize;
    bool IsExtensionToLoad;

    Ok &= InStream->Get(ActorMode);
    Ok &= InStream->Get(IsInLiquid);
    Ok &= InStream->Get(IsFootInLiquid);
    Ok &= InStream->Get(IsHeadInLiquid);
    Ok &= InStream->Get(LocationDensity);

    Ok &= InStream->Get(Riding_IsRiding);
    Ok &= InStream->Get(Riding_Voxel);
    Ok &= InStream->Get(Tmp_UL); Riding_VoxelInfo = Tmp_UL; // Corrected
    Ok &= InStream->Get(PlaneSpeed);
    Ok &= InStream->Get(PlaneCommandResponsiveness);
    Ok &= InStream->Get(PlaneEngineThrust);
    Ok &= InStream->Get(PlaneEngineOn);
    Ok &= InStream->Get(PlaneTakenOff);
    Ok &= InStream->Get(PlaneLandedCounter);
    Ok &= InStream->Get(PlaneToohighAlt);
/*
    if (Section_Version > 2)
    {
      Ok &= InStream->Get(Time_TotalGameTime);
      Ok &= InStream->Get(Time_ElapsedTimeSinceLastRespawn);
    }
*/
    Ok &= InStream->GetStringFixedLen(Section_Name.String,8);
    if (!(Ok && Section_Name == "RIDEXTEN" )) return(false);

    Ok &= InStream->Get(ExtensionBlocSize);

    Ok &= InStream->Get(IsExtensionToLoad);
    if (!Ok) return(false);
    if (GameEnv->VoxelTypeManager.VoxelTable[Riding_Voxel]->Is_HasAllocatedMemoryExtension)
    {
      Riding_VoxelInfo = (ZMemSize)GameEnv->VoxelTypeManager.VoxelTable[Riding_Voxel]->CreateVoxelExtension(); if (Riding_VoxelInfo == 0 ) return(false);
      if (IsExtensionToLoad)
      {
        ((ZVoxelExtension * )Riding_VoxelInfo)->Load(InStream);
      }
    }
    return(true);
  }

  return(false);
}

void ZActor_Player::DoPhysic(UELong FrameTime)
{
  double CycleTime = FrameTime / 1000.0;

  if (ActorMode == 0) { DoPhysic_GroundPlayer(CycleTime); return; }
  // if (ActorMode == 1) { DoPhysic_Plane_Old(CycleTime); return; }
  if (ActorMode == 2) { DoPhysic_Plane(CycleTime); return; }
  if (ActorMode == 3) { DoPhysic_SupermanPlayer(CycleTime); return; }

}

void ZActor_Player::DoPhysic_Plane(double CycleTime)
{
  ZVector3d P[32];
  UShort Voxel[32];
  ZVoxelType * VoxelType[32];
  bool   IsEmpty[32];
  ZVoxelWorld * World;
  ZVector3d Dep,Dep2;
  double DepLen;
  ULong i;

  double CapedCycleTime;

  // Sound of the reactor

  if (PlaneEngineOn && (!IsDead))
  {

     if (PlaneReactorSoundHandle==0 || PlaneWaitForRectorStartSound) {PlaneReactorSoundHandle = GameEnv->Sound->Start_PlaySound(3,true, false, 1.00,0); PlaneWaitForRectorStartSound = false; }
     else                             GameEnv->Sound->ModifyFrequency(PlaneReactorSoundHandle, (PlaneEngineThrust) / 60000.0 + 1.0);

  }


  // Caped cycle time for some calculation to avoid inconsistency

  CapedCycleTime = CycleTime;
  if (CapedCycleTime > 5.0) CapedCycleTime = 5.0;

  // Colored vision off. May be reactivated further.

  Camera.ColoredVision.Activate = false;

  // Define Detection points

  P[0] = ViewDirection.origin() + ZVector3d(0,0,0);   // #
  P[1] = ViewDirection.origin() + ZVector3d(0,128,0);   // #
  P[2] = ViewDirection.origin() + ZVector3d(0,-128,0);

  // Get the Voxel Informations

  World = PhysicsEngine->World;
  for (i=0;i<3;i++)
  {
    Voxel[i]     = World->GetVoxelPlayerCoord_Secure(P[i].x,P[i].y,P[i].z);
    VoxelType[i] = GameEnv->VoxelTypeManager.VoxelTable[Voxel[i]];
    IsEmpty[i]   = VoxelType[i]->Is_PlayerCanPassThrough;
  }

  IsOnGround = !IsEmpty[2];

  // Crash if collision with bloc

  if (!VoxelType[1]->Is_PlayerCanPassThrough) Event_PlaneCrash();

  // Angle computing

  if (ViewDirection.roll() >=-90.0 && ViewDirection.roll() <=90.0) 
	  ViewDirection.RotateAround( ZVector3d( 0, 1, 0 ), -(ViewDirection.roll() / 1440.0 * PlaneCommandResponsiveness)  * CycleTime );
  //if (ViewDirection.roll() <0.0 && ViewDirection.roll() >= -90 ) 
  //	  ViewDirection.RotateAround( ZVector3d( 0, 1, 0 ), ( - ( - ViewDirection.roll()) / 1440.0 * PlaneCommandResponsiveness) * CycleTime );

  // Take plane on ground if speed < 8000

  if (! PlaneTakenOff)
  {
    if (PlaneSpeed <= 7000.0)
    {
		if (IsOnGround) { 
			double pitch = ViewDirection.pitch();
			double roll = ViewDirection.roll();
			if( pitch )
				ViewDirection.RotatePitch( ViewDirection.pitch() ); 
			if( roll )
				ViewDirection.RotateRoll( -ViewDirection.roll() );
		}
    }
    else
    {
      if (!IsOnGround) PlaneTakenOff = true;
    }

  }

  // Landing : Crash if Landing at speed > 3000

  if (PlaneTakenOff && (!IsDead))
  {
    if (IsOnGround)
    {
		double pitch = ViewDirection.pitch();
		double roll = ViewDirection.roll();
      if (   PlaneSpeed > 3000.0  || PlaneSpeed < 1300.0                                     // Too Fast or too slow = Crash
          || (pitch  < -30.0 || pitch > 30.0 )                   // Bad pitch = Crash.
          || (roll  < -30.0 || roll > 30.0)                      // Bad roll  = Crash.
          )
      {
        Event_PlaneCrash();
      }
      if (pitch>0 && pitch < 45.0) { 
		  ViewDirection.RotatePitch( -0.0225 * CycleTime ); 
		  //if (ViewDirection.pitch() < 0.0 ) ViewDirection.pitch() = 0.0; 
		}
	  if (pitch>-45 && pitch<= 0) { ViewDirection.RotatePitch( + 0.0225 * CycleTime ); 
		  //if (pitch >= 360.0 ) pitch = 0.0; 
		  }
	  if (roll >0.5 && roll < 90.0) ViewDirection.RotateRoll(  - 0.0225 * CycleTime );
      if (roll <0.5 && roll < -90.0) ViewDirection.RotateRoll(  + 0.0225 * CycleTime );
      PlaneLandedCounter += CycleTime;
      if (PlaneLandedCounter > 2000.0 ) { 
		  //DebugBreak();
		  ViewDirection.RotatePitch( pitch ); 
		  ViewDirection.RotateRoll( -roll ); 
		 // ViewDirection.roll() = 0.0; 
		  PlaneTakenOff = false; 
		  PlaneLandedCounter = 0.0; 
	  }
    }
  }

  // Engine power

  //ZMatrix Direction = ViewDirection;
  ZVector3d CDir;
  //Direction.Len = 1.0;
  CDir = ViewDirection.z_axis();
  CDir.x *= -1;
  CDir.y *= -1;
  CDir.z *= -1;

  // Engine Thrust acceleration.

  // PlaneSpeed += -(( PlaneSpeed - PlaneEngineThrust ) / 100 * CycleTime);
  PlaneSpeed += PlaneEngineThrust / 8000 * CycleTime * ((PlaneTakenOff) ? 1.0 : 0.50);

  // Altitude change modify cynetic energy.

  if (!PlaneEngineOn)
  {
    if (ViewDirection.pitch() <0.0 && ViewDirection.pitch() >= -90.0)
    {
      // PlaneSpeed += -Velocity.y * sin((360.0 - ViewDirection.pitch)/57.295779513) / 100.0;
      PlaneSpeed += sin((-ViewDirection.pitch())/57.295779513) * CycleTime;
    }
    if (ViewDirection.pitch() >0.0 && ViewDirection.pitch() <= 90.0)
    {
      // PlaneSpeed +=  -Velocity.y * sin(ViewDirection.pitch/57.295779513) / 100.0;
      PlaneSpeed +=  -sin(ViewDirection.pitch()/57.295779513) * CycleTime / 2.0;

    }
  }

  // Insuficient speed make command more difficult.

  PlaneCommandResponsiveness = (PlaneSpeed - 1000) / 2000.0;

  if (PlaneCommandResponsiveness < 0.0) PlaneCommandResponsiveness = 0.0;
  if (PlaneSpeed >= 4000.0 || IsOnGround ) PlaneCommandResponsiveness = 1.0;
  if (PlaneToohighAlt) PlaneCommandResponsiveness = 0.0;
  if (IsDead) PlaneCommandResponsiveness = 0.0;



  // Insuficient speed lower portance and make nasty effects

  if      (PlaneSpeed < 2000.0) CDir.y = 0.0;
  else if (PlaneSpeed < 4000.0) CDir.y *= (PlaneSpeed - 2000.0) / 2000.0;
  double roll = ViewDirection.roll();
   if (PlaneCommandResponsiveness < 0.5 && (ViewDirection.roll() < -15.0 || ViewDirection.roll() >15.0)) 
	   ViewDirection.RotateRoll(0.01 * CycleTime );

  // The viscous friction loss...

  double FrictionCoef, PlaneSCX;
  ZVector3d Frottement;

  FrictionCoef = GameEnv->VoxelTypeManager.VoxelTable[Voxel[0]]->FrictionCoef;
  PlaneSCX = 1.0;
  PlaneSpeed /= (PlaneSpeed * PlaneSpeed * PlaneSCX / 100000000000.0 * CycleTime) + 1.0;
  Frottement = (Velocity * Velocity * FrictionCoef * PlayerSurface / 10000000.0 * CycleTime) + 1.0;
  Velocity /= Frottement;

  // Velocity += -(Velocity - (CDir * PlaneSpeed) );  // * PlaneCommandResponsiveness ;

  Velocity += -(Velocity - (CDir * PlaneSpeed) ) * PlaneCommandResponsiveness ;

  if (IsOnGround)
  {
    if (PlaneSpeed >0.0) {PlaneSpeed-= 0.05*CycleTime;  if (PlaneSpeed < 0.0) PlaneSpeed = 0.0; }
    if (Velocity.x > 0.0) { Velocity.x -= 0.05*CycleTime; }
    if (Velocity.x < 0.0) { Velocity.x += 0.05*CycleTime; }
    if (Velocity.z > 0.0) { Velocity.z -= 0.05*CycleTime; }
    if (Velocity.z < 0.0) { Velocity.z += 0.05*CycleTime; }
  }

  // *** Gravity

  if ( (!PlaneEngineOn) || PlaneSpeed < 4000.0 || (!PlaneTakenOff)  )
  {
    double Gravity, CubeY;
	CubeY = ViewDirection.origin().y / GlobalSettings.VoxelBlockSize;
    if      (CubeY > 10000.0 && CubeY < 15000.0) { Gravity = 5.0 - (( (CubeY-10000.0) * (CubeY-10000.0)) / 5000000.0); } //5000000.0;
    else if (CubeY <= 10000.0) { Gravity = 5.0; }
    else                       { Gravity = 0.0; }

    Velocity.y -= (10.0 - LocationDensity) * Gravity * CapedCycleTime * ((PlaneTakenOff || IsOnGround) ? 1.0 : 10.0);
    if (PlaneToohighAlt && ViewDirection.origin().y > 256000.0 ) 
	{
		if (Velocity.y < -250.0)  Velocity.y = -250*GlobalSettings.VoxelBlockSize; 
	}
    else                                           
	{
		if (Velocity.y < -50*GlobalSettings.VoxelBlockSize) Velocity.y = -50*GlobalSettings.VoxelBlockSize; 
	}
  }

  // If going too high, something nasty will happens

  if (ViewDirection.origin().y > (5000.0 * GlobalSettings.VoxelBlockSize) && (!PlaneToohighAlt) )
  {
    PlaneToohighAlt = true;
    PlaneEngineThrust = 0.0;
    PlaneEngineOn = false;
    GameEnv->GameWindow_Advertising->Advertise("ENGINE STALLED",ZGameWindow_Advertising::VISIBILITY_MEDIUM,0,3000.0, 3000.0 );
    GameEnv->GameWindow_Advertising->Advertise("PLANE IS FREE FALLING",ZGameWindow_Advertising::VISIBILITY_MEDLOW,0,3000.0, 3000.0 );

  }
  if (PlaneToohighAlt)
  {
    if (ViewDirection.origin().y > (500.0 * GlobalSettings.VoxelBlockSize) )
	{
		Camera.ColoredVision.Activate= true; 
		Camera.ColoredVision.Red = 0.8f; 
		Camera.ColoredVision.Green = 0.0; 
		Camera.ColoredVision.Blue = 0.0; 
		Camera.ColoredVision.Opacity = 0.3f;
	}
    //ViewDirection.pitch = 270.0;
    //ViewDirection.roll = 0.0;
    PlaneFreeFallCounter += CycleTime;
    if (PlaneFreeFallCounter > 1000.0)
    {
      PlaneFreeFallMessage = "FALL WARNING : ALTITUDE ";
      PlaneFreeFallMessage << ((Long)(floor(ViewDirection.origin().y/GlobalSettings.VoxelBlockSize)));
      PlaneFreeFallCounter = 0.0;
      GameEnv->GameWindow_Advertising->Advertise(PlaneFreeFallMessage.String ,ZGameWindow_Advertising::VISIBILITY_LOW,0,800.0, 400.0 );
    }
  }

  // Printing data
/*
  Test_T1++; if (Test_T1 > 100 )
  {
    Test_T1 = 0;
    printf("Speed : %lf Pitch: %lf Thrust:%lf Altitude:%lf IsOnGround:%d TakenOff:%d Gravity:%d CycleTime %lf\n", PlaneSpeed, ViewDirection.pitch, PlaneEngineThrust, ViewDirection.origin().y / GlobalSettings.VoxelBlockSize, IsOnGround, PlaneTakenOff, GravityApplied, CycleTime);
  }
*/
  // Velocity to displacement.

  Dep = Velocity * CycleTime / 1000.0;
  DepLen = sqrt(Dep.x*Dep.x + Dep.y*Dep.y + Dep.z*Dep.z);

  // Collision detection loop and displacement correction

  ZRayCast_in In;
  ZRayCast_out Out[32];
  double DistanceMin;
  Long CollisionIndice;
  Bool Collided;
  Bool Continue;
  Bool PEnable[32];


  In.Camera = 0;
  In.MaxCubeIterations = (ULong)ceil(DepLen / 256)+5; // 6;
  In.PlaneCubeDiff = In.MaxCubeIterations - 3;
  In.MaxDetectionDistance = 3000000.0;

  // ****
  for (i=0;i<24;i++) {PEnable[i] = true;}
  Continue = true;
  if ( (Dep.x == 0) && (Dep.y == 0) && (Dep.z == 0.0) ) { Continue = false; return; }


  while (Continue)
  {

    Collided = false;
    DistanceMin = 10000000.0;
    CollisionIndice = -1;
    for (i=0;i<1;i++)
    {

      if (PEnable[i]) // (PEnable[i])
      {
        bool ret;
        bool Redo;

        do
        {
          Redo = false;
          ret = World->RayCast_Vector(P[i],Dep , &In, &(Out[i]), false);         // Normal points.
          if (ret)
          {
            if (Out[i].CollisionDistance < DistanceMin )
            {
              if ( (Out[i].CollisionDistance <= DepLen) && (65535 == World->GetVoxel( Out[i].PointedVoxel.x, Out[i].PointedVoxel.y, Out[i].PointedVoxel.z )) )
              {
                World->GetVoxel_Secure( Out[i].PointedVoxel.x, Out[i].PointedVoxel.y, Out[i].PointedVoxel.z );
                Redo = true;
              }
              else
              {
                Collided = true; DistanceMin = Out[i].CollisionDistance; CollisionIndice = i;
              }
            }

          }
        } while (Redo);


      }
    }
    // printf("\n");

    DepLen = sqrt(Dep.x*Dep.x + Dep.y*Dep.y + Dep.z*Dep.z);

    if (Collided && (DistanceMin < DepLen || DistanceMin <= 1.1) )
    {
      switch(Out[CollisionIndice].CollisionAxe)
      {
        case 0: Dep.x=0.0; Event_Collision(Velocity.x / CycleTime); Velocity.x = 0.0; break;
        case 1: Dep.y=0.0; Event_Collision(Velocity.y / CycleTime); Velocity.y = 0.0; JumpDebounce = 0;break;
        case 2: Dep.z=0.0; Event_Collision(Velocity.z / CycleTime); Velocity.z = 0.0; break;
      }
    }
    else
    {
      ZVector3d NewLocation;

	  NewLocation = ViewDirection.origin() + Dep;

      //  SetPosition( NewLocation );
//
      ViewDirection.origin() = NewLocation;
	  Camera.orientation = ViewDirection;
	  Camera.orientation.translate_rel(  0.0,  (GlobalSettings.VoxelBlockSize/2),  0.0 );
//
      Deplacement = 0.0;
      Continue = false;
    }
  }

  // Dead Rolling and view

  if (IsDead)
  {
    //ViewDirection.roll += 0.5 * CycleTime * ( DeathChronometer / 10000.0 ); if (ViewDirection.roll > 360) ViewDirection.roll -= 360.0;
    //ViewDirection.pitch+= 1.2 * CycleTime * ( DeathChronometer / 10000.0 ); if (ViewDirection.pitch > 360) ViewDirection.pitch -= 360.0;

    ViewDirection.RotateRoll( + 0.3 * CycleTime * ( DeathChronometer / 10000.0 ) ); 
	//if (ViewDirection.roll > 360) ViewDirection.roll -= 360.0;
    ViewDirection.RotatePitch( + 0.5 * CycleTime * ( DeathChronometer / 10000.0 ) ); 
	//if (ViewDirection.pitch > 360) ViewDirection.pitch -= 360.0;
    DeathChronometer-= CycleTime;
    if (DeathChronometer> 4500.0 && DeathChronometer<5000.0)
    {
		DeathChronometer = 4500.0; 
		GameEnv->GameWindow_Advertising->Advertise("YOU ARE DEAD", ZGameWindow_Advertising::VISIBILITY_HIGH, 0, 4500.0, 0.0); 
	}
    if (DeathChronometer <= 0.0) 
	{
		Event_Death(); return;
	}
  }

}





// ********************************************************************************
//                          Old version of the plane
// ********************************************************************************

void ZActor_Player::DoPhysic_Plane_Old(double CycleTime)
{
  ZVector3d P[32];
  UShort Voxel[32];
  ZVoxelWorld * World;
  // bool   IsEmpty[32];
  ZVector3d Dep,Dep2;
  double DepLen;
  ULong i;

  // Colored vision off. May be reactivated further.

  Camera.ColoredVision.Activate = false;

  // Define Detection points

  P[0] = ViewDirection.origin() + ZVector3d(0,0,0);   // #

  // Get the Voxel Informations

  World = PhysicsEngine->World;
  for (i=0;i<24;i++)
  {
    Voxel[i]     = World->GetVoxelPlayerCoord(P[i].x,P[i].y,P[i].z);
    // IsEmpty[i]   = VoxelType[i]->Is_PlayerCanPassThrough;
  }


  // The gravity...

  // if      (CubeY > 10000.0 && CubeY < 15000.0) { Gravity = 5.0 - (( (CubeY-10000.0) * (CubeY-10000.0)) / 5000000.0); } //5000000.0;
  /* else if (CubeY <= 10000.0) { Gravity = 5.0; }
  else                       { Gravity = 0.0; }
  */

  // Velocity.y -= (PlayerDensity - LocationDensity) * Gravity * CycleTime;

  // Angle computing

  if (ViewDirection.roll() >0.0 && ViewDirection.roll() <=90.0) 
	  ViewDirection.RotateYaw(  ViewDirection.roll() / 1440.0 * CycleTime );
  if (ViewDirection.roll() <=360 && ViewDirection.roll() >= 270.0 ) 
	  ViewDirection.RotateYaw( - (360.0 - ViewDirection.roll()) / 1440.0 * CycleTime );


  // Engine power
  //ZPolar3d Direction = ViewDirection;
  ZVector3d CDir;
  //Direction.Len = 1.0;
  CDir = ViewDirection.z_axis() * -1;
  Velocity = CDir * 10 * 400.0;

  // Deplacement computation



  // The viscous friction loss...

  double FrictionCoef, PlaneSCX;
  ZVector3d Frottement;
  FrictionCoef = GameEnv->VoxelTypeManager.VoxelTable[Voxel[0]]->FrictionCoef;
  PlaneSCX = 1.0;
  Frottement = (Velocity * Velocity * FrictionCoef * PlaneSCX / 1000000000.0 * CycleTime) + 1.0;
  Velocity /= Frottement;




  Dep = Velocity * CycleTime / 1000.0;
  DepLen = sqrt(Dep.x*Dep.x + Dep.y*Dep.y + Dep.z*Dep.z);


  // Collision detection loop and displacement correction

  ZRayCast_in In;
  ZRayCast_out Out[32];
  double DistanceMin;
  Long CollisionIndice;
  Bool Collided;
  Bool Continue;
  Bool PEnable[32];


  In.Camera = 0;
  In.MaxCubeIterations = (ULong)ceil(DepLen / 256)+5; // 6;
  In.PlaneCubeDiff = In.MaxCubeIterations - 3;
  In.MaxDetectionDistance = 3000000.0;


  // ****
  for (i=0;i<24;i++) {PEnable[i] = true;}
  Continue = true;
  if ( (Dep.x == 0) && (Dep.y == 0) && (Dep.z == 0.0) ) { Continue = false; return; }

  Location_Old = ViewDirection.origin();
  while (Continue)
  {

    Collided = false;
    DistanceMin = 10000000.0;
    CollisionIndice = -1;
    for (i=0;i<1;i++)
    {

      if (PEnable[i]) // (PEnable[i])
      {
        bool ret;

        ret = World->RayCast_Vector(P[i],Dep , &In, &(Out[i]), false);         // Normal points.
        if (ret)
        {
          if (Out[i].CollisionDistance < DistanceMin ) { Collided = true; DistanceMin = Out[i].CollisionDistance; CollisionIndice = i; }
        }
      }
    }
    // printf("\n");

    DepLen = sqrt(Dep.x*Dep.x + Dep.y*Dep.y + Dep.z*Dep.z);

    if (Collided && (DistanceMin < DepLen || DistanceMin <= 1.1) )
    {
      switch(Out[CollisionIndice].CollisionAxe)
      {
        case 0: Dep.x=0.0; Event_Collision(Velocity.x); Velocity.x = 0.0; break;
        case 1: Dep.y=0.0; Event_Collision(Velocity.y); Velocity.y = 0.0; JumpDebounce = 0;break;
        case 2: Dep.z=0.0; Event_Collision(Velocity.z); Velocity.z = 0.0; break;
      }
    }
    else
    {
      ZVector3d NewLocation;

      NewLocation = ViewDirection.origin() + Dep;

        SetPosition( NewLocation );

        Deplacement = 0.0;
        Continue = false;

    }
  }
}























int ZVoxelRef::ForEachVoxel(  ZVoxelWorld * World, ZVoxelRef *v1, ZVoxelRef *v2, int (*f)(ZVoxelRef *v), bool not_zero )
{
	if( !v1 || !v2 )
		return not_zero;
	if( !v1->Sector || !v2->Sector )
		return not_zero;
	int v1x = v1->x + ( v1->Sector->Pos_x << ZVOXELBLOCSHIFT_X );
	int v1y = v1->y + ( v1->Sector->Pos_y << ZVOXELBLOCSHIFT_Y );
	int v1z = v1->z + ( v1->Sector->Pos_z << ZVOXELBLOCSHIFT_Z );
	int v2x = v2->x + ( v2->Sector->Pos_x << ZVOXELBLOCSHIFT_X );
	int v2y = v2->y + ( v2->Sector->Pos_y << ZVOXELBLOCSHIFT_Y );
	int v2z = v2->z + ( v2->Sector->Pos_z << ZVOXELBLOCSHIFT_Z );
	int del_x = v2x - v1x;
	int del_y = v2y - v1y;
	int del_z = v2z - v1z;
	int abs_x = del_x<0?-del_x:del_x;
	int abs_y = del_y<0?-del_y:del_y;
	int abs_z = del_z<0?-del_z:del_z;
	// cannot use iterate if either end is undefined.
	if( del_x )
	{
		if( del_y )
		{
			if( del_z )
			{
				if( abs_x > abs_y || ( abs_z > abs_y ) )
				{
					if( abs_z > abs_x )
					{
						// z is longest path
						int erry = -abs_z/2;
						int errx = -abs_z/2;
						int incy = del_y<0?-1:1;
						int incx = del_x<0?-1:1;
						int incz = del_z<0?-1:1;
						{
							int x = v1x;
							int y = v1y;
							for( int z = v1z + incz; z != v2z; z += incz )
							{
								errx += abs_x;
								if( errx > 0 )
								{
									errx -= abs_z;
									x += incx;
								}
								erry += abs_y;
								if( erry > 0 )
								{
									erry -= abs_z;
									y += incy;
								}
								{
									int val;
									ZVoxelRef v;
									if(  World->GetVoxelRef( v, x, y, z ) )
									{
										val = f( &v );
										if( (!not_zero && val ) || ( not_zero && !val ) )
											return val;
									}
								}
							}
						}
					}
					else
					{
						// x is longest.
						int erry = -abs_x/2;
						int errz = -abs_x/2;
						int incy = del_y<0?-1:1;
						int incx = del_x<0?-1:1;
						int incz = del_z<0?-1:1;
						{
							int y = v1y;
							int z = v1z;
							for( int x = v1x + incx; x != v2x; x += incx )
							{
								errz += abs_z;
								if( errz > 0 )
								{
									errz -= abs_x;
									z += incx;
								}
								erry += abs_y;
								if( erry > 0 )
								{
									erry -= abs_x;
									y += incy;
								}
								{
									int val;
									ZVoxelRef v;
									if( World->GetVoxelRef( v, x, y, z ) )
									{
										val = f( &v );
										if( (!not_zero && val ) || ( not_zero && !val ) )
											return val;
									}
								}
							}
						}
					}
				}
				else
				{
					// y is longest.
					int errx = -abs_y/2;
					int errz = -abs_y/2;
					int incy = del_y<0?-1:1;
					int incx = del_x<0?-1:1;
					int incz = del_z<0?-1:1;
					{
						int x = v1x;
						int z = v1x;
						for( int y = v1y + incy; y != v2y; y += incy )
						{
							errx += abs_x;
							if( errx > 0 )
							{
								errx -= abs_y;
								x += incx;
							}
							errz += abs_z;
							if( errz > 0 )
							{
								errz -= abs_y;
								z += incz;
							}
								{
									int val;
									ZVoxelRef v;
									if(  World->GetVoxelRef( v, x, y, z ) )
									{
										val = f( &v );
										if( (!not_zero && val ) || ( not_zero && !val ) )
											return val;
									}
								}
						}
					}
				}
			}
			else
			{
				// z is constant
				if( abs_x > abs_y )
				{
					// x is longest
					int erry = -abs_x/2;
					int incy = del_y<0?-1:1;
					int incx = del_x<0?-1:1;
					{
						int y = v1y;
						int z = v1z;
						for( int x = v1x + incx; x != v2x; x += incx )
						{
							erry += abs_y;
							if( erry > 0 )
							{
								erry -= abs_x;
								y += incy;
							}
								{
									int val;
									ZVoxelRef v;
									if(  World->GetVoxelRef( v, x, y, z ) )
									{
										val = f( &v );
										if( (!not_zero && val ) || ( not_zero && !val ) )
											return val;
									}
								}
						}
					}
				}
				else
				{
					// y is longest.
					int errx = -abs_y/2;
					int incy = del_y<0?-1:1;
					int incx = del_x<0?-1:1;
					// z is longest path
					{
						int x = v1x;
						int z = v1x;
						for( int y = v1y + incy; y != v2y; y += incy )
						{
							errx += abs_x;
							if( errx > 0 )
							{
								errx -= abs_y;
								x += incx;
							}
								{
									int val;
									ZVoxelRef v;
									if(  World->GetVoxelRef( v, x, y, z ) )
									{
										val = f( &v );
										if( (!not_zero && val ) || ( not_zero && !val ) )
											return val;
									}
								}
						}
					}
				}
			}
		}
		else
		{
			if( del_z )
			{
				if( abs_x > abs_z )
				{
					// x is longest.
					int errz = -abs_x/2;
					int incx = del_x<0?-1:1;
					int incz = del_z<0?-1:1;
					{
						int y = v1y;
						int z = v1z;
						for( int x = v1x + incx; x != v2x; x += incx )
						{
							errz += abs_z;
							if( errz > 0 )
							{
								errz -= abs_x;
								z += incx;
							}
								{
									int val;
									ZVoxelRef v;
									if(  World->GetVoxelRef( v, x, y, z ) )
									{
										val = f( &v );
										if( (!not_zero && val ) || ( not_zero && !val ) )
											return val;
									}
								}
						}
					}
				}
				else
				{
					// z is longest path
					int errx = -abs_z/2;
					int incx = del_x<0?-1:1;
					int incz = del_z<0?-1:1;
					{
						int x = v1x;
						int y = v1y;
						for( int z = v1z + incz; z != v2z; z += incz )
						{
							errx += abs_x;
							if( errx > 0 )
							{
								errx -= abs_z;
								x += incx;
							}
								{
									int val;
									ZVoxelRef v;
									if(  World->GetVoxelRef( v, x, y, z ) )
									{
										val = f( &v );
										if( (!not_zero && val ) || ( not_zero && !val ) )
											return val;
									}
								}
						}
					}
				}
			}
			else
			{
				// x is only changing.
				int incx = del_x<0?-1:1;
				for( int x = v1x + incx; x != v2x; x += incx )
				{
					int val;
					ZVoxelRef v;
					if(  World->GetVoxelRef( v, x, v1y, v1z ) )
					{
						val = f( &v );
										if( (!not_zero && val ) || ( not_zero && !val ) )
							return val;
					}
				}
			}
		}
	}
	else
	{
		if( del_y )
		{
			if( del_z )
			{
				if( abs_y > abs_z )
				{
					// y is longest.
					int errz = -abs_y/2;
					int incy = del_y<0?-1:1;
					int incz = del_z<0?-1:1;
					{
						int x = v1x;
						int z = v1x;
						for( int y = v1y + incy; y != v2y; y += incy )
						{
							errz += abs_z;
							if( errz > 0 )
							{
								errz -= abs_y;
								z += incz;
							}
								{
									int val;
									ZVoxelRef v;
									if(  World->GetVoxelRef( v, x, y, z ) )
									{
										val = f( &v );
										if( (!not_zero && val ) || ( not_zero && !val ) )
											return val;
									}
								}
						}
					}
				}
				else
				{
					// z is longest path
					int erry = -abs_z/2;
					int incy = del_y<0?-1:1;
					int incz = del_z<0?-1:1;
					{
						int x = v1x;
						int y = v1y;
						for( int z = v1z + incz; z != v2z; z += incz )
						{
							erry += abs_y;
							if( erry > 0 )
							{
								erry -= abs_z;
								y += incy;
							}
								{
									int val;
									ZVoxelRef v;
									if(  World->GetVoxelRef( v, x, y, z ) )
									{
										val = f( &v );
										if( (!not_zero && val ) || ( not_zero && !val ) )
											return val;
									}
								}
						}
					}
				}
			}
			else
			{
				// no del_x, no del_z
				// y is only changing.
				int incy = del_y<0?-1:1;
				for( int y = v1y + incy; y != v2y; y += incy )
				{
					int val;
					ZVoxelRef v;
					if( World->GetVoxelRef( v, v1x, y, v1z ) )
					{
						val = f( &v );
										if( (!not_zero && val ) || ( not_zero && !val ) )
							return val;
					}
				}
			}
		}
		else
		{
			// no del_x, no del_y...
			if( del_z )
			{
				if( del_z > 0 )
					for( int z = v1z + 1; z < v2z; z++ )
					{
						int val;
						ZVoxelRef v;
						if(  World->GetVoxelRef( v, v1x, v1y, z ) )
						{
						val = f( &v );
										if( (!not_zero && val ) || ( not_zero && !val ) )
							return val;
						}
					}
				else
					for( int z = v2z + 1; z < v1z; z++ )
					{
						int val;
						ZVoxelRef v;
						if(  World->GetVoxelRef( v, v1x, v1y, z ) )
						{
							val = f( &v );
										if( (!not_zero && val ) || ( not_zero && !val ) )
								return val;
						}
					}

			}
			else
			{
				// no delta diff, nothing to do.
			}
		}
	}
	return not_zero;
}

int TestIsNotEmpty( ZVoxelRef *v )
{
	return v->VoxelTypeManager->VoxelTable[v->VoxelType]->Is_PlayerCanPassThrough;
}


void ZActor_Player::DoPhysic_GroundPlayer(double CycleTime)
{
  ZVector3d Dep,Dep2;
  ZVector3d Tmp;
  ZVector3d P[32];
  Bool PEnable[32];
  bool PInvert[32];
  ZVoxelRef RealVoxel[32];
  UShort Voxel[32];
  bool   IsEmpty[32];
  ZVoxelType * VoxelType[32];
  ULong i;
  ZVoxelWorld * World;
  double DepLen;


  // Voxel Help System

  if (PointedVoxel.Collided)
  {
    UShort VoxelType;

    LastHelpTime += CycleTime;

    VoxelType = GameEnv->World->GetVoxel(PointedVoxel.PointedVoxel.x, PointedVoxel.PointedVoxel.y, PointedVoxel.PointedVoxel.z);
    if (GameEnv->VoxelTypeManager.VoxelTable[VoxelType]->Is_HasHelpingMessage)
    {
      if (   PointedVoxel.PointedVoxel.x != LastHelpVoxel.x
          || PointedVoxel.PointedVoxel.y != LastHelpVoxel.y
          || PointedVoxel.PointedVoxel.z != LastHelpVoxel.z
          || LastHelpTime > 3005.0                         )
      {
        LastHelpTime = 0.0;
        LastHelpVoxel.x = PointedVoxel.PointedVoxel.x;
        LastHelpVoxel.y = PointedVoxel.PointedVoxel.y;
        LastHelpVoxel.z = PointedVoxel.PointedVoxel.z;
        GameEnv->GameWindow_Advertising->Advertise(GameEnv->VoxelTypeManager.VoxelTable[VoxelType]->HelpingMessage.String, ZGameWindow_Advertising::VISIBILITY_VERYHARDTOREAD, 0, 3000.0,100.0 );
      }
    }

  }


  // Colored vision off. May be reactivated further.

  Camera.ColoredVision.Activate = false;

  // Define Detection points

  // front p[0], p[1], p[0],p[12], p[1], p[13], p[12], p[13] 
  // back p[0], p[1], p[0],p[12], p[1], p[13], p[12], p[13] 

  P[0] = ViewDirection.origin() + ZVector3d(-75.0,+0.0,+75.0); // floor plane, left, forward
  P[1] = ViewDirection.origin() + ZVector3d(+75.0,+0.0,+75.0); // floor plane, right, forward
  P[2] = ViewDirection.origin() + ZVector3d(+75.0,+0.0,-75.0); // floor plane, left, back
  P[3] = ViewDirection.origin() + ZVector3d(-75.0,+0.0,-75.0); // floor plane, right, back

  P[4] = ViewDirection.origin() + ZVector3d(-75.0,+128.0,+75.0);
  P[5] = ViewDirection.origin() + ZVector3d(+75.0,+128.0,+75.0);
  P[6] = ViewDirection.origin() + ZVector3d(+75.0,+128.0,-75.0);
  P[7] = ViewDirection.origin() + ZVector3d(-75.0,+128.0,-75.0);

  P[8] = ViewDirection.origin() + ZVector3d(-75.0,+384.0,+75.0);
  P[9] = ViewDirection.origin() + ZVector3d(+75.0,+384.0,+75.0);
  P[10] = ViewDirection.origin() + ZVector3d(+75.0,+384.0,-75.0);
  P[11] = ViewDirection.origin() + ZVector3d(-75.0,+384.0,-75.0);

  P[12] = ViewDirection.origin() + ZVector3d(-75.0,+475.0,+75.0);
  P[13] = ViewDirection.origin() + ZVector3d(+75.0,+475.0,+75.0);
  P[14] = ViewDirection.origin() + ZVector3d(+75.0,+475.0,-75.0);
  P[15] = ViewDirection.origin() + ZVector3d(-75.0,+475.0,-75.0);

  P[16] = ViewDirection.origin() + ZVector3d(-70.0,-5.0,-70.0); // # Detection points below the player
  P[17] = ViewDirection.origin() + ZVector3d(+70.0,-5.0,-70.0); // # Used for Anti-Fall.
  P[18] = ViewDirection.origin() + ZVector3d(+70.0,-5.0,+70.0); // #right;forward
  P[19] = ViewDirection.origin() + ZVector3d(-70.0,-5.0,+70.0); // #left;forward

  P[20] = ViewDirection.origin() + ZVector3d(0,-5,0);  // # Detection point are only for voxel
  P[21] = ViewDirection.origin() + ZVector3d(0,0,0);   // #
  P[22] = ViewDirection.origin() + ZVector3d(0,128,0); // #
  P[23] = ViewDirection.origin() + ZVector3d(0,384,0); // #

  // Get the Voxel Informations

  World = PhysicsEngine->World;
  for (i=0;i<24;i++)
  {
    if( World->GetVoxelRefPlayerCoord(RealVoxel[i], P[i].x,P[i].y,P[i].z) )
		Voxel[i] = RealVoxel[i].VoxelType;
	else
	{
		return;
		Voxel[i] = 0;
	}
    VoxelType[i] = GameEnv->VoxelTypeManager.VoxelTable[Voxel[i]];
    IsEmpty[i]   = VoxelType[i]->Is_PlayerCanPassThrough;
  }

  // Detect player is on ground
  int space_empty;
  space_empty = ZVoxelRef::ForEachVoxel( World, &RealVoxel[16], &RealVoxel[17], TestIsNotEmpty, true );
	if( space_empty )
		space_empty = ZVoxelRef::ForEachVoxel( World, &RealVoxel[18], &RealVoxel[19], TestIsNotEmpty, true );
  if ( !space_empty ) IsOnGround = true;
  else                                                            IsOnGround = false;
  //if ( IsEmpty[16] && IsEmpty[17] && IsEmpty[18] && IsEmpty[19] ) IsOnGround = false;
  //else                                                            IsOnGround = true;

  // Detect if player is in liquid and compute the mean density.


  IsFootInLiquid = VoxelType[22]->Is_Liquid;
  IsHeadInLiquid = VoxelType[23]->Is_Liquid;
  IsInLiquid = IsFootInLiquid && IsHeadInLiquid;
  LocationDensity = (VoxelType[22]->LiquidDensity + VoxelType[23]->LiquidDensity) / 2.0;
  IsWalking = (Deplacement.x != 0.0) || (Deplacement.z != 0.0 );

  if (Voxel[23]==85) { Camera.ColoredVision.Activate = true; Camera.ColoredVision.Red= 0.0f; Camera.ColoredVision.Green = 0.0f; Camera.ColoredVision.Blue = 1.0f; Camera.ColoredVision.Opacity  = 0.5f; }
  if (Voxel[23]==86) { Camera.ColoredVision.Activate = true; Camera.ColoredVision.Red= 0.0f; Camera.ColoredVision.Green = 1.0f; Camera.ColoredVision.Blue = 0.0f; Camera.ColoredVision.Opacity  = 0.5f; }
  if (Voxel[23]==89) { Camera.ColoredVision.Activate = true; Camera.ColoredVision.Red= 1.0f; Camera.ColoredVision.Green = 1.0f; Camera.ColoredVision.Blue = 0.0f; Camera.ColoredVision.Opacity  = 0.9f; }


  // Voxel collision and player damage.


  bool harming = 0;
  //UShort HarmingVoxel;
  double MaxHarming = 0;
  double VoxelHarming;
  for(i=0;i<16;i++) if (GameEnv->VoxelTypeManager.VoxelTable[Voxel[i]]->Is_Harming)
  {
    harming |= true;
    //HarmingVoxel = Voxel[i];
    VoxelHarming = GameEnv->VoxelTypeManager.VoxelTable[Voxel[i]]->HarmingLifePointsPerSecond;
    if (VoxelHarming > MaxHarming) MaxHarming = VoxelHarming;
  }
  if ((harming))
  {
    Camera.ColoredVision.Activate = true;
    Camera.ColoredVision.Red      = 1.0f;
    Camera.ColoredVision.Green    = 0.0f;
    Camera.ColoredVision.Blue     = 0.0f;
    Camera.ColoredVision.Opacity  = 0.5f;
    // printf("LifePoints: %lf \n",LifePoints);
    LifePoints -= MaxHarming / 1000.0 * CycleTime;
    if (LifePoints < 0.0) LifePoints = 0.0;
  }

  if (LifePoints <= 0.0 && !IsDead)
  {
    this->IsDead = true;
    GameEnv->GameWindow_Advertising->Advertise("YOU ARE DEAD", 0, 0, 10000.0, 10000.0);
    DeathChronometer = 10000.0;
  }

  if (IsDead)
  {
    Camera.ColoredVision.Activate = true; Camera.ColoredVision.Red= 0.0f; Camera.ColoredVision.Green = 0.0f; Camera.ColoredVision.Blue = 0.0f; Camera.ColoredVision.Opacity  = 0.8f;
    DeathChronometer -= CycleTime;
    if (DeathChronometer <= 0.0)
    {
      Event_Death();

    }

    return;
  }

  // Vision change for the head in voxel


  if (Voxel[23] == 52)
  {
    Camera.ColoredVision.Activate = true;
    Camera.ColoredVision.Red     = 1.0f;
    Camera.ColoredVision.Green   = 0.0f;
    Camera.ColoredVision.Blue    = 0.0f;
    Camera.ColoredVision.Opacity = 0.95f;
  }

  // Physical deplacement computation

  if (1)
  {
    ZVector3d Frottement, WalkSpeed, VelocityIncrease, MaxVelocityIncrease, GripFactor;
    double FrictionCoef;

    // Limit the frametime to avoid accidental stuttering nasty effects.

    if (CycleTime > 160.0) CycleTime = 160.0; // Limit frame time

    // Jumping from certain blocks won't permit you any motion control in space...

    bool ForceLostControll = false;;
    if (!GameEnv->VoxelTypeManager.VoxelTable[Voxel[20]]->Is_PlayerCanPassThrough)
    {
      KeepControlOnJumping = GameEnv->VoxelTypeManager.VoxelTable[Voxel[20]]->Is_KeepControlOnJumping;
    }
    else
    {
      if ( !KeepControlOnJumping ) {Deplacement = 0.0; ForceLostControll = true; } // Cancel space control if you jump from these blocks...
    }

    // The gravity...
    double Gravity, CubeY;
    CubeY = ViewDirection.origin().y / GlobalSettings.VoxelBlockSize;
    if      (CubeY > 10000.0 && CubeY < 15000.0) { Gravity = 5.0 - (( (CubeY-10000.0) * (CubeY-10000.0)) / 5000000.0); } //5000000.0;
    else if (CubeY <= 10000.0) { Gravity = 5.0; }
    else                       { Gravity = 0.0; }
  if (!this->Flag_ActivateAntiFall)
	if( !IsOnGround )
		Velocity.y -= (PlayerDensity - LocationDensity) * Gravity * CycleTime;

    //printf("Gravity %lf y: %lf\n",Gravity, CubeY);

    // Player legs action

    GripFactor.x = GameEnv->VoxelTypeManager.VoxelTable[Voxel[20]]->Grip_Horizontal;
    GripFactor.z = GripFactor.x;
    GripFactor.y = GameEnv->VoxelTypeManager.VoxelTable[Voxel[22]]->Grip_Vertical;
    WalkSpeed = Deplacement * 50.0;
    if (GameEnv->VoxelTypeManager.VoxelTable[Voxel[20]]->Is_SpaceGripType || ForceLostControll)
    {
      VelocityIncrease = WalkSpeed * (CycleTime / 16.0)* GripFactor;
      if (WalkSpeed.x > 0.0) {if (Velocity.x > WalkSpeed.x) if (VelocityIncrease.x>0.0) VelocityIncrease.x = 0.0; }
      if (WalkSpeed.x < 0.0) {if (Velocity.x < WalkSpeed.x) if (VelocityIncrease.x<0.0) VelocityIncrease.x = 0.0; }
      if (WalkSpeed.z > 0.0) {if (Velocity.z > WalkSpeed.z) if (VelocityIncrease.z>0.0) VelocityIncrease.z = 0.0; }
      if (WalkSpeed.z < 0.0) {if (Velocity.z < WalkSpeed.z) if (VelocityIncrease.z<0.0) VelocityIncrease.z = 0.0; }
    }
    else
    {
      MaxVelocityIncrease = (WalkSpeed - Velocity) * GripFactor;
      VelocityIncrease = MaxVelocityIncrease * (CycleTime / 16.0);
      VelocityIncrease.y = 0.0;
      if (fabs(VelocityIncrease.x) > fabs(MaxVelocityIncrease.x) ) VelocityIncrease.x = MaxVelocityIncrease.x;
      if (fabs(VelocityIncrease.y) > fabs(MaxVelocityIncrease.y) ) VelocityIncrease.y = MaxVelocityIncrease.y;
      if (fabs(VelocityIncrease.z) > fabs(MaxVelocityIncrease.z) ) VelocityIncrease.z = MaxVelocityIncrease.z;
    }
    Velocity += VelocityIncrease;

    // The viscous friction loss...

    FrictionCoef = GameEnv->VoxelTypeManager.VoxelTable[Voxel[20]]->FrictionCoef;
    FrictionCoef+= GameEnv->VoxelTypeManager.VoxelTable[Voxel[22]]->FrictionCoef;
    FrictionCoef+= GameEnv->VoxelTypeManager.VoxelTable[Voxel[23]]->FrictionCoef;
    Frottement = (Velocity * Velocity * FrictionCoef * PlayerSurface / 1000000000.0 * CycleTime) + 1.0;
    Velocity /= Frottement;

    // VelocityBooster.

    if (Voxel[20] == 16) Velocity *= 1.0 + CycleTime / 40.0;

    // Velocity to deplacement

    Dep = Velocity * CycleTime / 1000.0;
    DepLen = sqrt(Dep.x*Dep.x + Dep.y*Dep.y + Dep.z*Dep.z);
    // printf("Velocity %lf %lf %lf Increase: %lf %lf %lf CycleTime :%lf\n",Velocity.x, Velocity.y, Velocity.z, VelocityIncrease.x, VelocityIncrease.y, VelocityIncrease.z, CycleTime );

  }



  // Disable all control points. Mark anti fall points with invert flag.

  for (i=0;i<24;i++) {PEnable[i] = false; PInvert[i] = false; }
  for (i=16;i<20;i++){PInvert[i]=true;}

  // Vector direction enable some control points.

  if (Dep.x < 0.0 ) { PEnable[4]  = true; PEnable[7] = true; PEnable[8]  = true; PEnable[11] = true; }
  if (Dep.x > 0.0 ) { PEnable[5]  = true; PEnable[6] = true; PEnable[9]  = true; PEnable[10] = true; }
  if (Dep.z > 0.0 ) { PEnable[4]  = true; PEnable[5] = true; PEnable[8]  = true; PEnable[9]  = true; }
  if (Dep.z < 0.0 ) { PEnable[6]  = true; PEnable[7] = true; PEnable[10] = true; PEnable[11] = true; }
  if (Dep.y > 0.0 ) { PEnable[12] = true; PEnable[13]= true; PEnable[14] = true; PEnable[15] = true; }
  if (Dep.y < 0.0 ) { PEnable[0]  = true; PEnable[1] = true; PEnable[2]  = true; PEnable[3]  = true; }

  PEnable[0]  = true; PEnable[1] = true; PEnable[2]  = true; PEnable[3]  = true;

  // Anti Fall test point activation

  if (this->Flag_ActivateAntiFall)
  {
    if (Dep.x >0)
    {
       if   (Dep.z>0) {PEnable[16] = true;}
       else           {PEnable[19] = true;}
    }
    else
    {
      if   (Dep.z>0) {PEnable[17] = true;}
      else           {PEnable[18] = true;}
    }

    if ( Dep.y>0.0)                                                 {PEnable[16]=PEnable[17]=PEnable[18]=PEnable[19] = false; }
    if ( IsEmpty[19] && IsEmpty[18] && IsEmpty[17] && IsEmpty[16] ) {PEnable[16]=PEnable[17]=PEnable[18]=PEnable[19] = false; }
    if ( !IsEmpty[20] )                                             {PEnable[16]=PEnable[17]=PEnable[18]=PEnable[19] = false; }

    if ( IsEmpty[19] && IsEmpty[18] && IsEmpty[17] ) PEnable[16] = true;
    if ( IsEmpty[18] && IsEmpty[17] && IsEmpty[16] ) PEnable[19] = true;
    if ( IsEmpty[17] && IsEmpty[16] && IsEmpty[19] ) PEnable[18] = true;
    if ( IsEmpty[16] && IsEmpty[19] && IsEmpty[18] ) PEnable[17] = true;
  }

  // Collision detection loop and displacement correction

  ZRayCast_in In;
  ZRayCast_out Out[32];


  In.Camera = 0;
  In.MaxCubeIterations = (ULong)ceil(DepLen / 256)+5; // 6;
  In.PlaneCubeDiff = In.MaxCubeIterations - 3;
  In.MaxDetectionDistance = 3000000.0;
  double DistanceMin;
  Long CollisionIndice;
  Bool Collided, IsCollided_h;
  Bool Continue;

  // ****

  Continue = true;
  double roll = 0;
  	if( ActorMode == 0 )  // make sure we're standing up.
		ViewDirection.RotateRoll( roll = -ViewDirection.roll() );
  if ( ( roll == 0 ) && (Dep.x == 0) && (Dep.y == 0) && (Dep.z == 0.0) ) { Continue = false; 

	return;
  }

  // printf("Loc: %lf %lf %lf\n",ViewDirection.origin().x,ViewDirection.origin().y,ViewDirection.origin().z);

  Location_Old = ViewDirection.origin();
  IsCollided_h = false;
  while (Continue)
  {

    Collided = false;
    DistanceMin = 10000000.0;
    CollisionIndice = -1;
    for (i=0;i<20;i++)
    {

      if (PEnable[i]) // (PEnable[i])
      {
        bool ret;
	   if (this->Flag_ActivateAntiFall)
		   ret = 0;
	   else
	   {
        if (PInvert[i]) ret = World->RayCast_Vector_special(P[i],Dep/DepLen , &In, &(Out[i]), PInvert[i]); // If anti fall points, use special routine and invert empty/full detection.
		else            ret = World->RayCast_Vector(P[i],Dep/DepLen , &In, &(Out[i]), PInvert[i]);         // Normal points.

        if (ret)
        {
          if (Out[i].CollisionDistance < DistanceMin ) { Collided = true; DistanceMin = Out[i].CollisionDistance; CollisionIndice = i; }
        }
	   }
      }
    }
    // printf("\n");

    DepLen = sqrt(Dep.x*Dep.x + Dep.y*Dep.y + Dep.z*Dep.z);

    if (Collided && (DistanceMin < DepLen || DistanceMin <= 1.1) )
    {
      // printf("Collided(Loc:%lf %lf %lf dep: %lf %lf %lf Point : %lf %lf %lf Ind:%ld ))\n",ViewDirection.origin().x,ViewDirection.origin().y,ViewDirection.origin().z, Dep.x,Dep.y,Dep.z,Out[CollisionIndice].CollisionPoint.x,Out[CollisionIndice].CollisionPoint.y, Out[CollisionIndice].CollisionPoint.z, CollisionIndice);
      // World->RayCast_Vector(P[CollisionIndice],Dep , &In, &(Out[CollisionIndice]));
      // Dep = Dep - (P[CollisionIndice] - Out[CollisionIndice].CollisionPoint);
      // SetPosition( Out[CollisionIndice].CollisionPoint );
      // Dep = 0.0;
      switch(Out[CollisionIndice].CollisionAxe)
      {
        case 0: Dep.x=0.0; Event_Collision(Velocity.x); Velocity.x = 0.0; IsCollided_h = true; break;
        case 1: Dep.y=0.0; Event_Collision(Velocity.y); Velocity.y = 0.0; JumpDebounce = 0;break;
        case 2: Dep.z=0.0; Event_Collision(Velocity.z); Velocity.z = 0.0; IsCollided_h = true; break;
      }
      //Deplacement = 0.0;
      //return;
    }
    else
    {
      ZVector3d NewLocation;

      NewLocation = ViewDirection.origin() + Dep;


        SetPosition( NewLocation );
        Deplacement = 0.0;
        Continue = false;

    }
  }

  // Son du déplacement.

  #if COMPILEOPTION_FNX_SOUNDS_1 == 1

  bool WalkSoundOn;

  WalkSoundOn = IsWalking && !IsInLiquid && IsOnGround && !IsCollided_h && !Flag_ActivateAntiFall;
  if ( WalkSoundOn ) {if (WalkSoundHandle == 0) WalkSoundHandle = GameEnv->Sound->Start_PlaySound(4, true, true, 1.0, 0 ); }
  else               {if (WalkSoundHandle != 0) { GameEnv->Sound->Stop_PlaySound(WalkSoundHandle); WalkSoundHandle = 0; }}

  #endif

}

void ZActor_Player::DoPhysic_SupermanPlayer(double CycleTime)
{
  ZVector3d Dep,Dep2;
  ZVector3d Tmp;
  ZVector3d P[32];
  Bool PEnable[32];
  bool PInvert[32];
  UShort Voxel[32];
  bool   IsEmpty[32];
  ZVoxelType * VoxelType[32];
  ULong i;
  ZVoxelWorld * World;
  double DepLen;


  // Voxel Help System

  if (PointedVoxel.Collided)
  {
    UShort VoxelType;

    LastHelpTime += CycleTime;

    VoxelType = GameEnv->World->GetVoxel(PointedVoxel.PointedVoxel.x, PointedVoxel.PointedVoxel.y, PointedVoxel.PointedVoxel.z);
    if (GameEnv->VoxelTypeManager.VoxelTable[VoxelType]->Is_HasHelpingMessage)
    {
      if (   PointedVoxel.PointedVoxel.x != LastHelpVoxel.x
          || PointedVoxel.PointedVoxel.y != LastHelpVoxel.y
          || PointedVoxel.PointedVoxel.z != LastHelpVoxel.z
          || LastHelpTime > 3005.0                         )
      {
        LastHelpTime = 0.0;
        LastHelpVoxel.x = PointedVoxel.PointedVoxel.x;
        LastHelpVoxel.y = PointedVoxel.PointedVoxel.y;
        LastHelpVoxel.z = PointedVoxel.PointedVoxel.z;
        GameEnv->GameWindow_Advertising->Advertise(GameEnv->VoxelTypeManager.VoxelTable[VoxelType]->HelpingMessage.String, ZGameWindow_Advertising::VISIBILITY_VERYHARDTOREAD, 0, 3000.0,100.0 );
      }
    }

  }


  // Colored vision off. May be reactivated further.

  Camera.ColoredVision.Activate = false;

  // Define Detection points

  P[0] = ViewDirection.origin() + ZVector3d(-75.0,+0.0,+75.0);
  P[1] = ViewDirection.origin() + ZVector3d(+75.0,+0.0,+75.0);
  P[2] = ViewDirection.origin() + ZVector3d(+75.0,+0.0,-75.0);
  P[3] = ViewDirection.origin() + ZVector3d(-75.0,+0.0,-75.0);

  P[4] = ViewDirection.origin() + ZVector3d(-75.0,+(128.0),+75.0);
  P[5] = ViewDirection.origin() + ZVector3d(+75.0,+(128.0),+75.0);
  P[6] = ViewDirection.origin() + ZVector3d(+75.0,+(128.0),-75.0);
  P[7] = ViewDirection.origin() + ZVector3d(-75.0,+(128.0),-75.0);

  P[8] = ViewDirection.origin() + ZVector3d(-75.0,+384.0,+75.0);
  P[9] = ViewDirection.origin() + ZVector3d(+75.0,+384.0,+75.0);
  P[10] = ViewDirection.origin() + ZVector3d(+75.0,+384.0,-75.0);
  P[11] = ViewDirection.origin() + ZVector3d(-75.0,+384.0,-75.0);

  P[12] = ViewDirection.origin() + ZVector3d(-75.0,+500.0,+75.0);
  P[13] = ViewDirection.origin() + ZVector3d(+75.0,+500.0,+75.0);
  P[14] = ViewDirection.origin() + ZVector3d(+75.0,+500.0,-75.0);
  P[15] = ViewDirection.origin() + ZVector3d(-75.0,+500.0,-75.0);

  P[16] = ViewDirection.origin() + ZVector3d(-70.0,-5.0,-70.0); // # Detection points behind the player
  P[17] = ViewDirection.origin() + ZVector3d(+70.0,-5.0,-70.0); // # Used for Anti-Fall.
  P[18] = ViewDirection.origin() + ZVector3d(+70.0,-5.0,+70.0); // #
  P[19] = ViewDirection.origin() + ZVector3d(-70.0,-5.0,+70.0); // #

  P[20] = ViewDirection.origin() + ZVector3d(0,-5,0);  // # Detection point are only for voxel
  P[21] = ViewDirection.origin() + ZVector3d(0,0,0);   // #
  P[22] = ViewDirection.origin() + ZVector3d(0,128,0); // #
  P[23] = ViewDirection.origin() + ZVector3d(0,384,0); // #

  // Get the Voxel Informations

  World = PhysicsEngine->World;
  for (i=0;i<24;i++)
  {
    Voxel[i]     = World->GetVoxelPlayerCoord(P[i].x,P[i].y,P[i].z);
    VoxelType[i] = GameEnv->VoxelTypeManager.VoxelTable[Voxel[i]];
    IsEmpty[i]   = VoxelType[i]->Is_PlayerCanPassThrough;
  }

  // Detect player is on ground

  if ( IsEmpty[16] && IsEmpty[17] && IsEmpty[18] && IsEmpty[19] ) IsOnGround = false;
  else                                                            IsOnGround = true;

  // Detect if player is in liquid and compute the mean density.


  IsFootInLiquid = VoxelType[22]->Is_Liquid;
  IsHeadInLiquid = VoxelType[23]->Is_Liquid;
  IsInLiquid = IsFootInLiquid && IsHeadInLiquid;
  LocationDensity = (VoxelType[22]->LiquidDensity + VoxelType[23]->LiquidDensity) / 2.0;

  if (Voxel[23]==85) { Camera.ColoredVision.Activate = true; Camera.ColoredVision.Red= 0.0f; Camera.ColoredVision.Green = 0.0f; Camera.ColoredVision.Blue = 1.0f; Camera.ColoredVision.Opacity  = 0.5f; }
  if (Voxel[23]==86) { Camera.ColoredVision.Activate = true; Camera.ColoredVision.Red= 0.0f; Camera.ColoredVision.Green = 1.0f; Camera.ColoredVision.Blue = 0.0f; Camera.ColoredVision.Opacity  = 0.5f; }
  if (Voxel[23]==89) { Camera.ColoredVision.Activate = true; Camera.ColoredVision.Red= 1.0f; Camera.ColoredVision.Green = 1.0f; Camera.ColoredVision.Blue = 0.0f; Camera.ColoredVision.Opacity  = 0.9f; }

  // Voxel collision and player damage.


  bool harming = 0;
  //UShort HarmingVoxel;
  double MaxHarming = 0;
  double VoxelHarming;
  for(i=0;i<16;i++) if (GameEnv->VoxelTypeManager.VoxelTable[Voxel[i]]->Is_Harming)
  {
    harming |= true;
    //HarmingVoxel = Voxel[i];
    VoxelHarming = GameEnv->VoxelTypeManager.VoxelTable[Voxel[i]]->HarmingLifePointsPerSecond;
    if (VoxelHarming > MaxHarming) MaxHarming = VoxelHarming;
  }
  if ((harming))
  {
    Camera.ColoredVision.Activate = true;
    Camera.ColoredVision.Red      = 1.0f;
    Camera.ColoredVision.Green    = 0.0f;
    Camera.ColoredVision.Blue     = 0.0f;
    Camera.ColoredVision.Opacity  = 0.5f;
    // printf("LifePoints: %lf \n",LifePoints);
    LifePoints -= MaxHarming / 1000.0 * CycleTime;
    if (LifePoints < 0.0) LifePoints = 0.0;
  }

  if (LifePoints <= 0.0 && !IsDead)
  {
    this->IsDead = true;
    GameEnv->GameWindow_Advertising->Advertise("YOU ARE DEAD", 0, 0, 10000.0, 10000.0);
    DeathChronometer = 10000.0;
  }

  if (IsDead)
  {
    Camera.ColoredVision.Activate = true; Camera.ColoredVision.Red= 0.0f; Camera.ColoredVision.Green = 0.0f; Camera.ColoredVision.Blue = 0.0f; Camera.ColoredVision.Opacity  = 0.8f;
    DeathChronometer -= CycleTime;
    if (DeathChronometer <= 0.0)
    {
      Event_Death();

    }
    return;
  }

  // Vision change for the head in voxel


  if (Voxel[23] == 52)
  {
    Camera.ColoredVision.Activate = true;
    Camera.ColoredVision.Red     = 1.0f;
    Camera.ColoredVision.Green   = 0.0f;
    Camera.ColoredVision.Blue    = 0.0f;
    Camera.ColoredVision.Opacity = 0.95f;
  }

  // Physical deplacement computation


  if (1)
  {
    ZVector3d Frottement, WalkSpeed, VelocityIncrease, MaxVelocityIncrease, GripFactor;
    double FrictionCoef;

    // Limit the frametime to avoid accidental stuttering nasty effects.

    if (CycleTime > 160.0) CycleTime = 160.0; // Limit frame time

    // Jumping from certain blocks won't permit you any motion control in space...

    if (!GameEnv->VoxelTypeManager.VoxelTable[Voxel[20]]->Is_PlayerCanPassThrough)
    {
      KeepControlOnJumping = GameEnv->VoxelTypeManager.VoxelTable[Voxel[20]]->Is_KeepControlOnJumping;
    }
    else
    {
      if ( !KeepControlOnJumping ) {Deplacement = 0.0; } // Cancel space control if you jump from these blocks...
    }

    // No gravity for superman...

    // Disabled : Velocity.y -= (PlayerDensity - LocationDensity) * Gravity * CycleTime;

    //printf("Gravity %lf y: %lf\n",Gravity, CubeY);

    // Player legs action

    GripFactor.x = 1.0;
    GripFactor.z = 1.0;
    GripFactor.y = 1.0;
    WalkSpeed = Deplacement * 50.0;

    MaxVelocityIncrease = (WalkSpeed - Velocity) * GripFactor;
    VelocityIncrease = MaxVelocityIncrease * (CycleTime / 16.0);

    if (fabs(VelocityIncrease.x) > fabs(MaxVelocityIncrease.x) ) VelocityIncrease.x = MaxVelocityIncrease.x;
    if (fabs(VelocityIncrease.y) > fabs(MaxVelocityIncrease.y) ) VelocityIncrease.y = MaxVelocityIncrease.y;
    if (fabs(VelocityIncrease.z) > fabs(MaxVelocityIncrease.z) ) VelocityIncrease.z = MaxVelocityIncrease.z;
    Velocity += VelocityIncrease;

    // The viscous friction loss...

    FrictionCoef = GameEnv->VoxelTypeManager.VoxelTable[Voxel[20]]->FrictionCoef;
    FrictionCoef+= GameEnv->VoxelTypeManager.VoxelTable[Voxel[22]]->FrictionCoef;
    FrictionCoef+= GameEnv->VoxelTypeManager.VoxelTable[Voxel[23]]->FrictionCoef;
    Frottement = (Velocity * Velocity * FrictionCoef * PlayerSurface / 1000000000.0 * CycleTime) + 1.0;
    Velocity /= Frottement;

    // VelocityBooster.

    if (Voxel[20] == 16) Velocity *= 1.0 + CycleTime / 40.0;

    // Velocity to deplacement

    Dep = Velocity * CycleTime / 1000.0;
    DepLen = sqrt(Dep.x*Dep.x + Dep.y*Dep.y + Dep.z*Dep.z);
    // printf("Velocity %lf %lf %lf Increase: %lf %lf %lf CycleTime :%lf\n",Velocity.x, Velocity.y, Velocity.z, VelocityIncrease.x, VelocityIncrease.y, VelocityIncrease.z, CycleTime );

  }



  // Disable all control points. Mark anti fall points with invert flag.

  for (i=0;i<24;i++) {PEnable[i] = false; PInvert[i] = false; }
  for (i=16;i<20;i++){PInvert[i]=true;}

  // Vector direction enable some control points.

  if (Dep.x < 0.0 ) { PEnable[4]  = true; PEnable[7] = true; PEnable[8]  = true; PEnable[11] = true; }
  if (Dep.x > 0.0 ) { PEnable[5]  = true; PEnable[6] = true; PEnable[9]  = true; PEnable[10] = true; }
  if (Dep.z > 0.0 ) { PEnable[4]  = true; PEnable[5] = true; PEnable[8]  = true; PEnable[9]  = true; }
  if (Dep.z < 0.0 ) { PEnable[6]  = true; PEnable[7] = true; PEnable[10] = true; PEnable[11] = true; }
  if (Dep.y > 0.0 ) { PEnable[12] = true; PEnable[13]= true; PEnable[14] = true; PEnable[15] = true; }
  if (Dep.y < 0.0 ) { PEnable[0]  = true; PEnable[1] = true; PEnable[2]  = true; PEnable[3]  = true; }

  PEnable[0]  = true; PEnable[1] = true; PEnable[2]  = true; PEnable[3]  = true;

  // Anti Fall test point activation

  if (this->Flag_ActivateAntiFall)
  {
    if (Dep.x >0)
    {
       if   (Dep.z>0) {PEnable[16] = true;}
       else           {PEnable[19] = true;}
    }
    else
    {
      if   (Dep.z>0) {PEnable[17] = true;}
      else           {PEnable[18] = true;}
    }

    if ( Dep.y>0.0)                                                 {PEnable[16]=PEnable[17]=PEnable[18]=PEnable[19] = false; }
    if ( IsEmpty[19] && IsEmpty[18] && IsEmpty[17] && IsEmpty[16] ) {PEnable[16]=PEnable[17]=PEnable[18]=PEnable[19] = false; }
    if ( !IsEmpty[20] )                                             {PEnable[16]=PEnable[17]=PEnable[18]=PEnable[19] = false; }

    if ( IsEmpty[19] && IsEmpty[18] && IsEmpty[17] ) PEnable[16] = true;
    if ( IsEmpty[18] && IsEmpty[17] && IsEmpty[16] ) PEnable[19] = true;
    if ( IsEmpty[17] && IsEmpty[16] && IsEmpty[19] ) PEnable[18] = true;
    if ( IsEmpty[16] && IsEmpty[19] && IsEmpty[18] ) PEnable[17] = true;
  }

  // Collision detection loop and displacement correction

  ZRayCast_in In;
  ZRayCast_out Out[32];


  In.Camera = 0;
  In.MaxCubeIterations = (ULong)ceil(DepLen / 256)+5; // 6;
  In.PlaneCubeDiff = In.MaxCubeIterations - 3;
  In.MaxDetectionDistance = 3000000.0;
  double DistanceMin;
  Long CollisionIndice;
  Bool Collided;
  Bool Continue;

  // ****

  Continue = true;
  if ( (Dep.x == 0) && (Dep.y == 0) && (Dep.z == 0.0) ) { Continue = false; return; }

  // printf("Loc: %lf %lf %lf\n",ViewDirection.origin().x,ViewDirection.origin().y,ViewDirection.origin().z);

  Location_Old = ViewDirection.origin();
  while (Continue)
  {

    Collided = false;
    DistanceMin = 10000000.0;
    CollisionIndice = -1;
    for (i=0;i<20;i++)
    {

      if (PEnable[i]) // (PEnable[i])
      {
        bool ret;
        if (PInvert[i]) ret = World->RayCast_Vector_special(P[i],Dep , &In, &(Out[i]), PInvert[i]); // If anti fall points, use special routine and invert empty/full detection.
        else            ret = World->RayCast_Vector(P[i],Dep , &In, &(Out[i]), PInvert[i]);         // Normal points.

        if (ret)
        {
          if (Out[i].CollisionDistance < DistanceMin ) { Collided = true; DistanceMin = Out[i].CollisionDistance; CollisionIndice = i; }
        }
      }
    }
    // printf("\n");

    DepLen = sqrt(Dep.x*Dep.x + Dep.y*Dep.y + Dep.z*Dep.z);

    if (Collided && (DistanceMin < DepLen || DistanceMin <= 1.1) )
    {
      // printf("Collided(Loc:%lf %lf %lf dep: %lf %lf %lf Point : %lf %lf %lf Ind:%ld ))\n",ViewDirection.origin().x,ViewDirection.origin().y,ViewDirection.origin().z, Dep.x,Dep.y,Dep.z,Out[CollisionIndice].CollisionPoint.x,Out[CollisionIndice].CollisionPoint.y, Out[CollisionIndice].CollisionPoint.z, CollisionIndice);
      // World->RayCast_Vector(P[CollisionIndice],Dep , &In, &(Out[CollisionIndice]));
      // Dep = Dep - (P[CollisionIndice] - Out[CollisionIndice].CollisionPoint);
      // SetPosition( Out[CollisionIndice].CollisionPoint );
      // Dep = 0.0;
      switch(Out[CollisionIndice].CollisionAxe)
      {
        case 0: Dep.x=0.0; Event_Collision(Velocity.x); Velocity.x = 0.0; break;
        case 1: Dep.y=0.0; Event_Collision(Velocity.y); Velocity.y = 0.0; JumpDebounce = 0;break;
        case 2: Dep.z=0.0; Event_Collision(Velocity.z); Velocity.z = 0.0; break;
      }
      //Deplacement = 0.0;
      //return;
    }
    else
    {
      ZVector3d NewLocation;

      NewLocation = ViewDirection.origin() + Dep;


        SetPosition( NewLocation );
        Deplacement = 0.0;
        Continue = false;

    }
  }

}




void ZActor_Player::Action_GoFastForward(double speed)
{
	Deplacement = Deplacement + ( ViewDirection._2d_forward() * speed );
  //Deplacement.x -= ViewDirection._2d_forward()[0]*speed;
  //Deplacement.y -= ViewDirection._2d_forward()[1]*speed;
  //Deplacement.z -= ViewDirection._2d_forward()[2]*speed;
}

void ZActor_Player::Action_GoForward()
{
  switch (ActorMode)
  {
    case 0:
	   	Deplacement = Deplacement + ( ViewDirection._2d_forward() * Speed_Walk );
          //Deplacement.x -= ViewDirection._2d_forward()[0]*Speed_Walk;
           //  Deplacement.y -= ViewDirection._2d_forward()[1]*Speed_Walk;
           //  Deplacement.z -= ViewDirection._2d_forward()[2]*Speed_Walk;
             break;

    case 1:
             // ViewDirection.pitch+=0.1; if (ViewDirection.pitch >=360.0) ViewDirection.pitch-= 360.0;
             break;
    case 2:  if (IsDead) 
			 {
				 return;
			 }
             if (PlaneEngineOn)
             {
               PlaneEngineThrust += 50.0 * GameEnv->Time_GameLoop;
               if (PlaneEngineThrust > PlaneMaxThrust) PlaneEngineThrust = PlaneMaxThrust;
               if (PlaneEngineThrust < PlaneMinThrust) PlaneEngineThrust = PlaneMinThrust;
             }
             break;

    case 3:  
	   	Deplacement = Deplacement + ( ViewDirection._2d_forward() * Speed_Walk );
			 //Deplacement.x +=ViewDirection.x_axis()[0]*Speed_Walk;
             //Deplacement.y +=ViewDirection.x_axis()[1]*Speed_Walk;
             //Deplacement.z +=ViewDirection.x_axis()[2]*Speed_Walk;
             break;

  }
}

void ZActor_Player::Action_GoBackward()
{
  switch (ActorMode)
  {
    case 0:
	   	Deplacement = Deplacement - ( ViewDirection._2d_forward() * Speed_Walk );
		//Deplacement.x += ViewDirection.z_axis()[0]*Speed_Walk;
		//Deplacement.y += ViewDirection.z_axis()[1]*Speed_Walk;
         //   Deplacement.z += ViewDirection.z_axis()[2]*Speed_Walk;
            break;
			
    case 1:
            // ViewDirection.pitch-=0.1; if (ViewDirection.pitch <0.0) ViewDirection.pitch+= 360.0;
            break;
    case 2:  if (IsDead) return;
             if (PlaneEngineOn)
             {
               PlaneEngineThrust -= 50.0 * GameEnv->Time_GameLoop;
               if (PlaneEngineThrust < PlaneMinThrust) PlaneEngineThrust = PlaneMinThrust;
             }
             break;
    case 3:
            //Deplacement.x -= sin(ViewDirection.yaw/180.0 * 3.14159265)*Speed_Walk;
            //Deplacement.z -= cos(ViewDirection.yaw/180.0 * 3.14159265)*Speed_Walk;
	   	Deplacement = Deplacement - ( ViewDirection._2d_forward() * Speed_Walk );
		//Deplacement.x -= ViewDirection.z_axis()[0]*Speed_Walk;
		//Deplacement.y -= ViewDirection.z_axis()[1]*Speed_Walk;
          //  Deplacement.z -= ViewDirection.z_axis()[2]*Speed_Walk;
            break;
  }
}

void ZActor_Player::Action_GoLeftStraff()
{
  switch (ActorMode)
  {
    case 0:
	   	Deplacement = Deplacement - ( ViewDirection._2d_left() * Speed_Walk );
            //Deplacement.x += ViewDirection._2d_forward()[2]*Speed_Walk;
            //Deplacement.y += ViewDirection._2d_forward()[1]*Speed_Walk;
            //Deplacement.z -= ViewDirection._2d_forward()[0]*Speed_Walk;
            break;
    case 1:
            // ViewDirection.yaw-=0.1 ; if (ViewDirection.yaw <0.0) ViewDirection.yaw+= 360.0;
            break;
    case 2: if (IsDead) return;
            if (PlaneEngineOn)
            {
              PlaneEngineOn = false;
              PlaneEngineThrust = 0.0;
              if ((PlaneReactorSoundHandle)) { GameEnv->Sound->Stop_PlaySound(PlaneReactorSoundHandle); PlaneReactorSoundHandle = 0; }
              GameEnv->GameWindow_Advertising->Advertise("ENGINE OFF",ZGameWindow_Advertising::VISIBILITY_MEDLOW,0,1000.0, 500.0 );
            }
            break;
    case 3:
	   	Deplacement = Deplacement - ( ViewDirection._2d_left() * Speed_Walk );
            //Deplacement.x -= ViewDirection.x_axis()[0]*Speed_Walk;
            //Deplacement.y -= ViewDirection.x_axis()[1]*Speed_Walk;
            //Deplacement.z -= ViewDirection.x_axis()[2]*Speed_Walk;
            break;
  }


}

void ZActor_Player::Action_GoRightStraff()
{
  switch (ActorMode)
  {
     case 0:
  	   	Deplacement = Deplacement + ( ViewDirection._2d_left() * Speed_Walk );
          //Deplacement.x -= ViewDirection._2d_forward()[2]*Speed_Walk;
          //  Deplacement.y -= ViewDirection._2d_forward()[1]*Speed_Walk;
           // Deplacement.z += ViewDirection._2d_forward()[0]*Speed_Walk;
             break;
     case 1:
             // ViewDirection.yaw+=0.1; if (ViewDirection.yaw >360.0) ViewDirection.yaw-= 360.0;
             break;
     case 2: if (IsDead) return;
             if (!PlaneEngineOn && ((!PlaneToohighAlt) || ViewDirection.origin().y < 250.0 * GlobalSettings.VoxelBlockSize) )
             {
               PlaneEngineOn = true;
               PlaneEngineThrust = 0.0;
               if (PlaneToohighAlt) { PlaneToohighAlt = false; PlaneEngineThrust = PlaneMaxThrust; PlaneSpeed = 30000.0; }
               GameEnv->GameWindow_Advertising->Advertise("ENGINE ON",ZGameWindow_Advertising::VISIBILITY_MEDLOW,0,1000.0, 500.0 );
               PlaneToohighAlt = false;
               PlaneWaitForRectorStartSound = false;
               if ((PlaneReactorSoundHandle)) {GameEnv->Sound->Stop_PlaySound(PlaneReactorSoundHandle);PlaneReactorSoundHandle = 0;}
               PlaneReactorSoundHandle = GameEnv->Sound->Start_PlaySound(2,false, false, 1.0,&PlaneWaitForRectorStartSound);

             }
             break;
     case 3:
	   	Deplacement = Deplacement + ( ViewDirection._2d_left() * Speed_Walk );
            //Deplacement.x += ViewDirection.x_axis()[0]*Speed_Walk;
            //Deplacement.y += ViewDirection.x_axis()[1]*Speed_Walk;
            //Deplacement.z += ViewDirection.x_axis()[2]*Speed_Walk;
             break;

  }
}

void ZActor_Player::Action_GoUp()
{
  switch (ActorMode)
  {
    case 3: Deplacement = ViewDirection.y_axis() * ( 1.0 * Speed_Walk );;
            break;
  }
}

void ZActor_Player::Action_GoDown()
{
  switch (ActorMode)
  {
  case 3: Deplacement = ViewDirection.y_axis() * ( -1.0 * Speed_Walk );
            break;
  }
}

void ZActor_Player::Action_Jump()
{
  switch (ActorMode)
  {
    case 0:
       // GameEnv->Time_GameLoop * 4.0  * 1.5
      if (JumpDebounce==0)
      {
        Velocity.y += 2000.0; JumpDebounce = 64;
        if (IsFootInLiquid) Velocity.y += 500.0;
      }
      break;
  }
}

void ZActor_Player::Start_Riding(Long x, Long y, Long z)
{
  VoxelLocation Loc;

  if ( (!Riding_IsRiding) && GameEnv->World->GetVoxelLocation(&Loc, x,y,z))
  {
    if (COMPILEOPTION_ALLOWVEHICLE == 0)
    {
      GameEnv->GameWindow_Advertising->Clear();
      GameEnv->GameWindow_Advertising->Advertise("NOT AVAILLABLE IN DEMO VERSION", ZGameWindow_Advertising::VISIBILITY_MEDIUM, 0, 5000,1500);
      return;
    }
    Riding_Voxel = Loc.Sector->Data[Loc.Offset].Data;
    Riding_VoxelInfo = Loc.Sector->Data[Loc.Offset].OtherInfos;

    if (GameEnv->VoxelTypeManager.VoxelTable[Riding_Voxel]->Is_Rideable && COMPILEOPTION_ALLOWVEHICLE == 1 )
    {
      Loc.Sector->Data[Loc.Offset].Data = 0;
      Loc.Sector->Data[Loc.Offset].OtherInfos =0;
      GameEnv->World->SetVoxel_WithCullingUpdate(x,y,z,0,ZVoxelSector::CHANGE_CRITICAL,true,0);
      Riding_IsRiding = true;

      switch(Riding_Voxel)
      {
        case 96: ActorMode = 2;
			//ViewDirection.RotateYaw( -45.0 );
			//ViewDirection.RotateYaw( 45.0 );
                 //ViewDirection.pitch = 0.0;
                 //ViewDirection.roll  = 0.0;
                 //ViewDirection.yaw -= 45.0;                                           
				 //if (ViewDirection.yaw < 0.0) ViewDirection.yaw += 360.0;
			double yaw = 180 + ViewDirection.yaw();
			ViewDirection.RotateAbsolute( (floor((yaw - 45) / 90.0) + 1.0) * 90.0 - 180, 0, 0 );  
				 //if (ViewDirection.yaw >= 360.0) ViewDirection.yaw -= 360.0;

                 ZVector3d NewPlayerLocation;
                 GameEnv->World->Convert_Coords_VoxelToPlayer(x,y,z,NewPlayerLocation.x,NewPlayerLocation.y, NewPlayerLocation.z);
                 NewPlayerLocation.x += (GlobalSettings.VoxelBlockSize/2); NewPlayerLocation.z += (GlobalSettings.VoxelBlockSize/2);
                 ViewDirection.origin() = NewPlayerLocation;

                 break;

      }
    }
  }
}

void ZActor_Player::Stop_Riding()
{
  ZVector3L VLoc;
  VoxelLocation Loc;

  if (Riding_IsRiding)
  {
    GameEnv->World->Convert_Coords_PlayerToVoxel(ViewDirection.origin().x, ViewDirection.origin().y, ViewDirection.origin().z, VLoc.x, VLoc.y, VLoc.z);
    if (GameEnv->World->SetVoxel_WithCullingUpdate(VLoc.x, VLoc.y, VLoc.z, Riding_Voxel, ZVoxelSector::CHANGE_CRITICAL, false, &Loc))
    {
      Loc.Sector->Data[Loc.Offset].OtherInfos = Riding_VoxelInfo;
      Riding_Voxel = 0;
      Riding_VoxelInfo = 0;
      Riding_IsRiding = false;

      ViewDirection.origin().y += GlobalSettings.VoxelBlockSize;
      ActorMode = 0;
    }
  }
}

void ZActor_Player::Event_Death()
{
  Init(true);
}


void ZActor_Player::Event_PlaneCrash()
{
  GameEnv->GameWindow_Advertising->Clear();
  GameEnv->GameWindow_Advertising->Advertise("CRASH", ZGameWindow_Advertising::VISIBILITY_MEDLOW, 0, 3000.0, 0.0);
  if ((PlaneReactorSoundHandle)) { GameEnv->Sound->Stop_PlaySound(PlaneReactorSoundHandle); PlaneReactorSoundHandle = 0; }
  GameEnv->Sound->PlaySound(1);
  IsDead = true;
  DeathChronometer = 10000.0;
  PlaneEngineThrust = 0.0;
}

void ZActor_Player::Event_DeadlyFall()
{
  GameEnv->GameWindow_Advertising->Clear();
  GameEnv->GameWindow_Advertising->Advertise("FATAL FALL : YOU ARE DEAD", ZGameWindow_Advertising::VISIBILITY_HIGH, 0, 8000.0, 0.0);
  GameEnv->Sound->PlaySound(1);
  IsDead = true;
  DeathChronometer = 10000.0;
  ViewDirection.origin().y -= GlobalSettings.VoxelBlockSize;
}

