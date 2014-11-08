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
#include <GL/glew.h>
#include <math.h>
#include <stdio.h>

#  include "ZRender_Interface.h"

#ifndef Z_ZHIGHPERFTIMER_H
#  include "ZHighPerfTimer.h"
#endif

#ifndef Z_ZGAME_H
#  include "ZGame.h"
#endif

#ifndef Z_ZGAMESTAT_H
#  include "ZGameStat.h"
#endif



void ZRender_Interface::SetWorld( ZVoxelWorld * World )
{
	this->voxelCuller->SetWorld( World );
  this->World = World;
}

void ZRender_Interface::SetCamera( ZCamera * Camera )
{
  this->Camera = Camera;
}

void ZRender_Interface::SetActor( ZActor * Actor )
{
  this->Actor = Actor;
  if( Actor )
	this->Camera = &Actor->Camera;
  else
	  this->Camera = NULL;
}

void ZRender_Interface::SetVoxelTypeManager( ZVoxelTypeManager * Manager )
{
  VoxelTypeManager = Manager;
}

void ZRender_Interface::Init()
{
  RadiusZones.SetSize(17,7,17);
  // RadiusZones.DrawZones( 5.0, 3.5, 3.0, 2.0 );
  // RadiusZones.DebugOut();
}

void ZRender_Interface::Render_DebugLine( ZVector3d & Start, ZVector3d & End)
{


  glDisable(GL_TEXTURE_2D);
  glEnable(GL_LINE_SMOOTH);

  glEnable (GL_LINE_SMOOTH);
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glHint (GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
  glLineWidth (3.5);

  glColor3f(1.0,1.0,1.0);
  //glLineWidth(0.001f);
  //glPointSize(0.001f);
    glBegin(GL_LINES);
      glVertex3f((float)Start.x,(float)Start.y,(float)Start.z);glVertex3f((float)End.x,(float)End.y,(float)End.z);
    glEnd();

  glColor3f(1.0,1.0,1.0);
  glEnable(GL_TEXTURE_2D);
}

void ZRender_Interface::Render_VoxelSelector(ZVoxelCoords * SelectedVoxel, float r, float g, float b)
{

  //      PointedCube.x = 1;
  //      PointedCube.y = 0;
  //      PointedCube.z = 0;
  /*


        ZVector3f P1,P2,P3,P4,P5,P6,P7,P8;
        P1.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + 0.0f;   P1.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + 0.0f;   P1.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + 0.0f;
        P2.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + 0.0f;   P2.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + 0.0f;   P2.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize;
        P3.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P3.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + 0.0f;   P3.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize;
        P4.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P4.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + 0.0f;   P4.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + 0.0f;
        P5.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + 0.0f;   P5.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P5.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + 0.0f;
        P6.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + 0.0f;   P6.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P6.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize;
        P7.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P7.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P7.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize;
        P8.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P8.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P8.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + 0.0f;
*/

        glDisable(GL_TEXTURE_2D);

        //      PointedCube.x = 1;
        //      PointedCube.y = 0;
        //      PointedCube.z = 0;

              ZVector3f P1,P2,P3,P4,P5,P6,P7,P8;
              P1.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + 0.0f;   P1.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + 0.0f; P1.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + 0.0f;
              P2.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + 0.0f;   P2.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + 0.0f; P2.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize;
              P3.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P3.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + 0.0f; P3.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize;
              P4.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P4.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + 0.0f; P4.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + 0.0f;
              P5.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + 0.0f;   P5.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P5.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + 0.0f;
              P6.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + 0.0f;   P6.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P6.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize;
              P7.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P7.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P7.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize;
              P8.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P8.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P8.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + 0.0f;


              glDisable(GL_TEXTURE_2D);
              glColor3f(r,g,b);
              glEnable(GL_LINE_SMOOTH);

              glEnable (GL_LINE_SMOOTH);
              glEnable (GL_BLEND);
              glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
              glHint (GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
              glLineWidth (3.5);


              //glLineWidth(0.001f);
              //glPointSize(0.001f);
                glBegin(GL_LINES);
                  glVertex3f(P1.x,P1.y,P1.z);glVertex3f(P2.x,P2.y,P2.z);
                  glVertex3f(P2.x,P2.y,P2.z);glVertex3f(P3.x,P3.y,P3.z);
                  glVertex3f(P3.x,P3.y,P3.z);glVertex3f(P4.x,P4.y,P4.z);
                  glVertex3f(P4.x,P4.y,P4.z);glVertex3f(P1.x,P1.y,P1.z);

                  glVertex3f(P5.x,P5.y,P5.z);glVertex3f(P6.x,P6.y,P6.z);
                  glVertex3f(P6.x,P6.y,P6.z);glVertex3f(P7.x,P7.y,P7.z);
                  glVertex3f(P7.x,P7.y,P7.z);glVertex3f(P8.x,P8.y,P8.z);
                  glVertex3f(P8.x,P8.y,P8.z);glVertex3f(P5.x,P5.y,P5.z);

                  glVertex3f(P1.x,P1.y,P1.z);glVertex3f(P5.x,P5.y,P5.z);
                  glVertex3f(P2.x,P2.y,P2.z);glVertex3f(P6.x,P6.y,P6.z);
                  glVertex3f(P3.x,P3.y,P3.z);glVertex3f(P7.x,P7.y,P7.z);
                  glVertex3f(P4.x,P4.y,P4.z);glVertex3f(P8.x,P8.y,P8.z);
                glEnd();
              glColor3f(1.0,1.0,1.0);

              glEnable(GL_TEXTURE_2D);
        glEnable(GL_LINE_SMOOTH);

        glEnable (GL_LINE_SMOOTH);
        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glHint (GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
        glLineWidth (3.5);


        //glLineWidth(0.001f);
        //glPointSize(0.001f);
          glBegin(GL_LINES);
            glVertex3f(P1.x,P1.y,P1.z);glVertex3f(P2.x,P2.y,P2.z);
            glVertex3f(P2.x,P2.y,P2.z);glVertex3f(P3.x,P3.y,P3.z);
            glVertex3f(P3.x,P3.y,P3.z);glVertex3f(P4.x,P4.y,P4.z);
            glVertex3f(P4.x,P4.y,P4.z);glVertex3f(P1.x,P1.y,P1.z);

            glVertex3f(P5.x,P5.y,P5.z);glVertex3f(P6.x,P6.y,P6.z);
            glVertex3f(P6.x,P6.y,P6.z);glVertex3f(P7.x,P7.y,P7.z);
            glVertex3f(P7.x,P7.y,P7.z);glVertex3f(P8.x,P8.y,P8.z);
            glVertex3f(P8.x,P8.y,P8.z);glVertex3f(P5.x,P5.y,P5.z);

            glVertex3f(P1.x,P1.y,P1.z);glVertex3f(P5.x,P5.y,P5.z);
            glVertex3f(P2.x,P2.y,P2.z);glVertex3f(P6.x,P6.y,P6.z);
            glVertex3f(P3.x,P3.y,P3.z);glVertex3f(P7.x,P7.y,P7.z);
            glVertex3f(P4.x,P4.y,P4.z);glVertex3f(P8.x,P8.y,P8.z);
          glEnd();
        glColor3f(1.0,1.0,1.0);
        glEnable(GL_TEXTURE_2D);

}



Bool ZRender_Interface::LoadVoxelTexturesToGPU(PTRSZVAL psvInit)
{
  ULong i;
  ZVoxelType * VoxelType;

  for (i=0;i<65536;i++)
  {
    if ( !(VoxelType = VoxelTypeManager->VoxelTable[i])->Is_NoType)
    {
      if (VoxelType->MainTexture)
      {
        glGenTextures(1,&VoxelType->OpenGl_TextureRef[psvInit]);
        glBindTexture(GL_TEXTURE_2D,VoxelType->OpenGl_TextureRef[psvInit]);
        /*
        glTexImage2D (GL_TEXTURE_2D,      //Type : texture 2D
                      0,                 //Mipmap : none
                      GL_RGBA8,          //Format : RGBA
                      VoxelType->MainTexture->Width,         //Width
                      VoxelType->MainTexture->Height,        //Height
                      0,                 //Largeur du bord : 0     img4_m6.LoadBMP("textures/texture_cubeglow_mip_6.bmp");
                      GL_BGRA,    //Format : RGBA
                      GL_UNSIGNED_BYTE,   //Type des couleurs
                      (GLubyte *)VoxelType->MainTexture->BitmapMemory//Addresse de l'image
                     );
        */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // GL_LINEAR GL_NEAREST
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // if (i & 1) glTexParameteri(GL_TEXTURE_2D, 0x84FE /*TEXTURE_MAX_ANISOTROPY_EXT*/, 8);
        glTexParameteri(GL_TEXTURE_2D, 0x84FE /*TEXTURE_MAX_ANISOTROPY_EXT*/, 8);
        gluBuild2DMipmaps(GL_TEXTURE_2D,      //Type : texture 2D
            GL_RGBA8,          //Format : RGBA
            VoxelType->MainTexture->Width,         //Width
            VoxelType->MainTexture->Height,        //Height
            GL_RGBA,//BGRA,    //Format : RGBA
            GL_UNSIGNED_BYTE,   //Type des couleurs
            (GLubyte *)VoxelType->MainTexture->BitmapMemory//Addresse de l'image
           );


          //glTexEnvf(0x8500 /* TEXTURE_FILTER_CONTROL_EXT */, 0x8501 /* TEXTURE_LOD_BIAS_EXT */,3.0);
        // if ((i & 1) ) glTexEnvf(0x8500 /* TEXTURE_FILTER_CONTROL_EXT */, 0x8501 /* TEXTURE_LOD_BIAS_EXT */,-4.25);

        glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
      }
    }
  }

  return(true);
}

void ZRender_Interface::FreeDisplayData(ZVoxelSector * Sector)
{
  ZRender_Interface_displaydata * DisplayData;

  DisplayData = (ZRender_Interface_displaydata *)Sector->DisplayData;

  if (DisplayData)
  {
    if (DisplayData->DisplayList_Regular)
    {
		glDeleteLists (DisplayData->DisplayList_Regular[current_gl_camera],1);
      DisplayData->DisplayList_Regular[current_gl_camera] = 0;
    }
    if (DisplayData->DisplayList_Transparent)
    {
      glDeleteLists (DisplayData->DisplayList_Transparent[current_gl_camera],1);
      DisplayData->DisplayList_Transparent[current_gl_camera] = 0;
    }
    delete DisplayData;
  }
}

Bool ZRender_Interface::LoadTexturesToGPU(PTRSZVAL psvInit)
{
  ULong i;
  ULong TextureCount;
  ZTexture_Entry * Entry;

  if (!TextureManager) return(false);

  TextureCount = TextureManager->GetTexture_Count();

  for (i=0;i<TextureCount;i++)
  {
    if ((Entry = TextureManager->GetTextureEntry(i)))
    {
      glGenTextures(1,&Entry->OpenGl_TextureRef[psvInit]);
	  glBindTexture(GL_TEXTURE_2D,Entry->OpenGl_TextureRef[current_gl_camera]);
      if (Entry->LinearInterpolation)
      {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // GL_LINEAR GL_NEAREST
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }
      else
      {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // GL_LINEAR GL_NEAREST
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }
      glTexParameteri(GL_TEXTURE_2D, 0x84FE /*TEXTURE_MAX_ANISOTROPY_EXT*/, 8);
      gluBuild2DMipmaps(GL_TEXTURE_2D,      //Type : texture 2D
          GL_RGBA8,          //Format : RGBA
          Entry->Texture->Width,         //Width
          Entry->Texture->Height,        //Height
          GL_RGBA,//GL_BGRA,    //Format : RGBA
          GL_UNSIGNED_BYTE,   //Type des couleurs
          (GLubyte *)Entry->Texture->BitmapMemory//Addresse de l'image
         );


          //glTexEnvf(0x8500 /* TEXTURE_FILTER_CONTROL_EXT */, 0x8501 /* TEXTURE_LOD_BIAS_EXT */,3.0);
        // if ((i & 1) ) glTexEnvf(0x8500 /* TEXTURE_FILTER_CONTROL_EXT */, 0x8501 /* TEXTURE_LOD_BIAS_EXT */,-4.25);

        glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
    }
  }

  return(true);
}



void ZRender_Interface::ComputeAndSetAspectRatio(double VerticalFOV, double PixelAspectRatio, ZVector2L & ViewportResolution)
{
  double FocusDistance = 50.0;
  VerticalFOV = 63.597825649;
  PixelAspectRatio = 1.0;

  double Frustum_V;
  double Frustum_H;
  double Aspect_Ratio;

  Aspect_Ratio = (ViewportResolution.x / ViewportResolution.y) * PixelAspectRatio;

  // FOV Limitation to safe values.

  if (VerticalFOV < 5.0 ) VerticalFOV = 5.0;
  if (VerticalFOV > 160.0 ) VerticalFOV = 160.0;

  Frustum_V = tan(VerticalFOV / 2.0) * FocusDistance;
  Frustum_H = Frustum_V * Aspect_Ratio;

  glFrustum(Frustum_H, -Frustum_H, -Frustum_V, Frustum_V, FocusDistance, 1000000.0); // Official Way

}

