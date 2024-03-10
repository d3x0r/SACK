
#include "test.h"

#define F(name, code )

#pragma optimize( "st", on )
void test1( void ) {
    uint32_t a = rand();
    uint32_t b, c;
    b = NTOHL1( a );
    c = NTOHL1( b );
    printf( "Test 1_1: %08x %08x %s\n", a, b, (a==c)?"Match":"fail" );
}
#pragma optimize( "", on )


#pragma optimize( "st", on )
void test2( void ) {
    uint32_t a = rand();
    uint32_t b, c;
    b = NTOHL2( a );
    c = NTOHL2( b );
    printf( "Test 1_2: %08x %08x %s\n", a, b, (a==c)?"Match":"fail" );
}
#pragma optimize( "", on )


#pragma optimize( "st", on )
void test3( void ) {
    uint32_t a = rand();
    uint32_t b, c;
    // C - bswap 
    b = NTOHL3( a );
    c = NTOHL3( b );
    printf( "Test 1_3: %08x %08x %s\n", a, b, (a==c)?"Match":"fail" );
}
#pragma optimize( "", on )


#pragma optimize( "st", on )
void test4( void ) {
    uint32_t a = rand();
    uint32_t b, c;
    // C - bswap
    b = NTOHL4( a );
    c = NTOHL4( b );
    printf( "Test 1_4: %08x %08x %s\n", a, b, (a==c)?"Match":"fail" );
}
#pragma optimize( "", on )


#pragma optimize( "st", on )
void test1_arr( int ofs ) {
    uint8_t a[12];
	 ((uint32_t*)(a+ofs))[0] = rand();
    uint32_t b, c;
    b = NTOHL1( ((uint32_t*)(a+ofs))[0] );
    c = NTOHL1( b );
    printf( "Test 1_1_arr: %08x %08x %s\n", ((uint32_t*)(a+ofs))[0], b, (((uint32_t*)(a+ofs))[0]==c)?"Match":"fail" );
}
#pragma optimize( "", on )


#pragma optimize( "st", on )
void test2_arr( int ofs ) {
    uint8_t a[12];
	 ((uint32_t*)(a+ofs))[0] = rand();
    uint32_t b, c;
    b = NTOHL2( ((uint32_t*)(a+ofs))[0] );
    c = NTOHL2( b );
    printf( "Test 1_2_arr: %08x %08x %s\n", ((uint32_t*)(a+ofs))[0], b, (((uint32_t*)(a+ofs))[0]==c)?"Match":"fail" );
}
#pragma optimize( "", on )


#pragma optimize( "st", on )
void test3_arr( int ofs ) {
    uint8_t a[12];
	 ((uint32_t*)(a+ofs))[0] = rand();
    uint32_t b, c;
    // C - bswap 
    b = NTOHL3( ((uint32_t*)(a+ofs))[0] );
    c = NTOHL3( b );
    printf( "Test 1_3_arr: %08x %08x %s\n", ((uint32_t*)(a+ofs))[0], b, (((uint32_t*)(a+ofs))[0]==c)?"Match":"fail" );
}
#pragma optimize( "", on )


#pragma optimize( "st", on )
void test4_arr( int ofs ) {
    uint8_t a[12];
	 ((uint32_t*)(a+ofs))[0] = rand();
    uint32_t b, c;
    // C - bswap
    b = NTOHL4( ((uint32_t*)(a+ofs))[0] );
    c = NTOHL4( b );
    printf( "Test 1_4_arr: %08x %08x %s\n", ((uint32_t*)(a+ofs))[0], b, (((uint32_t*)(a+ofs))[0]==c)?"Match":"fail" );
}
#pragma optimize( "", on )



#pragma optimize( "st", on )
void test2_1( void ) {
    uint32_t a = rand();
    uint32_t b, c;
    NTOHL2_1( b, a );
    NTOHL2_1( c, b );
    printf( "Test 2_1: %08x %08x %s\n", a, b, (a==c)?"Match":"fail" );
}
#pragma optimize( "", on )

#pragma optimize( "st", on )
void test2_2( void ) {
    uint32_t a = rand();
    uint32_t b, c;
    NTOHL2_2( b, a );
    NTOHL2_2( c, b );
    printf( "Test 2_2: %08x %08x %s\n", a, b, (a==c)?"Match":"fail" );
}
#pragma optimize( "", on )

