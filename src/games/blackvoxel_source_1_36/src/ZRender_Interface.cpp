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



Bool ZRender_Interface::LoadVoxelTexturesToGPU()
{
  ULong i;
  ZVoxelType * VoxelType;

  for (i=0;i<65536;i++)
  {
    if ( !(VoxelType = VoxelTypeManager->VoxelTable[i])->Is_NoType)
    {
      if (VoxelType->MainTexture)
      {
        glGenTextures(1,&VoxelType->OpenGl_TextureRef);
        glBindTexture(GL_TEXTURE_2D,VoxelType->OpenGl_TextureRef);
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
            GL_BGRA,    //Format : RGBA
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
      glDeleteLists (DisplayData->DisplayList_Regular,1);
      DisplayData->DisplayList_Regular = 0;
    }
    if (DisplayData->DisplayList_Transparent)
    {
      glDeleteLists (DisplayData->DisplayList_Transparent,1);
      DisplayData->DisplayList_Transparent = 0;
    }
    delete DisplayData;
  }
}

Bool ZRender_Interface::LoadTexturesToGPU()
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
      glGenTextures(1,&Entry->OpenGl_TextureRef);
      glBindTexture(GL_TEXTURE_2D,Entry->OpenGl_TextureRef);
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
          GL_BGRA,    //Format : RGBA
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


#define TC_S0_P0_X 0.0
#define TC_S0_P0_Y       0.25
#define TC_S0_P4_X 0.25
#define TC_S0_P4_Y       0.25
#define TC_S0_P5_X 0.25   
#define TC_S0_P5_Y       0.50
#define TC_S0_P1_X 0.0
#define TC_S0_P1_Y       0.50
#define TC_S1_P0_X 0.25
#define TC_S1_P0_Y 0.0
#define TC_S1_P3_X 0.50
#define TC_S1_P3_Y 0.0
#define TC_S1_P7_X 0.50
#define TC_S1_P7_Y 0.25
#define TC_S1_P4_X 0.25
#define TC_S1_P4_Y 0.25
#define TC_S2_P2_X 0.50
#define TC_S2_P2_Y 0.75
#define TC_S2_P6_X 0.50 
#define TC_S2_P6_Y 0.50
#define TC_S2_P5_X 0.25
#define TC_S2_P5_Y 0.50
#define TC_S2_P1_X 0.25 
#define TC_S2_P1_Y 0.75
#define TC_S3_P7_X 0.50
#define TC_S3_P7_Y 0.25
#define TC_S3_P3_X 0.75
#define TC_S3_P3_Y 0.25
#define TC_S3_P2_X 0.75
#define TC_S3_P2_Y 0.50
#define TC_S3_P6_X 0.50
#define TC_S3_P6_Y 0.50
#define TC_S4_P4_X 0.25
#define TC_S4_P4_Y 0.25
#define TC_S4_P7_X 0.50
#define TC_S4_P7_Y 0.25
#define TC_S4_P6_X 0.50
#define TC_S4_P6_Y 0.50
#define TC_S4_P5_X 0.25
#define TC_S4_P5_Y 0.50
#define TC_S5_P0_X 1.0
#define TC_S5_P0_Y 0.25
#define TC_S5_P1_X 1.0
#define TC_S5_P1_Y 0.50
#define TC_S5_P2_X 0.75
#define TC_S5_P2_Y 0.50
#define TC_S5_P3_X 0.75
#define TC_S5_P3_Y 0.25
//		glTRI( 5,0,1,2,3 );
#define glTRI_normal(s,a,b,c,d)             glTexCoord2f(TC_S##s##_P##a##_X,TC_S##s##_P##a##_Y); glVertex3f(P##a.x, P##a.y, P##a.z );  \
	glTexCoord2f(TC_S##s##_P##b##_X,TC_S##s##_P##b##_Y);  glVertex3f(P##b.x, P##b.y, P##b.z );        \
	glTexCoord2f(TC_S##s##_P##d##_X,TC_S##s##_P##d##_Y);  glVertex3f(P##d.x, P##d.y, P##d.z );        \
           glTexCoord2f(TC_S##s##_P##d##_X,TC_S##s##_P##d##_Y);  glVertex3f(P##d.x, P##d.y, P##d.z ); \
           glTexCoord2f(TC_S##s##_P##b##_X,TC_S##s##_P##b##_Y); glVertex3f(P##b.x, P##b.y, P##b.z );  \
		   glTexCoord2f(TC_S##s##_P##c##_X,TC_S##s##_P##c##_Y); glVertex3f(P##c.x, P##c.y, P##c.z );

#define glTRI_except(s,a,b,c,d,e,f,g,h)             glTexCoord2f(TC_S##s##_P##a##_X,TC_S##s##_P##a##_Y); glVertex3f(P##e.x, P##e.y, P##e.z );  \
	glTexCoord2f(TC_S##s##_P##b##_X,TC_S##s##_P##b##_Y);  glVertex3f(P##f.x, P##f.y, P##f.z );        \
	glTexCoord2f(TC_S##s##_P##d##_X,TC_S##s##_P##d##_Y);  glVertex3f(P##h.x, P##h.y, P##h.z );        \
           glTexCoord2f(TC_S##s##_P##d##_X,TC_S##s##_P##d##_Y);  glVertex3f(P##h.x, P##h.y, P##h.z ); \
           glTexCoord2f(TC_S##s##_P##b##_X,TC_S##s##_P##b##_Y); glVertex3f(P##f.x, P##f.y, P##f.z );  \
		   glTexCoord2f(TC_S##s##_P##c##_X,TC_S##s##_P##c##_Y); glVertex3f(P##g.x, P##g.y, P##g.z );

void ZRender_Interface::EmitFaces( ZVoxelType ** VoxelTypeTable, UShort &VoxelType, UShort &prevVoxelType, ULong info
							  , Long x, Long y, Long z
							  , Long Sector_Display_x, Long Sector_Display_y, Long Sector_Display_z )
{
  ZVector3f P0,P1,P2,P3,P4,P5,P6,P7;
  float cubx, cuby, cubz;
	        // Offset = y + ( x << ZVOXELBLOCSHIFT_Y )+ (z << (ZVOXELBLOCSHIFT_Y + ZVOXELBLOCSHIFT_X));

        // glTexEnvf(0x8500 /* TEXTURE_FILTER_CONTROL_EXT */, 0x8501 /* TEXTURE_LOD_BIAS_EXT */,VoxelTypeManager->VoxelTable[VoxelType]->TextureLodBias);
        if (VoxelType != prevVoxelType) glBindTexture(GL_TEXTURE_2D, VoxelTypeManager->VoxelTable[VoxelType]->OpenGl_TextureRef);
        prevVoxelType = VoxelType;
        cubx = (float)(x*GlobalSettings.VoxelBlockSize + Sector_Display_x);
        cuby = (float)(y*GlobalSettings.VoxelBlockSize + Sector_Display_y);
        cubz = (float)(z*GlobalSettings.VoxelBlockSize + Sector_Display_z);

        if (VoxelTypeTable[VoxelType]->DrawInfo & ZVOXEL_DRAWINFO_SPECIALRENDERING ) 
		{
			VoxelTypeTable[VoxelType]->SpecialRender(cubx,cuby,cubz); 
			return; 
		}

        P0.x = cubx;           P0.y = cuby;          P0.z = cubz;
        P1.x = cubx + GlobalSettings.VoxelBlockSize;  P1.y = cuby;          P1.z = cubz;
        P2.x = cubx + GlobalSettings.VoxelBlockSize;  P2.y = cuby;          P2.z = cubz+GlobalSettings.VoxelBlockSize;
        P3.x = cubx;           P3.y = cuby;          P3.z = cubz+GlobalSettings.VoxelBlockSize;
        P4.x = cubx;           P4.y = cuby + GlobalSettings.VoxelBlockSize; P4.z = cubz;
        P5.x = cubx + GlobalSettings.VoxelBlockSize;  P5.y = cuby + GlobalSettings.VoxelBlockSize; P5.z = cubz;
        P6.x = cubx + GlobalSettings.VoxelBlockSize;  P6.y = cuby + GlobalSettings.VoxelBlockSize; P6.z = cubz + GlobalSettings.VoxelBlockSize;
        P7.x = cubx;           P7.y = cuby + GlobalSettings.VoxelBlockSize; P7.z = cubz + GlobalSettings.VoxelBlockSize;


		// if it's otherwise entirely covered...
	  if( !( info & ( DRAWFACE_ALL ) )
		 &&( !( info & ( DRAWFACE_BEHIND_HAS_ABOVE | DRAWFACE_LEFT_HAS_ABOVE ) )
		 || !( info & ( DRAWFACE_AHEAD_HAS_ABOVE | DRAWFACE_LEFT_HAS_ABOVE ) )
		 || !( info & ( DRAWFACE_BEHIND_HAS_ABOVE | DRAWFACE_RIGHT_HAS_ABOVE ) )
		 || !( info & ( DRAWFACE_AHEAD_HAS_ABOVE | DRAWFACE_RIGHT_HAS_ABOVE ) ) )
		  )
	  {
		  //info |= DRAWFACE_ABOVE;
	  }

#if 1
			  if( ( info & DRAWFACE_ALL ) == (DRAWFACE_LEFT|DRAWFACE_ABOVE|DRAWFACE_AHEAD ) )
			  {
				  if( info & ( DRAWFACE_BELOW_HAS_LEFT | DRAWFACE_BELOW_HAS_AHEAD ) )
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						
						glBegin(GL_TRIANGLES);
							glTRI_except( 4, 4, 7, 6, 5, 0, 3, 6, 1 );
						glEnd();
						
						Stat_FaceTop++;
				  }
				  else
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P3.x, P3.y, P3.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P6.x, P6.y, P6.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P1.x, P1.y, P1.z );
						glEnd();

						Stat_FaceTop++;
				  }
			  }
			  else if( ( info & DRAWFACE_ALL ) == (DRAWFACE_LEFT|DRAWFACE_BELOW|DRAWFACE_AHEAD ) )
			  {
				  if( info & ( DRAWFACE_ABOVE_HAS_LEFT | DRAWFACE_ABOVE_HAS_AHEAD ) )
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
							glTRI_except( 5, 0, 1, 2, 3, 4, 5, 2, 7 );
						glEnd();

						Stat_FaceTop++;
				  }
				  else
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P7.x, P7.y, P7.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P6.y, P5.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P2.x, P1.y, P2.z );
						glEnd();

						Stat_FaceTop++;
				  }
			  }
			  else if( ( info & DRAWFACE_ALL ) == (DRAWFACE_RIGHT|DRAWFACE_ABOVE|DRAWFACE_AHEAD ) )
			  {
				  if( info & ( DRAWFACE_BELOW_HAS_RIGHT | DRAWFACE_BELOW_HAS_AHEAD ) )
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
							glTRI_except( 4, 4, 7, 6, 5, 0, 7, 2, 1 );
						glEnd();

						Stat_FaceTop++;
				  }
				  else
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P7.x, P7.y, P7.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P2.x, P2.y, P2.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glEnd();

						Stat_FaceTop++;
				  }
			  }
			  else if( ( info & DRAWFACE_ALL ) == (DRAWFACE_RIGHT|DRAWFACE_BELOW|DRAWFACE_AHEAD ) )
			  {
				  if( info & ( DRAWFACE_ABOVE_HAS_LEFT | DRAWFACE_ABOVE_HAS_AHEAD ) )
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
	  							glTRI_except( 5, 0, 1, 2, 3, 0, 5, 6, 3 );
						glEnd();

						Stat_FaceTop++;
				  }
				  else
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						  glTexCoord2f(0.25,0.25); glVertex3f(P6.x, P6.y, P6.z );
						glEnd();

						Stat_FaceTop++;
				  }
			  }
			  else if( ( info & DRAWFACE_ALL ) == (DRAWFACE_LEFT|DRAWFACE_ABOVE|DRAWFACE_BEHIND ) )
			  {
				  if( info & ( DRAWFACE_BELOW_HAS_LEFT | DRAWFACE_BELOW_HAS_BEHIND ) )
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
	  					  glTRI_except( 4, 4, 7, 6, 5, 0, 3, 2, 5 );
						glEnd();

						Stat_FaceTop++;
				  }
				  else if( info & ( DRAWFACE_BELOW_HAS_AHEAD  ) )
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
	  					  glTRI_except( 4, 4, 7, 6, 5, 4, 0, 2, 5 );
						glEnd();

						Stat_FaceTop++;
				  }
				  else
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P0.x, P0.y, P0.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P2.x, P2.y, P2.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P5.x, P5.y, P5.z );
						glEnd();
				  }
						Stat_FaceTop++;
			  }
			  else if( ( info & DRAWFACE_ALL ) == (DRAWFACE_LEFT|DRAWFACE_BELOW|DRAWFACE_BEHIND ) )
			  {
				  if( info & ( DRAWFACE_BELOW_HAS_LEFT | DRAWFACE_BELOW_HAS_BEHIND ) )
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
	  					  glTRI_except( 5, 0, 1, 2, 3, 4, 1, 6, 7 );
						glEnd();

						Stat_FaceTop++;
				  }
				  else
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P1.x, P1.y, P1.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P6.x, P6.y, P6.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						glEnd();

					Stat_FaceTop++;
				  }
			  }
			  else if( ( info & DRAWFACE_ALL ) == (DRAWFACE_RIGHT|DRAWFACE_ABOVE|DRAWFACE_BEHIND ) )
			  {
				  if( info & ( DRAWFACE_BELOW_HAS_LEFT | DRAWFACE_BELOW_HAS_BEHIND ) )
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
	  					  glTRI_except( 4, 4, 7, 6, 5, 4, 3, 2, 1 );
						glEnd();

						Stat_FaceTop++;
				  }
				  else
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P1.x, P1.y, P1.z );
						glEnd();

						Stat_FaceTop++;
				  }
			  }
			  else if( ( info & DRAWFACE_ALL ) == (DRAWFACE_RIGHT|DRAWFACE_BELOW|DRAWFACE_BEHIND ) )
			  {
				  if( info & ( DRAWFACE_BELOW_HAS_LEFT | DRAWFACE_BELOW_HAS_BEHIND ) )
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
	  					  glTRI_except( 5, 0, 1, 2, 3, 0, 5, 6, 7 );
						glEnd();

						Stat_FaceTop++;
				  }
				  else
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P5.y, P5.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P7.x, P7.y, P7.z );
						  glTexCoord2f(0.25,0.25); glVertex3f(P0.x, P0.y, P0.z );
						glEnd();

						Stat_FaceTop++;
				  }
			  }
			  else 
