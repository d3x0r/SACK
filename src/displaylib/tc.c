
int main( void )
{
   int pa1 = 300;
	int pa2 = 400;

	int po1 = 290;
	int po2 = 380;

	//int en = 400;
	//int ex = 3400;
	int en = 300;
	int ex = 3200;

   int ew = ex - en;

   int r = 640;

	int ei1 = ( ( ( po1 ) * ( ew ) ) / r ) + en;
	int ei2 = ( ( ( po2 ) * ( ew ) ) / r ) + en;

	printf( WIDE("%d = %d (old params)\n"), ei1, po1 );
	printf( WIDE("%d = %d (old params)\n"), ei2, po2 );

	int nen = ( pa2 * ei1 - pa1 * ei2 ) / ( pa2 - pa1 );
	int new = ( ( ei1 - nen ) * r ) / pa1;
   int nex = new + nen;

	printf( WIDE("min %d  max %d\n"), nen, nex );

	int npo1 = r * ( ei1 - nen ) / ( nex - nen );
   printf( WIDE("%d = %d (with new params)\n"), ei1, npo1 );
	int npo2 = r * ( ei2 - nen ) / ( nex - nen );
   printf( WIDE("%d = %d (with new params)\n"), ei2, npo2 );
}