#pragma optimize( "st", on )
void test2_3( void ) {
    uint32_t a = rand();
    uint32_t b, c;
    // C - bswap (only c=b)
    NTOHL2_3( b, a );
    NTOHL2_3( c, b );
    printf( "Test 2_3: %08x %08x %s\n", a, b, (a==c)?"Match":"fail" );
}
#pragma optimize( "", on )


#pragma optimize( "st", on )
void test2_4( void ) {
    uint32_t a = rand();
    uint32_t b, c;
    NTOHL2_4( b, a );
    NTOHL2_4( c, b );
    printf( "Test 2_4: %08x %08x %s\n", a, b, (a==c)?"Match":"fail" );
}
#pragma optimize( "", on )

#pragma optimize( "st", on )
void test2_5( void ) {
    uint32_t a = rand();
    uint32_t b, c;
    NTOHL2_5( b, a );
    NTOHL2_5( c, b );
    printf( "Test 2_5: %08x %08x %s\n", a, b, (a==c)?"Match":"fail" );
}
#pragma optimize( "", on )


// --------- Test 2 - complex array arguments

#pragma optimize( "st", on )
void test2_1_arr( int ofs ) {
    uint8_t a[12];
	 ((uint32_t*)(a+ofs))[0] = rand();
	 
    uint32_t b, c;
    NTOHL2_1( b, ((uint32_t*)(a+ofs))[0]);
    NTOHL2_1( c, b );
    printf( "Test 2_1(arr): %08x %08x %s\n", (*(uint32_t*)(a+ofs)), b, ( (*(uint32_t*)(a+ofs))==c)?"Match":"fail" );
}
#pragma optimize( "", on )

#pragma optimize( "st", on )
void test2_2_arr( int ofs ) {
    uint8_t a[12];
	 ((uint32_t*)(a+ofs))[0] = rand();
    uint32_t b, c;
    NTOHL2_2( b, ((uint32_t*)(a+ofs))[0]);
    NTOHL2_2( c, b );
    printf( "Test 2_2(arr): %08x %08x %s\n", (*(uint32_t*)(a+ofs)), b, ((*(uint32_t*)(a+ofs))==c)?"Match":"fail" );
}
#pragma optimize( "", on )

#pragma optimize( "st", on )
void test2_3_arr( int ofs ) {
    uint8_t a[12];
	 ((uint32_t*)(a+ofs))[0] = rand();
    uint32_t b, c;
    NTOHL2_3( b, ((uint32_t*)(a+ofs))[0]);
    NTOHL2_3( c, b );
    printf( "Test 2_3(arr): %08x %08x %s\n", (*(uint32_t*)(a+ofs)), b, ((*(uint32_t*)(a+ofs))==c)?"Match":"fail" );
}
#pragma optimize( "", on )


#pragma optimize( "st", on )
void test2_4_arr( int ofs ) {
    uint8_t a[12];
	 ((uint32_t*)(a+ofs))[0] = rand();
    uint32_t b, c;
    NTOHL2_4( b, ((uint32_t*)(a+ofs))[0]);
    NTOHL2_4( c, b );
    printf( "Test 2_4(arr): %08x %08x %s\n", (*(uint32_t*)(a+ofs)), b, ((*(uint32_t*)(a+ofs))==c)?"Match":"fail" );
}
#pragma optimize( "", on )

#pragma optimize( "st", on )
void test2_5_arr( int ofs ) {
    uint8_t a[12];
	 ((uint32_t*)(a+ofs))[0] = rand();
    uint32_t b, c;
    NTOHL2_5( b, ((uint32_t*)(a+ofs))[0]);
    NTOHL2_5( c, b );
    printf( "Test 2_5(arr): %08x %08x %s\n", (*(uint32_t*)(a+ofs)), b, ((*(uint32_t*)(a+ofs))==c)?"Match":"fail" );
}
#pragma optimize( "", on )


int main( void ) {
	test1();
	test2();
	test3();
	test4();

	test1_arr(8);
	test2_arr(8);
	test3_arr(8);
	test4_arr(8);

	test2_1();
	test2_2();
	test2_3();
	test2_4();
	test2_5();

	test2_1_arr(8);
	test2_2_arr(8);
	test2_3_arr(8);
	test2_4_arr(8);
	test2_5_arr(8);
	return 0;
}