#endif
#if 1
			  if( ( info & DRAWFACE_ALL ) == (DRAWFACE_LEFT|DRAWFACE_ABOVE ) )
			  {
				  if( ( info & (DRAWFACE_AHEAD_HAS_ABOVE|DRAWFACE_ABOVE_HAS_LEFT|DRAWFACE_BEHIND_HAS_ABOVE|DRAWFACE_BELOW_HAS_LEFT) ) 
					  == (DRAWFACE_AHEAD_HAS_ABOVE|DRAWFACE_ABOVE_HAS_LEFT|DRAWFACE_BEHIND_HAS_ABOVE|DRAWFACE_BELOW_HAS_LEFT) )
				  {
					  goto default_draw;
				  }
				  else if( ( info & (DRAWFACE_AHEAD_HAS_ABOVE|DRAWFACE_AHEAD_HAS_LEFT) ) == (DRAWFACE_AHEAD_HAS_ABOVE|DRAWFACE_AHEAD_HAS_LEFT) )
				  {
					glBegin(GL_TRIANGLES);
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.50,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P5.y, P5.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.50,0.0);  glVertex3f(P6.x, P6.y, P6.z );
						glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P5.y, P5.z );

						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						glTexCoord2f(0.25,0.25); glVertex3f(P6.x, P6.y, P6.z );
					glEnd();
				  }
				  else if( ( info & (DRAWFACE_BEHIND_HAS_ABOVE|DRAWFACE_BEHIND_HAS_LEFT) ) == (DRAWFACE_BEHIND_HAS_ABOVE|DRAWFACE_BEHIND_HAS_LEFT) )
				  {
					glBegin(GL_TRIANGLES);
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.50,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P5.y, P5.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.50,0.0);  glVertex3f(P6.x, P6.y, P6.z );
						glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P5.y, P5.z );

						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						glTexCoord2f(0.25,0.25); glVertex3f(P6.x, P6.y, P6.z );
					glEnd();
				  }
				  else 
				  
				  {

						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
   						  glTRI_except( 4,4,7,6,5,0,3,6,5 );
						glEnd();

						Stat_FaceTop++;
				  }
			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_RIGHT|DRAWFACE_ABOVE ) )
			  {
				  if( ( ( info & (DRAWFACE_AHEAD_HAS_ABOVE|DRAWFACE_AHEAD_HAS_RIGHT|DRAWFACE_BEHIND_HAS_ABOVE|DRAWFACE_BEHIND_HAS_RIGHT) )
					  == (DRAWFACE_AHEAD_HAS_ABOVE|DRAWFACE_AHEAD_HAS_RIGHT|DRAWFACE_BEHIND_HAS_ABOVE|DRAWFACE_BEHIND_HAS_RIGHT) ) )
				  {
					  goto default_draw;
				  }
				  else if( ( info & (DRAWFACE_AHEAD_HAS_ABOVE|DRAWFACE_AHEAD_HAS_RIGHT) ) == (DRAWFACE_AHEAD_HAS_ABOVE|DRAWFACE_AHEAD_HAS_RIGHT) )
				  {
					glBegin(GL_TRIANGLES);
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.50,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P5.y, P5.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.50,0.0);  glVertex3f(P6.x, P6.y, P6.z );
						glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P5.y, P5.z );

						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						glTexCoord2f(0.25,0.25); glVertex3f(P6.x, P6.y, P6.z );
					glEnd();
				  }
				  else if( ( info & (DRAWFACE_BEHIND_HAS_ABOVE|DRAWFACE_BEHIND_HAS_RIGHT) ) == (DRAWFACE_BEHIND_HAS_ABOVE|DRAWFACE_BEHIND_HAS_RIGHT) )
				  {
					glBegin(GL_TRIANGLES);
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.50,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P5.y, P5.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.50,0.0);  glVertex3f(P6.x, P6.y, P6.z );
						glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P5.y, P5.z );

						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						glTexCoord2f(0.25,0.25); glVertex3f(P6.x, P6.y, P6.z );
					glEnd();
				  }
				  else 
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
   						  glTRI_except( 4,4,7,6,5,4,7,2,1 );
						glEnd();

						Stat_FaceTop++;
				  }
			  }
			  else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_AHEAD|DRAWFACE_ABOVE ) )
			  {
                Stat_RenderDrawFaces++;
				Stat_FaceFront++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.25); glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.25,0.0);  glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.50,0.0);  glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.50,0.0);  glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.50,0.25); glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.25,0.25); glVertex3f(P1.x, P1.y, P1.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else 
#endif
				  if( ( info & DRAWFACE_ALL )  == (DRAWFACE_BEHIND|DRAWFACE_ABOVE ) )
			  {
				  if( info & ( DRAWFACE_LEFT_HAS_ABOVE | DRAWFACE_LEFT_HAS_BEHIND ) )
				  {
					  if( info & ( DRAWFACE_RIGHT_HAS_ABOVE | DRAWFACE_RIGHT_HAS_BEHIND ) )
						  goto default_draw;
						Stat_RenderDrawFaces++;
						Stat_FaceRight++;
						glBegin(GL_TRIANGLES);

						  glTexCoord2f(0.50,0.50);  glVertex3f(P3.x, P3.y, P3.z );
						  glTexCoord2f(0.25,0.50); glVertex3f(P2.x, P2.y, P2.z );
						  glTexCoord2f(0.50,0.75);  glVertex3f(P6.x, P6.y, P6.z );
						  glTexCoord2f(0.50,0.50);  glVertex3f(P4.x, P4.y, P4.z );
						  glTexCoord2f(0.25,0.50); glVertex3f(P3.x, P3.y, P3.z );
						  glTexCoord2f(0.50,0.75);  glVertex3f(P6.x, P6.y, P6.z );
						  glTexCoord2f(0.25,0.75); glVertex3f(P4.x, P4.y, P4.z );
						  glTexCoord2f(0.50,0.75);  glVertex3f(P6.x, P6.y, P6.z );
						  glTexCoord2f(0.25,0.50); glVertex3f(P5.x, P5.y, P5.z );
						glEnd();

						Stat_FaceTop++;
				  }
				  else 	if( info & ( DRAWFACE_RIGHT_HAS_ABOVE | DRAWFACE_RIGHT_HAS_BEHIND ) )
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceRight++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.50,0.50);  glVertex3f(P3.x, P3.y, P3.z );
						  glTexCoord2f(0.25,0.50); glVertex3f(P2.x, P2.y, P2.z );
						  glTexCoord2f(0.50,0.75);  glVertex3f(P6.x, P6.y, P6.z );
						  glTexCoord2f(0.50,0.50);  glVertex3f(P4.x, P4.y, P4.z );
						  glTexCoord2f(0.25,0.50); glVertex3f(P3.x, P3.y, P3.z );
						  glTexCoord2f(0.50,0.75);  glVertex3f(P6.x, P6.y, P6.z );
						  glTexCoord2f(0.25,0.75); glVertex3f(P4.x, P4.y, P4.z );
						  glTexCoord2f(0.50,0.75);  glVertex3f(P6.x, P6.y, P6.z );
						  glTexCoord2f(0.25,0.50); glVertex3f(P5.x, P5.y, P5.z );
						glEnd();

						Stat_FaceTop++;
				  }
				  else
				  {
                Stat_RenderDrawFaces++;
                Stat_FaceRight++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                glEnd();

                Stat_FaceTop++;
				  }
			  }
