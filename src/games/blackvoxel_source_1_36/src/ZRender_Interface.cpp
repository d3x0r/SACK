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
	simple_shader = new ZRender_Shader_Simple( this );
	gui_shader = new ZRender_Shader_Gui_Texture( this );
	simple_texture_shader = new ZRender_Shader_Simple_Texture( this );

	RadiusZones.SetSize(17,7,17);
	// RadiusZones.DrawZones( 5.0, 3.5, 3.0, 2.0 );
	// RadiusZones.DebugOut();
}

void ZRender_Interface::Render_DebugLine( ZVector3f & Start, ZVector3f & End)
{
	ZVector4f c;
	c.r = 1.0;
	c.g = 1.0;
	c.b = 1.0;
	c.a = 1.0;
	simple_shader->DrawLine( &Start, &End, &c );
#if 0
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
#endif
}

void ZRender_Interface::Render_VoxelSelector(ZVoxelCoords * SelectedVoxel, float r, float g, float b)
{
    ZVector3f P1,P2,P3,P4,P5,P6,P7,P8;
    P1.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + 0.0f;   P1.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + 0.0f; P1.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + 0.0f;
    P2.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + 0.0f;   P2.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + 0.0f; P2.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize;
    P3.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P3.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + 0.0f; P3.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize;
    P4.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P4.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + 0.0f; P4.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + 0.0f;
    P5.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + 0.0f;   P5.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P5.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + 0.0f;
    P6.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + 0.0f;   P6.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P6.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize;
    P7.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P7.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P7.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize;
    P8.x = SelectedVoxel->x * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P8.y = SelectedVoxel->y * GlobalSettings.VoxelBlockSize + GlobalSettings.VoxelBlockSize; P8.z = SelectedVoxel->z * GlobalSettings.VoxelBlockSize + 0.0f;

	{
		ZVector4f c;
		c.r = r;
		c.g = g;
		c.b = b;
		c.a = 1.0;
		simple_shader->DrawBox( &P1, &P2, &P3, &P4, &P5, &P6, &P7, &P8, &c );
	}
#if 0
        glDisable(GL_TEXTURE_2D);
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
#endif

}


