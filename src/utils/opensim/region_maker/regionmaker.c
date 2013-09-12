
#include <stdhdrs.h>

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
#define center_point_x 178
#define center_point_y 173
#define sub_uuid "bb29"
#define center_point_uuid 584

int main( void )
{
	FILE *output;
	int x, y;
   int w = 5, h = 5;
	output = fopen( "region.test.ini", "wt" );
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
         fprintf( output, "ExternalHostName = 127.0.0.1\n" );
         fprintf( output, "MasterAvatarFirstName = master\n" );
         fprintf( output, "MasterAvatarLastName = moose\n" );
			fprintf( output, "MasterAvatarSandboxPassword = sandB0><\n" );
			fprintf( output, ";NonphysicalPrimMax = 0\n" );
			fprintf( output, ";PhysicalPrimMax = 0\n" );
			fprintf( output, "ClampPrimSize = False\n" );
			fprintf( output, ";MaxPrims = 15000\n" );
         fprintf( output, "\n" );
		}
	}
   return 0;
}
