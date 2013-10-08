
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

#define MyRegion WIDE("Pantero")
//#define MyIP WIDE("0.0.0.0")
#define center_point_x 178
#define center_point_y 173
#define sub_uuid WIDE("bb29")
#define center_point_uuid 584

int main( void )
{
	FILE *output;
	int x, y;
   int w = 5, h = 5;
	output = sack_fopen( 0, WIDE("region.test.ini"), WIDE("wt") );
	for( x = 0; x < w; x++ )
	{
		for( y = 0; y < h; y++ )
		{
         if( (x == w/2) && (y == h/2) )
				fprintf( output, WIDE("[%s]\n"), MyRegion );
			else
				fprintf( output, WIDE("[%s:%c%c]\n"), MyRegion, 'A'+x, 'A' + y );

			fprintf( output, WIDE("RegionUUID = 2023b505-56f8-4d67-%s-b56764a68%3d\n")
					 , sub_uuid
					 , center_point_uuid+(x*h+y) - (w*h/2) );
         fprintf( output, WIDE("Location = %d,%d\n"), center_point_x + x - w/2, center_point_y+y-h/2);
			fprintf( output, WIDE("InternalAddress = 0.0.0.0\n") );

         // use ports 9000 and above, don't center on port
         fprintf( output, WIDE("InternalPort = %d\n"), 9000 + (x*h+y) );
         fprintf( output, WIDE("AllowAlternatePorts = False\n") );
         fprintf( output, WIDE("ExternalHostName = 127.0.0.1\n") );
         fprintf( output, WIDE("MasterAvatarFirstName = master\n") );
         fprintf( output, WIDE("MasterAvatarLastName = moose\n") );
			fprintf( output, WIDE("MasterAvatarSandboxPassword = sandB0><\n") );
			fprintf( output, WIDE(";NonphysicalPrimMax = 0\n") );
			fprintf( output, WIDE(";PhysicalPrimMax = 0\n") );
			fprintf( output, WIDE("ClampPrimSize = False\n") );
			fprintf( output, WIDE(";MaxPrims = 15000\n") );
         fprintf( output, WIDE("\n") );
		}
	}
   return 0;
}