#if 1
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_LEFT|DRAWFACE_BELOW ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P1.x, P1.y, P1.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_RIGHT|DRAWFACE_BELOW ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P5.x, P5.y, P5.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_AHEAD|DRAWFACE_BELOW ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceFront++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_BEHIND|DRAWFACE_BELOW ) )
			  {
                Stat_RenderDrawFaces++;
				Stat_FaceBack++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P1.x, P1.y, P1.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P6.x, P6.y, P6.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_LEFT|DRAWFACE_AHEAD ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
				glTRI_except( 1, 0, 3, 7, 4, 1, 3, 7, 5 );
                glEnd();
                Stat_FaceTop++;

			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_RIGHT|DRAWFACE_AHEAD ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
				glTRI_except( 2, 6, 2, 1, 5, 6, 2, 0, 4 );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_LEFT|DRAWFACE_BEHIND ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
				glTRI_except( 1, 0, 3, 7, 4, 0, 2, 6, 4 );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_RIGHT|DRAWFACE_BEHIND ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
				glTRI_except( 2, 6, 2, 1, 5, 7, 3, 1, 5 );
                glEnd();

                Stat_FaceTop++;

	}
#endif
    else 
	{
	default_draw:
				//Left
				if (info & DRAWFACE_LEFT)
				{
				  Stat_RenderDrawFaces++;
				  Stat_FaceLeft++;
				  glBegin(GL_TRIANGLES);
					glTRI_normal( 1,0,3,7,4 );
				  glEnd();
				}

				// Right
				if (info & DRAWFACE_RIGHT)
				{
				  Stat_RenderDrawFaces++;
				  Stat_FaceRight++;
				  glBegin(GL_TRIANGLES);
					glTRI_normal( 2,6,2,1,5 );
				  glEnd();
				}

				//Front
				if (info & DRAWFACE_AHEAD)
				{
				  Stat_RenderDrawFaces++;
				  Stat_FaceFront++;
				  glBegin(GL_TRIANGLES);
					glTRI_normal( 0,4,5,1,0 );
				  glEnd();
				}

				//Back
				if (info & DRAWFACE_BEHIND)
				{
				  Stat_RenderDrawFaces++;
				  Stat_FaceBack++;
				  glBegin(GL_TRIANGLES);
					glTRI_normal( 3,2,6,7,3 );
				  glEnd();
				}

				// Top
				if (info & DRAWFACE_ABOVE)
				{
					if( ( ( info & ( DRAWFACE_LEFT_HAS_ABOVE | DRAWFACE_RIGHT_HAS_ABOVE | DRAWFACE_AHEAD_HAS_ABOVE | DRAWFACE_BEHIND_HAS_ABOVE ) )
						== ( DRAWFACE_LEFT_HAS_ABOVE | DRAWFACE_RIGHT_HAS_ABOVE | DRAWFACE_AHEAD_HAS_ABOVE | DRAWFACE_BEHIND_HAS_ABOVE )  )
					   || ( ( info & ( DRAWFACE_LEFT | DRAWFACE_RIGHT | DRAWFACE_AHEAD | DRAWFACE_BEHIND ) )
						== ( DRAWFACE_LEFT | DRAWFACE_RIGHT | DRAWFACE_AHEAD | DRAWFACE_BEHIND ) )  )
					{
						goto normal_flat_top;
					}
					else if( !( info & ( DRAWFACE_BEHIND_HAS_ABOVE | DRAWFACE_LEFT_HAS_BEHIND | DRAWFACE_LEFT_HAS_ABOVE ) )
						&& ( info & ( DRAWFACE_LEFT_HAS_AHEAD | DRAWFACE_RIGHT_HAS_AHEAD | DRAWFACE_RIGHT_HAS_BEHIND ) ) 
						== ( DRAWFACE_LEFT_HAS_AHEAD | DRAWFACE_RIGHT_HAS_AHEAD | DRAWFACE_RIGHT_HAS_BEHIND ) )
					{
						goto top_right_back_diagonal;
					}
					else if( !(info & DRAWFACE_LEFT_HAS_BEHIND ) )
					{
						top_right_back_diagonal:
						  Stat_RenderDrawFaces++;
						  Stat_FaceTop++;
						  glBegin(GL_TRIANGLES);
							glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
							glTexCoord2f(0.50,0.25);  glVertex3f(P6.x, P6.y, P6.z );
							glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
							glTexCoord2f(0.25,0.50);  glVertex3f(P4.x, P4.y, P4.z );
							glTexCoord2f(0.50,0.25); glVertex3f(P3.x, P3.y, P3.z );
							glTexCoord2f(0.50,0.50); glVertex3f(P6.x, P6.y, P6.z );
						  glEnd();
					}
					else if( !(info & DRAWFACE_RIGHT_HAS_BEHIND ) )
					{
						  Stat_RenderDrawFaces++;
						  Stat_FaceTop++;
						  glBegin(GL_TRIANGLES);
							glTexCoord2f(0.50,0.25);  glVertex3f(P7.x, P7.y, P7.z );
							glTexCoord2f(0.25,0.25); glVertex3f(P2.x, P2.y, P2.z );
							glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
							glTexCoord2f(0.50,0.25); glVertex3f(P4.x, P4.y, P4.z );
							glTexCoord2f(0.25,0.50);  glVertex3f(P7.x, P7.y, P7.z );
							glTexCoord2f(0.50,0.50); glVertex3f(P5.x, P5.y, P5.z );
						  glEnd();
					}
					else if( !(info & DRAWFACE_LEFT_HAS_AHEAD ) )
					{
						  Stat_RenderDrawFaces++;
						  Stat_FaceTop++;
						  glBegin(GL_TRIANGLES);
							glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
							glTexCoord2f(0.50,0.25);  glVertex3f(P7.x, P7.y, P7.z );
							glTexCoord2f(0.25,0.25); glVertex3f(P6.x, P6.y, P6.z );
							glTexCoord2f(0.50,0.25); glVertex3f(P0.x, P0.y, P0.z );
							glTexCoord2f(0.25,0.50);  glVertex3f(P7.x, P7.y, P7.z );
							glTexCoord2f(0.50,0.50); glVertex3f(P5.x, P5.y, P5.z );
						  glEnd();
					}
					else if( !(info & DRAWFACE_RIGHT_HAS_AHEAD ) )
					{
						  Stat_RenderDrawFaces++;
						  Stat_FaceTop++;
						  glBegin(GL_TRIANGLES);
							glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
							glTexCoord2f(0.50,0.25);  glVertex3f(P7.x, P7.y, P7.z );
							glTexCoord2f(0.25,0.50);  glVertex3f(P6.x, P6.y, P6.z );
							glTexCoord2f(0.25,0.50);  glVertex3f(P1.x, P1.y, P1.z );
							glTexCoord2f(0.50,0.50); glVertex3f(P4.x, P4.y, P4.z );
							glTexCoord2f(0.50,0.25); glVertex3f(P6.x, P6.y, P6.z );
						  glEnd();
					}
					else
					{
						normal_flat_top:
					  Stat_RenderDrawFaces++;
					  Stat_FaceTop++;
					  glBegin(GL_TRIANGLES);
						glTRI_normal( 4,4,7,6,5 );
					  glEnd();
					}
				}

			   // Bottom
			   if (info & DRAWFACE_BELOW)
			   {
				 Stat_RenderDrawFaces++;
				 Stat_FaceBottom++;
				 glBegin(GL_TRIANGLES);
					glTRI_normal( 5,0,1,2,3 );
				 glEnd();
			}
   }
}



