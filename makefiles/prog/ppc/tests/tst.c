
#define thing(name) printf( WIDE("%d,%d"), a->name b->name );

thing(x)

CMDARG

#define PLUS +
#define EMPTY
#define F(x) =x=

+PLUS -EMPTY- PLUS+ F(=)

#define add(x, y, z) x + y +z;
add(1,2, 3);
add(1,2, +);

#define TEXT "add stuff"


printf( WIDE("one ") TEXT " two" );
printf( WIDE("one \")" TEXT "\" two" );

#define concat(a,b) a##b

concat(concat(__,func),)()

#define tst concat

	tst(a,b)


#define openfunc(name)  name(
#define ofunc2(name,arg1) name(arg

	openfunc(test) int a );
ofunc2(F) );
