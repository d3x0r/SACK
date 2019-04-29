
#include <stdhdrs.h>
#include <filesys.h>
#if 0
[Pantera]
RegionUUID = 2023b505-56f8-4d67-bb28-b56764a68584
Location = 173,173
InternalAddress = 192.168.173.2
InternalPort = 9000
AllowAlternatePorts = False
ExternalHostName = 192.168.173.2
MasterAvatarFirstName = moose
MasterAvatarLastName = mr
MasterAvatarSandboxPassword = asdf
#endif

#define MyRegion "Pantero"
//#define MyIP "0.0.0.0"
#define center_point_x 9910
#define center_point_y 9929
#define sub_uuid "bb29"
#define center_point_uuid 584

int main( void )
{
	FILE *output;
	int x, y;
   int w = 5, h = 5;
	output = sack_fopen( 0, "region.test.ini", "wt" );
	for( x = 0; x < w; x++ )
	{
		for( y = 0; y < h; y++ )
		{
         if( (x == w/2) && (y == h/2) )
				fprintf( output, "[%s]\n", MyRegion );
			else
				fprintf( output, "[%s:%c%c]\n", MyRegion, 'A'+x, 'A' + y );

			fprintf( output, "RegionUUID = 2023b505-56f8-4d67-%s-b56764a68%3d\n"
					 , sub_uuid
					 , center_point_uuid+(x*h+y) - (w*h/2) );
         fprintf( output, "Location = %d,%d\n", center_point_x + x - w/2, center_point_y+y-h/2);
			fprintf( output, "InternalAddress = 0.0.0.0\n" );

         // use ports 9000 and above, don't center on port
         fprintf( output, "InternalPort = %d\n", 9000 + (x*h+y) );
         fprintf( output, "AllowAlternatePorts = False\n" );
         fprintf( output, "ExternalHostName = d3x0r.org\n" );
         fprintf( output, "MasterAvatarFirstName = Decker\n" );
         fprintf( output, "MasterAvatarLastName = Thornfoot\n" );
			fprintf( output, "MasterAvatarSandboxPassword = sandB0><\n" );
			fprintf( output, ";NonphysicalPrimMax = 0\n" );
			fprintf( output, ";PhysicalPrimMax = 0\n" );
			fprintf( output, "ClampPrimSize = False\n" );
			fprintf( output, ";MaxPrims = 15000\n" );
         fprintf( output, "\n" );
		}
	}
	fclose( output );
   return 0;
}