void ZRender_Interface::Render_EmptySector(int x, int y, int z, float r, float g, float b)
{

    ZVector3f P1,P2,P3,P4,P5,P6,P7,P8;
	ZVector4f c;
	c.r = r;
	c.g = g;
	c.b = b;
	c.a = 0.5f;
	P1.x = x * ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize + 0.0f;   P1.y = y * ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize + 0.0f; P1.z = z * ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize + 0.0f;
    P2.x = x * ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize + 0.0f;   P2.y = y * ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize + 0.0f; P2.z = z * ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize + ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize;
    P3.x = x * ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize + ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize; P3.y = y * ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize + 0.0f; P3.z = z * ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize + ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize;
    P4.x = x * ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize + ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize; P4.y = y * ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize + 0.0f; P4.z = z * ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize + 0.0f;
    P5.x = x * ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize + 0.0f;   P5.y = y * ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize + ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize; P5.z = z * ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize + 0.0f;
    P6.x = x * ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize + 0.0f;   P6.y = y * ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize + ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize; P6.z = z * ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize + ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize;
    P7.x = x * ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize + ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize; P7.y = y * ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize + ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize; P7.z = z * ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize + ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize;
    P8.x = x * ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize + ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize; P8.y = y * ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize + ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize; P8.z = z * ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize + 0.0f;

	simple_shader->DrawBox( &P1, &P2, &P3, &P4
		, &P5, &P6, &P7, &P8
		, &c );

#if 0
	CheckErr();

              glDisable(GL_TEXTURE_2D);
              glColor3f(r,g,b);
              glEnable(GL_LINE_SMOOTH);
	CheckErr();

              glEnable (GL_LINE_SMOOTH);
              glEnable (GL_BLEND);
	CheckErr();
              glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
              glHint (GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
	CheckErr();
              glLineWidth (3.5);
    //glLineWidth(0.001f);
    //glPointSize(0.001f);
			  	CheckErr();
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
#endif

}



Bool ZRender_Interface::LoadVoxelTexturesToGPU(uintptr_t psvInit)
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
#if 0
void ZRender_Interface::FreeDisplayData(ZVoxelSector * Sector)
{
	lprintf( "common interface cannot release display data." );
#if 0
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
#endif
}
#endif
Bool ZRender_Interface::LoadTexturesToGPU(uintptr_t psvInit)
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

void ZRender_Interface::DrawReticule( void )
{
    if (BvProp_CrossHairType==1 && BvProp_DisplayCrossHair)
    {
		ZVector3f v[4];
		ZVector3f vo[4];
		ZVector4f c;
		int n;
		c.r = 1.0;
		c.g = 1.0;
		c.b = 1.0;
		c.a = 1.0;

#define center_x 0.0
#define ret_width 0.005
#define ret_len 0.05
#define ret_ofs 0.05
#define center_y 0.0
#define reticle_depth 2
		v[0].z= reticle_depth - 0.1;
		v[1].z= reticle_depth - 0.1;
		v[2].z= reticle_depth - 0.1;
		v[3].z= reticle_depth - 0.1;

		v[0].x = center_x-ret_width;   v[0].y = center_y-(ret_ofs+ret_len);
		v[1].x = center_x+ret_width;   v[1].y = center_y-(ret_ofs+ret_len);
		v[2].x = center_x+ret_width;   v[2].y = center_y-ret_ofs;          
		v[3].x = center_x-ret_width;   v[3].y = center_y-ret_ofs;          

		for( n = 0; n < 4; n++ )
			Camera->orientation.Apply( vo[n], v[n] );
		simple_shader->DrawFilledRect( vo, c );

		v[0].x = center_x-ret_width;   v[0].y = center_y+ret_ofs;
		v[1].x = center_x+ret_width;   v[1].y = center_y+ret_ofs;
		v[2].x = center_x+ret_width;   v[2].y = center_y+(ret_ofs+ret_len);
		v[3].x = center_x-ret_width;   v[3].y = center_y+(ret_ofs+ret_len);

		for( n = 0; n < 4; n++ )
			Camera->orientation.Apply( vo[n], v[n] );
		simple_shader->DrawFilledRect( vo, c );

		v[0].x = center_x-(ret_ofs+ret_len);   v[0].y = center_y-ret_width;
		v[1].x = center_x-ret_ofs;   v[1].y = center_y-ret_width;
		v[2].x = center_x-ret_ofs;   v[2].y = center_y+ret_width;
		v[3].x = center_x-(ret_ofs+ret_len);   v[3].y = center_y+ret_width;

		for( n = 0; n < 4; n++ )
			Camera->orientation.Apply( vo[n], v[n] );
		simple_shader->DrawFilledRect( vo, c );

		v[0].x = center_x+(ret_ofs+ret_len);   v[0].y = center_y+ret_width;   v[0].z= reticle_depth;
		v[1].x = center_x+ret_ofs;   v[1].y = center_y+ret_width;   v[1].z= reticle_depth;
		v[2].x = center_x+ret_ofs;   v[2].y = center_y-ret_width;   v[2].z= reticle_depth;
		v[3].x = center_x+(ret_ofs+ret_len);   v[3].y = center_y-ret_width;   v[3].z= reticle_depth;

		for( n = 0; n < 4; n++ )
			Camera->orientation.Apply( vo[n], v[n] );
		simple_shader->DrawFilledRect( vo, c );
	}

#if 0
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluOrtho2D(0, 1440, 900.0 , 0.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (BvProp_CrossHairType==1 && BvProp_DisplayCrossHair)
    {

      glDisable(GL_TEXTURE_2D);
      glBegin(GL_POLYGON);
        glColor3f(1.0,1.0,1.0);
        glVertex3f(720.0f-1.0f, 450.0f-20.0f , 0.0f);
        glVertex3f(720.0f+1.0f, 450.0f-20.0f , 0.0f);
        glVertex3f(720.0f+1.0f, 450.0f-10.0f , 0.0f);
        glVertex3f(720.0f-1.0f, 450.0f-10.0f , 0.0f);
      glEnd();

      glBegin(GL_POLYGON);
        glColor3f(1.0,1.0,1.0);
        glVertex3f(720.0f-1.0f, 450.0f+10.0f , 0.0f);
        glVertex3f(720.0f+1.0f, 450.0f+10.0f , 0.0f);
        glVertex3f(720.0f+1.0f, 450.0f+20.0f , 0.0f);
        glVertex3f(720.0f-1.0f, 450.0f+20.0f , 0.0f);
      glEnd();

      glBegin(GL_POLYGON);
        glColor3f(1.0,1.0,1.0);
        glVertex3f(720.0f-20.0f, 450.0f-1.0f , 0.0f);
        glVertex3f(720.0f-10.0f, 450.0f-1.0f , 0.0f);
        glVertex3f(720.0f-10.0f, 450.0f+1.0f , 0.0f);
        glVertex3f(720.0f-20.0f, 450.0f+1.0f , 0.0f);
      glEnd();

      glBegin(GL_POLYGON);
        glColor3f(1.0,1.0,1.0);
        glVertex3f(720.0f+20.0f, 450.0f+1.0f , 0.0f);
        glVertex3f(720.0f+10.0f, 450.0f+1.0f , 0.0f);
        glVertex3f(720.0f+10.0f, 450.0f-1.0f , 0.0f);
        glVertex3f(720.0f+20.0f, 450.0f-1.0f , 0.0f);
      glEnd();

      glColor3f(1.0,1.0,1.0);
      glEnable(GL_TEXTURE_2D);
    }
#endif
}

void ZRender_Interface::DrawColorOverlay( void )
{
    if (Camera->ColoredVision.Activate)
    {
		ZVector3f v[4];
		ZVector3f vo[4];
		ZVector4f c;
		int n;
		c.r = Camera->ColoredVision.Red;
		c.g = Camera->ColoredVision.Green;
		c.b = Camera->ColoredVision.Blue;
		c.a = Camera->ColoredVision.Opacity;

		v[0].x = Aspect_Ratio[0] * -reticle_depth;   v[0].y = reticle_depth;   v[0].z= reticle_depth + 0.001;
		v[1].x = Aspect_Ratio[0] * reticle_depth;   v[1].y = reticle_depth;   v[1].z= reticle_depth + 0.001;
		v[2].x = Aspect_Ratio[0] * reticle_depth;   v[2].y = -reticle_depth;   v[2].z= reticle_depth + 0.001;
		v[3].x = Aspect_Ratio[0] * -reticle_depth;   v[3].y = -reticle_depth;   v[3].z= reticle_depth + 0.001;

		for( n = 0; n < 4; n++ )
			Camera->orientation.Apply( vo[n], v[n] );
		simple_shader->DrawFilledRect( vo, c );

#if 0
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_DEPTH_TEST);
      glColor4f(Camera->ColoredVision.Red,Camera->ColoredVision.Green,Camera->ColoredVision.Blue,Camera->ColoredVision.Opacity);
      glBegin(GL_POLYGON);
        glVertex3f(0.0f   , 0.0f   , 0.0f);
        glVertex3f(1440.0f, 0.0f   , 0.0f);
        glVertex3f(1440.0f, 900.0f , 0.0f);
        glVertex3f(0.0f   , 900.0f , 0.0f);
      glEnd();
      glColor3f(1.0,1.0,1.0);
      glEnable(GL_TEXTURE_2D);
      glEnable(GL_DEPTH_TEST);
				{ int x; if( x = glGetError() ) 
					printf( "glerror(%d): %d\n", __LINE__, x );}
#endif
    }

}