void ZRender_Interface::MakeSectorRenderingData(ZVoxelSector * Sector)
{
  Long x,y,z;
  ULong info;
  UShort cube, prevcube;

  ULong Offset;
  Long Sector_Display_x, Sector_Display_y, Sector_Display_z;
  ZRender_Interface_displaydata * DisplayData;
  ULong Pass;
  Bool Draw;
  ZVoxelType ** VoxelTypeTable = VoxelTypeManager->VoxelTable;
  ZVector3f P0,P1,P2,P3,P4,P5,P6,P7;


  // Display list creation or reuse.

  if (Sector->DisplayData == 0) { Sector->DisplayData = new ZRender_Interface_displaydata; }
  DisplayData = (ZRender_Interface_displaydata *)Sector->DisplayData;
  if ( DisplayData->DisplayList_Regular == 0 )    DisplayData->DisplayList_Regular = glGenLists(1);
  if ( DisplayData->DisplayList_Transparent == 0) DisplayData->DisplayList_Transparent = glGenLists(1);

  if (Sector->Flag_Render_Dirty || 1 )
  {
    Sector_Display_x = Sector->Pos_x * Sector->Size_x * GlobalSettings.VoxelBlockSize;
    Sector_Display_y = Sector->Pos_y * Sector->Size_y * GlobalSettings.VoxelBlockSize;
    Sector_Display_z = Sector->Pos_z * Sector->Size_z * GlobalSettings.VoxelBlockSize;

    Sector->Flag_Void_Regular = true;
    Sector->Flag_Void_Transparent = true;

    for (Pass=0; Pass<2; Pass++)
    {
      switch(Pass)
      {
        case 0: glNewList(DisplayData->DisplayList_Regular, GL_COMPILE); break;
        case 1: glNewList(DisplayData->DisplayList_Transparent, GL_COMPILE); break;
      }
      prevcube = 0;
      for ( z=0 ; z < Sector->Size_z ; z++ )
      {
        for ( x=0 ; x < Sector->Size_x ; x++ )
        {
          for ( y=0 ; y < Sector->Size_y ; y++ )
          {
            Offset = y + ( x*Sector->Size_y )+ (z * (Sector->Size_y*Sector->Size_x));
            cube = Sector->Data[Offset];
            info = Sector->FaceCulling[Offset];


            if (cube>0 && info != DRAWFACE_NONE)
            {
              switch(Pass)
              {
                case 0: if ( VoxelTypeTable[cube]->Draw_TransparentRendering ) { Draw = false;  Sector->Flag_Void_Transparent = false; }
                        else                                                   { Draw = true;   Sector->Flag_Void_Regular     = false; }
                        break;
                case 1: Draw = ( VoxelTypeTable[cube]->Draw_TransparentRendering ) ? true:false; break;
              }
            } else Draw = false;

            if (Draw)
            {
				EmitFaces( VoxelTypeTable, cube, prevcube, info, x, y, z, Sector_Display_x, Sector_Display_y, Sector_Display_z );
#if 0
              // glTexEnvf(0x8500 /* TEXTURE_FILTER_CONTROL_EXT */, 0x8501 /* TEXTURE_LOD_BIAS_EXT */,VoxelTypeManager->VoxelTable[cube]->TextureLodBias);
              if (cube != prevcube) glBindTexture(GL_TEXTURE_2D, VoxelTypeManager->VoxelTable[cube]->OpenGl_TextureRef);
              prevcube = cube;
              cubx = (float)(x*GlobalSettings.VoxelBlockSize + Sector_Display_x);
              cuby = (float)(y*GlobalSettings.VoxelBlockSize + Sector_Display_y);
              cubz = (float)(z*GlobalSettings.VoxelBlockSize + Sector_Display_z);

              if (VoxelTypeTable[cube]->DrawInfo & ZVOXEL_DRAWINFO_SPECIALRENDERING ) 
			  {
				  VoxelTypeTable[cube]->SpecialRender(cubx,cuby,cubz); 
				  continue; 
			  }

              P0.x = cubx;           P0.y = cuby;          P0.z = cubz;
              P1.x = cubx + GlobalSettings.VoxelBlockSize;  P1.y = cuby;          P1.z = cubz;
              P2.x = cubx + GlobalSettings.VoxelBlockSize;  P2.y = cuby;          P2.z = cubz+GlobalSettings.VoxelBlockSize;
              P3.x = cubx;           P3.y = cuby;          P3.z = cubz+GlobalSettings.VoxelBlockSize;
              P4.x = cubx;           P4.y = cuby + GlobalSettings.VoxelBlockSize; P4.z = cubz;
              P5.x = cubx + GlobalSettings.VoxelBlockSize;  P5.y = cuby + GlobalSettings.VoxelBlockSize; P5.z = cubz;
              P6.x = cubx + GlobalSettings.VoxelBlockSize;  P6.y = cuby + GlobalSettings.VoxelBlockSize; P6.z = cubz + GlobalSettings.VoxelBlockSize;
              P7.x = cubx;           P7.y = cuby + GlobalSettings.VoxelBlockSize; P7.z = cubz + GlobalSettings.VoxelBlockSize;

              //Left
			  if( info == (DRAWFACE_LEFT|DRAWFACE_ABOVE ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.25); glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.25); glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.25,0.25); glVertex3f(P5.x, P5.y, P5.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( info == (DRAWFACE_RIGHT|DRAWFACE_ABOVE ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P1.x, P1.y, P1.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                glEnd();

                Stat_FaceTop++;

			  }
			  else if( info == (DRAWFACE_AHEAD|DRAWFACE_ABOVE ) )
			  {
                Stat_RenderDrawFaces++;
				Stat_FaceFront++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.25); glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.25,0.0);  glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.50,0.0);  glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.50,0.0);  glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.50,0.25); glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.25,0.25); glVertex3f(P1.x, P1.y, P1.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( info == (DRAWFACE_BEHIND|DRAWFACE_ABOVE ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceRight++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( info == (DRAWFACE_LEFT|DRAWFACE_BELOW ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P1.x, P1.y, P1.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( info == (DRAWFACE_RIGHT|DRAWFACE_BELOW ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P5.x, P5.y, P5.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( info == (DRAWFACE_AHEAD|DRAWFACE_BELOW ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceFront++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( info == (DRAWFACE_BEHIND|DRAWFACE_BELOW ) )
			  {
                Stat_RenderDrawFaces++;
				Stat_FaceBack++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P1.x, P1.y, P1.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P6.x, P6.y, P6.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else 

			  {
				  if (info & DRAWFACE_LEFT)
              {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.25); glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
                glEnd();
              }

              // Right
              if (info & DRAWFACE_RIGHT)
              {
                Stat_RenderDrawFaces++;
                Stat_FaceRight++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P1.x, P1.y, P1.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P5.x, P5.y, P5.z );
                glEnd();
              }

              //Front
              if (info & DRAWFACE_AHEAD)
              {
                Stat_RenderDrawFaces++;
                Stat_FaceFront++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.0,0.25); glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.25,0.25);  glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.0,0.50); glVertex3f(P1.x, P1.y, P1.z );
                  glTexCoord2f(0.0,0.25); glVertex3f(P0.x, P0.y, P0.z );
                glEnd();
              }

              //Back
              if (info & DRAWFACE_BEHIND)
              {
                Stat_RenderDrawFaces++;
                Stat_FaceBack++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.75,0.50); glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.75,0.25);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.75,0.25);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.50); glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.50,0.25); glVertex3f(P7.x, P7.y, P7.z );
                glEnd();
              }

              // Top
              if (info & DRAWFACE_ABOVE)
              {
                Stat_RenderDrawFaces++;
                Stat_FaceTop++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.50,0.25);  glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.50,0.25); glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.50,0.50); glVertex3f(P6.x, P6.y, P6.z );
                glEnd();
              }

             // Bottom
             if (info & DRAWFACE_BELOW)
             {
               Stat_RenderDrawFaces++;
               Stat_FaceBottom++;
               glBegin(GL_TRIANGLES);
                 glTexCoord2f(1.0,0.25); glVertex3f(P0.x, P0.y, P0.z );
                 glTexCoord2f(1.0,0.50);  glVertex3f(P1.x, P1.y, P1.z );
                 glTexCoord2f(0.75,0.25);  glVertex3f(P3.x, P3.y, P3.z );
                 glTexCoord2f(0.75,0.25);  glVertex3f(P3.x, P3.y, P3.z );
                 glTexCoord2f(1.0,0.50); glVertex3f(P1.x, P1.y, P1.z );
                 glTexCoord2f(0.75,0.50); glVertex3f(P2.x, P2.y, P2.z );
               glEnd();
              }
            }
