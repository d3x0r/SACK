
#define macro(a,b,c,d,e) printf( WIDE("Macro called with:") #a #b #c #d #e )
#define macro2(...) printf( WIDE("Macro called with: %d"), __SZ_ARGS__, __VA_ARGS__ )

macro2( 1>2?56:"hi", 3.0, 1>2?56:"hi" );

macro( test,this,short )
macro( test,this,short,with,overflow,and,stuff )

macro2( test,this and params,short )
	macro2( test,this,short,( with,overflow,and ),stuff,"much",more,many,paramters )
	macro2();

#define __STRING(n) #n
	__STRING(test)
#define ASM(n) __STRING(n)
#define ASM2(n) ASM(n)
#define NOTHING
	ASM2( test);
   ASM2( NOTHING );