#endif
			}
          }
        }
      }
      glEndList();

      // if in the first pass, the sector has no transparent block, the second pass is cancelled.

      if (Sector->Flag_Void_Transparent) break;
    }
    Sector->Flag_Render_Dirty = false;
  }

}























void ZRender_Interface::MakeSectorRenderingData_Sorted(ZVoxelSector * Sector)
{
  Long x,y,z;
  ULong info, i;
  UShort VoxelType, prevVoxelType;

  // ULong Offset;
  float cubx, cuby, cubz;
  Long Sector_Display_x, Sector_Display_y, Sector_Display_z;
  ZRender_Interface_displaydata * DisplayData;
  ULong Pass;
  ZVoxelType ** VoxelTypeTable = VoxelTypeManager->VoxelTable;
  ZVector3f P0,P1,P2,P3,P4,P5,P6,P7;

  ZRender_Sorter::RenderBucket * RenderBucket;

  // Set flags

  Sector->Flag_Void_Regular = true;
  Sector->Flag_Void_Transparent = true;
  Sector->Flag_Render_Dirty = false;

  // Render sorter action

  RenderSorter.ProcessSector(Sector);
  if (!RenderSorter.GetBucketCount()) return;

  // Check what blocktypes

  RenderSorter.Rendering_Start();
  for (i=0;i<RenderSorter.GetBucketCount();i++)
  {
    VoxelType = RenderSorter.Rendering_GetNewBucket()->VoxelType;

    if (VoxelTypeTable[VoxelType]->Draw_TransparentRendering) Sector->Flag_Void_Transparent = false;
    else                                                      Sector->Flag_Void_Regular     = false;
  }

  // Display list creation or reuse.

  if (Sector->DisplayData == 0) { Sector->DisplayData = new ZRender_Interface_displaydata; }
  DisplayData = (ZRender_Interface_displaydata *)Sector->DisplayData;
  if ( (!Sector->Flag_Void_Regular)     && (DisplayData->DisplayList_Regular == 0) )    DisplayData->DisplayList_Regular = glGenLists(1);
  if ( (!Sector->Flag_Void_Transparent) && (DisplayData->DisplayList_Transparent == 0) ) DisplayData->DisplayList_Transparent = glGenLists(1);

  // Computing Sector Display coordinates;

  Sector_Display_x = (Sector->Pos_x * Sector->Size_x) << GlobalSettings.VoxelBlockSizeBits;
  Sector_Display_y = (Sector->Pos_y * Sector->Size_y) << GlobalSettings.VoxelBlockSizeBits;
  Sector_Display_z = (Sector->Pos_z * Sector->Size_z) << GlobalSettings.VoxelBlockSizeBits;

  for (Pass=0; Pass<2; Pass++)
  {
    if (!Pass) { if (Sector->Flag_Void_Regular)     continue; glNewList(DisplayData->DisplayList_Regular, GL_COMPILE); }
    else       { if (Sector->Flag_Void_Transparent) continue; glNewList(DisplayData->DisplayList_Transparent, GL_COMPILE); }

    prevVoxelType = 0;

    RenderSorter.Rendering_Start();

    while (( RenderBucket = RenderSorter.Rendering_GetNewBucket() ))
    {
      VoxelType = RenderBucket->VoxelType;

      // Is it the right voxel transparency type for that rendering pass ?

      if ( (Pass>0) != VoxelTypeTable[VoxelType]->Draw_TransparentRendering) continue;

      // Render one RenderBucket.

      for (i=0;i<RenderBucket->VoxelCount;i++)
      {
		  register uint64_t Pck;

        // Gettint Voxel Informations from the table.

        Pck = RenderBucket->RenderTable[i].PackedInfos;

        // Unpacking voxel infos

        info = Pck & ( ( 1 << (64 - ( ZVOXELBLOCSHIFT_X+ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_Z ) ) ) - 1 );
        z    = (Pck >> (64-(ZVOXELBLOCSHIFT_X+ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_Z))) & ZVOXELBLOCMASK_Z;
        y    = (Pck >> (64-(ZVOXELBLOCSHIFT_X+ZVOXELBLOCSHIFT_Y))) & ZVOXELBLOCMASK_Y;
        x    = (Pck >> (64-(ZVOXELBLOCSHIFT_X))) & ZVOXELBLOCMASK_X;

        // Offset = y + ( x << ZVOXELBLOCSHIFT_Y )+ (z << (ZVOXELBLOCSHIFT_Y + ZVOXELBLOCSHIFT_X));
		EmitFaces( VoxelTypeTable, VoxelType, prevVoxelType, info, x, y, z, Sector_Display_x, Sector_Display_y, Sector_Display_z );
#if 0
        // glTexEnvf(0x8500 /* TEXTURE_FILTER_CONTROL_EXT */, 0x8501 /* TEXTURE_LOD_BIAS_EXT */,VoxelTypeManager->VoxelTable[VoxelType]->TextureLodBias);
        if (VoxelType != prevVoxelType) glBindTexture(GL_TEXTURE_2D, VoxelTypeManager->VoxelTable[VoxelType]->OpenGl_TextureRef);
        prevVoxelType = VoxelType;
        cubx = (float)(x*GlobalSettings.VoxelBlockSize + Sector_Display_x);
        cuby = (float)(y*GlobalSettings.VoxelBlockSize + Sector_Display_y);
        cubz = (float)(z*GlobalSettings.VoxelBlockSize + Sector_Display_z);

        if (VoxelTypeTable[VoxelType]->DrawInfo & ZVOXEL_DRAWINFO_SPECIALRENDERING ) {VoxelTypeTable[VoxelType]->SpecialRender(cubx,cuby,cubz); continue; }

        P0.x = cubx;           P0.y = cuby;          P0.z = cubz;
        P1.x = cubx + GlobalSettings.VoxelBlockSize;  P1.y = cuby;          P1.z = cubz;
        P2.x = cubx + GlobalSettings.VoxelBlockSize;  P2.y = cuby;          P2.z = cubz+GlobalSettings.VoxelBlockSize;
        P3.x = cubx;           P3.y = cuby;          P3.z = cubz+GlobalSettings.VoxelBlockSize;
        P4.x = cubx;           P4.y = cuby + GlobalSettings.VoxelBlockSize; P4.z = cubz;
        P5.x = cubx + GlobalSettings.VoxelBlockSize;  P5.y = cuby + GlobalSettings.VoxelBlockSize; P5.z = cubz;
        P6.x = cubx + GlobalSettings.VoxelBlockSize;  P6.y = cuby + GlobalSettings.VoxelBlockSize; P6.z = cubz + GlobalSettings.VoxelBlockSize;
        P7.x = cubx;           P7.y = cuby + GlobalSettings.VoxelBlockSize; P7.z = cubz + GlobalSettings.VoxelBlockSize;


		// if it's otherwise entirely covered...
	  if( !( info & ( DRAWFACE_ALL ) )
		 &&( !( info & ( DRAWFACE_BEHIND_HAS_ABOVE | DRAWFACE_LEFT_HAS_ABOVE ) )
		 || !( info & ( DRAWFACE_AHEAD_HAS_ABOVE | DRAWFACE_LEFT_HAS_ABOVE ) )
		 || !( info & ( DRAWFACE_BEHIND_HAS_ABOVE | DRAWFACE_RIGHT_HAS_ABOVE ) )
		 || !( info & ( DRAWFACE_AHEAD_HAS_ABOVE | DRAWFACE_RIGHT_HAS_ABOVE ) ) )
		  )
	  {
		  info |= DRAWFACE_ABOVE;
	  }


			  if( ( info & DRAWFACE_ALL ) == (DRAWFACE_LEFT|DRAWFACE_ABOVE|DRAWFACE_AHEAD ) )
			  {
				  if( info & ( DRAWFACE_BELOW_HAS_LEFT | DRAWFACE_BELOW_HAS_AHEAD ) )
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P3.x, P3.y, P3.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P6.x, P6.y, P6.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						  glTexCoord2f(0.25,0.25); glVertex3f(P0.x, P0.y, P0.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P6.x, P6.y, P6.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P1.x, P1.y, P1.z );
						glEnd();

						Stat_FaceTop++;
				  }
				  else
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P3.x, P3.y, P3.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P6.x, P6.y, P6.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P1.x, P1.y, P1.z );
						glEnd();

						Stat_FaceTop++;
				  }
			  }
			  else if( ( info & DRAWFACE_ALL ) == (DRAWFACE_LEFT|DRAWFACE_BELOW|DRAWFACE_AHEAD ) )
			  {
				  if( info & ( DRAWFACE_ABOVE_HAS_LEFT | DRAWFACE_ABOVE_HAS_AHEAD ) )
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P7.x, P7.y, P7.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P2.x, P1.y, P2.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P6.y, P5.z );
						  glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P2.x, P1.y, P2.z );
						glEnd();

						Stat_FaceTop++;
				  }
				  else
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P7.x, P7.y, P7.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P6.y, P5.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P2.x, P1.y, P2.z );
						glEnd();

						Stat_FaceTop++;
				  }
			  }
			  else if( ( info & DRAWFACE_ALL ) == (DRAWFACE_RIGHT|DRAWFACE_ABOVE|DRAWFACE_AHEAD ) )
			  {
				  if( info & ( DRAWFACE_BELOW_HAS_RIGHT | DRAWFACE_BELOW_HAS_AHEAD ) )
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P7.x, P7.y, P7.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P1.x, P1.y, P1.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						  glTexCoord2f(0.25,0.25); glVertex3f(P7.x, P7.y, P7.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P2.x, P2.y, P2.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P1.x, P1.y, P1.z );
						glEnd();

						Stat_FaceTop++;
				  }
				  else
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P7.x, P7.y, P7.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P2.x, P2.y, P2.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glEnd();

						Stat_FaceTop++;
				  }
			  }
			  else if( ( info & DRAWFACE_ALL ) == (DRAWFACE_RIGHT|DRAWFACE_BELOW|DRAWFACE_AHEAD ) )
			  {
				  if( info & ( DRAWFACE_ABOVE_HAS_LEFT | DRAWFACE_ABOVE_HAS_AHEAD ) )
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P7.x, P7.y, P7.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P2.x, P1.y, P2.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P6.y, P5.z );
						  glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P2.x, P1.y, P2.z );
						glEnd();

						Stat_FaceTop++;
				  }
				  else
				  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						  glTexCoord2f(0.25,0.25); glVertex3f(P6.x, P6.y, P6.z );
						glEnd();

						Stat_FaceTop++;
				  }
			  }
			  else if( ( info & DRAWFACE_ALL ) == (DRAWFACE_LEFT|DRAWFACE_ABOVE|DRAWFACE_BEHIND ) )
			  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P0.x, P0.y, P0.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P2.x, P2.y, P2.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P5.x, P5.y, P5.z );
						glEnd();

						Stat_FaceTop++;
			  }
			  else if( ( info & DRAWFACE_ALL ) == (DRAWFACE_LEFT|DRAWFACE_BELOW|DRAWFACE_BEHIND ) )
			  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P1.x, P1.y, P1.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P6.x, P6.y, P6.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						glEnd();

						Stat_FaceTop++;
			  }
			  else if( ( info & DRAWFACE_ALL ) == (DRAWFACE_RIGHT|DRAWFACE_ABOVE|DRAWFACE_BEHIND ) )
			  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P1.x, P1.y, P1.z );
						glEnd();

						Stat_FaceTop++;
			  }
			  else if( ( info & DRAWFACE_ALL ) == (DRAWFACE_RIGHT|DRAWFACE_BELOW|DRAWFACE_BEHIND ) )
			  {
						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P5.y, P5.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P7.x, P7.y, P7.z );
						  glTexCoord2f(0.25,0.25); glVertex3f(P0.x, P0.y, P0.z );
						glEnd();

						Stat_FaceTop++;
			  }
			  else if( ( info & DRAWFACE_ALL ) == (DRAWFACE_LEFT|DRAWFACE_ABOVE ) )
			  {
				  if( ( ( info & (DRAWFACE_AHEAD_HAS_ABOVE|DRAWFACE_AHEAD_HAS_LEFT) ) )
				     && ( ( info & ( DRAWFACE_BEHIND_HAS_ABOVE|DRAWFACE_BEHIND_HAS_LEFT) ) ) )
				  {
					  goto default_draw;
				  }
				  else if( ( info & (DRAWFACE_AHEAD_HAS_ABOVE|DRAWFACE_AHEAD_HAS_LEFT) ) )
				  {
					glBegin(GL_TRIANGLES);
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.50,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P5.y, P5.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.50,0.0);  glVertex3f(P6.x, P6.y, P6.z );
						glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P5.y, P5.z );

						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						glTexCoord2f(0.25,0.25); glVertex3f(P6.x, P6.y, P6.z );
					glEnd();
				  }
				  else if( ( info & (DRAWFACE_BEHIND_HAS_ABOVE|DRAWFACE_BEHIND_HAS_LEFT) ) )
				  {
					glBegin(GL_TRIANGLES);
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.50,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P5.y, P5.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P4.x, P4.y, P4.z );
						glTexCoord2f(0.25,0.25); glVertex3f((P0.x+P6.x)/2, (P0.y+P6.y)/2, (P0.z+P6.z)/2 );
						glTexCoord2f(0.50,0.0);  glVertex3f(P6.x, P6.y, P6.z );
						glTexCoord2f(0.25,0.0);  glVertex3f(P5.x, P5.y, P5.z );

						glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						glTexCoord2f(0.25,0.25); glVertex3f(P6.x, P6.y, P6.z );
					glEnd();
				  }
				  else 
				  {

						Stat_RenderDrawFaces++;
						Stat_FaceLeft++;
						glBegin(GL_TRIANGLES);
						  glTexCoord2f(0.25,0.25); glVertex3f(P5.x, P5.y, P5.z );
						  glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						  glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
						  glTexCoord2f(0.50,0.25); glVertex3f(P6.x, P6.y, P6.z );
						  glTexCoord2f(0.25,0.25); glVertex3f(P5.x, P5.y, P5.z );
						glEnd();

						Stat_FaceTop++;
				  }
			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_RIGHT|DRAWFACE_ABOVE ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P1.x, P1.y, P1.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                glEnd();

                Stat_FaceTop++;

			  }
			  else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_AHEAD|DRAWFACE_ABOVE ) )
			  {
                Stat_RenderDrawFaces++;
				Stat_FaceFront++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.25); glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.25,0.0);  glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.50,0.0);  glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.50,0.0);  glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.50,0.25); glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.25,0.25); glVertex3f(P1.x, P1.y, P1.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_BEHIND|DRAWFACE_ABOVE ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceRight++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_LEFT|DRAWFACE_BELOW ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P1.x, P1.y, P1.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_RIGHT|DRAWFACE_BELOW ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P5.x, P5.y, P5.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_AHEAD|DRAWFACE_BELOW ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceFront++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_BEHIND|DRAWFACE_BELOW ) )
			  {
                Stat_RenderDrawFaces++;
				Stat_FaceBack++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P1.x, P1.y, P1.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P6.x, P6.y, P6.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_LEFT|DRAWFACE_AHEAD ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.75); glVertex3f(P1.x, P1.y, P1.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P1.x, P1.y, P1.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P5.x, P5.y, P5.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_RIGHT|DRAWFACE_AHEAD ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P6.x, P6.y, P6.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_LEFT|DRAWFACE_BEHIND ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P4.x, P4.y, P4.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else if( ( info & DRAWFACE_ALL )  == (DRAWFACE_RIGHT|DRAWFACE_BEHIND ) )
			  {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.50,0.50);  glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P1.x, P1.y, P1.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P1.x, P1.y, P1.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P5.x, P5.y, P5.z );
                glEnd();

                Stat_FaceTop++;

			  }
              else 
			  {
	default_draw:
        //Left
        if (info & DRAWFACE_LEFT)
        {
          Stat_RenderDrawFaces++;
          Stat_FaceLeft++;
          glBegin(GL_TRIANGLES);
            glTRI_normal( 1,0,3,7,4 );
            //glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
            //glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
            //glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
            //glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
            //glTexCoord2f(0.50,0.25); glVertex3f(P7.x, P7.y, P7.z );
            //glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
          glEnd();
        }

        // Right
        if (info & DRAWFACE_RIGHT)
        {
          Stat_RenderDrawFaces++;
          Stat_FaceRight++;
          glBegin(GL_TRIANGLES);
            glTRI_normal( 2,6,2,1,5 );
            //glTexCoord2f(0.50,0.50);  glVertex3f(P6.x, P6.y, P6.z );
            //glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
            //glTexCoord2f(0.25,0.50); glVertex3f(P5.x, P5.y, P5.z );
            //glTexCoord2f(0.25,0.50); glVertex3f(P5.x, P5.y, P5.z );
            //glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
            //glTexCoord2f(0.25,0.75); glVertex3f(P1.x, P1.y, P1.z );
          glEnd();
        }

        //Front
        if (info & DRAWFACE_AHEAD)
        {
          Stat_RenderDrawFaces++;
          Stat_FaceFront++;
          glBegin(GL_TRIANGLES);
            glTRI_normal( 0,4,5,1,0 );
            //glTexCoord2f(0.25,0.25);  glVertex3f(P4.x, P4.y, P4.z );
            //glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
            //glTexCoord2f(0.0,0.25); glVertex3f(P0.x, P0.y, P0.z );
            //glTexCoord2f(0.0,0.25); glVertex3f(P0.x, P0.y, P0.z );
            //glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
            //glTexCoord2f(0.0,0.50); glVertex3f(P1.x, P1.y, P1.z );
          glEnd();
        }

        //Back
        if (info & DRAWFACE_BEHIND)
        {
          Stat_RenderDrawFaces++;
          Stat_FaceBack++;
          glBegin(GL_TRIANGLES);
            glTRI_normal( 3,2,6,7,3 );
            //glTexCoord2f(0.75,0.50); glVertex3f(P2.x, P2.y, P2.z );
            //glTexCoord2f(0.50,0.50);  glVertex3f(P6.x, P6.y, P6.z );
            //glTexCoord2f(0.75,0.25);  glVertex3f(P3.x, P3.y, P3.z );
            //glTexCoord2f(0.75,0.25);  glVertex3f(P3.x, P3.y, P3.z );
            //glTexCoord2f(0.50,0.50); glVertex3f(P6.x, P6.y, P6.z );
            //glTexCoord2f(0.50,0.25); glVertex3f(P7.x, P7.y, P7.z );
          glEnd();
        }

        // Top
        if (info & DRAWFACE_ABOVE)
        {
			if( ( ( info & ( DRAWFACE_LEFT_HAS_ABOVE | DRAWFACE_RIGHT_HAS_ABOVE | DRAWFACE_AHEAD_HAS_ABOVE | DRAWFACE_BEHIND_HAS_ABOVE ) )
				== ( DRAWFACE_LEFT_HAS_ABOVE | DRAWFACE_RIGHT_HAS_ABOVE | DRAWFACE_AHEAD_HAS_ABOVE | DRAWFACE_BEHIND_HAS_ABOVE )  )
			   || ( ( info & ( DRAWFACE_LEFT | DRAWFACE_RIGHT | DRAWFACE_AHEAD | DRAWFACE_BEHIND ) )
				== ( DRAWFACE_LEFT | DRAWFACE_RIGHT | DRAWFACE_AHEAD | DRAWFACE_BEHIND ) )  )
			{
				goto normal_flat_top;
			}
			else if( !( info & DRAWFACE_LEFT_HAS_BEHIND )
				&& ( info & ( DRAWFACE_LEFT_HAS_AHEAD | DRAWFACE_RIGHT_HAS_AHEAD | DRAWFACE_RIGHT_HAS_BEHIND ) ) 
				== ( DRAWFACE_LEFT_HAS_AHEAD | DRAWFACE_RIGHT_HAS_AHEAD | DRAWFACE_RIGHT_HAS_BEHIND ) )
			{
				goto top_right_back_diagonal;
			}
			else if( !(info & DRAWFACE_LEFT_HAS_BEHIND ) )
			{
				top_right_back_diagonal:
				  Stat_RenderDrawFaces++;
				  Stat_FaceTop++;
				  glBegin(GL_TRIANGLES);
					glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
					glTexCoord2f(0.50,0.25);  glVertex3f(P6.x, P6.y, P6.z );
					glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
					glTexCoord2f(0.25,0.50);  glVertex3f(P4.x, P4.y, P4.z );
					glTexCoord2f(0.50,0.25); glVertex3f(P3.x, P3.y, P3.z );
					glTexCoord2f(0.50,0.50); glVertex3f(P6.x, P6.y, P6.z );
				  glEnd();
			}
			else if( !(info & DRAWFACE_RIGHT_HAS_BEHIND ) )
			{
				  Stat_RenderDrawFaces++;
				  Stat_FaceTop++;
				  glBegin(GL_TRIANGLES);
					glTexCoord2f(0.50,0.25);  glVertex3f(P7.x, P7.y, P7.z );
					glTexCoord2f(0.25,0.25); glVertex3f(P2.x, P2.y, P2.z );
					glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
					glTexCoord2f(0.50,0.25); glVertex3f(P4.x, P4.y, P4.z );
					glTexCoord2f(0.25,0.50);  glVertex3f(P7.x, P7.y, P7.z );
					glTexCoord2f(0.50,0.50); glVertex3f(P5.x, P5.y, P5.z );
				  glEnd();
			}
			else if( !(info & DRAWFACE_LEFT_HAS_AHEAD ) )
			{
				  Stat_RenderDrawFaces++;
				  Stat_FaceTop++;
				  glBegin(GL_TRIANGLES);
					glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
					glTexCoord2f(0.50,0.25);  glVertex3f(P7.x, P7.y, P7.z );
					glTexCoord2f(0.25,0.25); glVertex3f(P6.x, P6.y, P6.z );
					glTexCoord2f(0.50,0.25); glVertex3f(P0.x, P0.y, P0.z );
					glTexCoord2f(0.25,0.50);  glVertex3f(P7.x, P7.y, P7.z );
					glTexCoord2f(0.50,0.50); glVertex3f(P5.x, P5.y, P5.z );
				  glEnd();
			}
			else if( !(info & DRAWFACE_RIGHT_HAS_AHEAD ) )
			{
				  Stat_RenderDrawFaces++;
				  Stat_FaceTop++;
				  glBegin(GL_TRIANGLES);
					glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
					glTexCoord2f(0.50,0.25);  glVertex3f(P7.x, P7.y, P7.z );
					glTexCoord2f(0.25,0.50);  glVertex3f(P6.x, P6.y, P6.z );
					glTexCoord2f(0.25,0.50);  glVertex3f(P1.x, P1.y, P1.z );
					glTexCoord2f(0.50,0.50); glVertex3f(P4.x, P4.y, P4.z );
					glTexCoord2f(0.50,0.25); glVertex3f(P6.x, P6.y, P6.z );
				  glEnd();
			}
			else
			{
				normal_flat_top:
          Stat_RenderDrawFaces++;
          Stat_FaceTop++;
          glBegin(GL_TRIANGLES);
			glTRI_normal( 4,4,7,6,5 );
            //glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
            //glTexCoord2f(0.50,0.25);  glVertex3f(P7.x, P7.y, P7.z );
            //glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
            //glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
            //glTexCoord2f(0.50,0.25); glVertex3f(P7.x, P7.y, P7.z );
            //glTexCoord2f(0.50,0.50); glVertex3f(P6.x, P6.y, P6.z );
          glEnd();
			}
        }

       // Bottom
       if (info & DRAWFACE_BELOW)
       {
         Stat_RenderDrawFaces++;
         Stat_FaceBottom++;
         glBegin(GL_TRIANGLES);
			glTRI_normal( 5,0,1,2,3 );
           //glTexCoord2f(1.0,0.25); glVertex3f(P0.x, P0.y, P0.z );
           //glTexCoord2f(1.0,0.50);  glVertex3f(P1.x, P1.y, P1.z );
           //glTexCoord2f(0.75,0.25);  glVertex3f(P3.x, P3.y, P3.z );
           //glTexCoord2f(0.75,0.25);  glVertex3f(P3.x, P3.y, P3.z );
           //glTexCoord2f(1.0,0.50); glVertex3f(P1.x, P1.y, P1.z );
           //glTexCoord2f(0.75,0.50); glVertex3f(P2.x, P2.y, P2.z );
         glEnd();
        }
	   }
#endif

      }
    }
    glEndList();
  }
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

