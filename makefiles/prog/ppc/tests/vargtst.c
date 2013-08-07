#define simple(a,b,...)   __VA_ARGS__
#define stringize(...)   #__VA_ARGS__
#define s2(a,...) #a stringize( thing, ##__VA_ARGS__ )
#define simplecat(...)  f(stuff,__VA_ARGS__ )
#define simplecat2(...)  f(stuff,##__VA_ARGS__ )
//simple
simple( 1 )
//simple2
simple( 1, 2 )
//simple3
simple( 1, 2, 3 )
simple( 1, 2, 3, 4 )
simple( 1, 2, 3, 4,5 )

stringize( )
stringize( 1 )
stringize( 1, 2)
stringize( 1, 2, 3 )
stringize( 1, 2, 3, 4 )
stringize( 1, 2, 3, 4,5 )

s2( none )
s2( one, 1 )
s2( two, 1, 2)
s2( three, 1, 2, 3 )
s2( four, 1, 2, 3, 4 )
s2( five, 1, 2, 3, 4, 5 )


#define test(name,...) #__VA_ARGS__ | name##__VA_ARGS__##two | name##__VA_ARGS__ | __VA_ARGS__##name
#define test1(name,...) #__VA_ARGS__ |  name,##__VA_ARGS__##,two | name,##__VA_ARGS__,two | name,__VA_ARGS__##,two | __VA_ARGS__##,name
//
#define test3(name,name2,...) #__VA_ARGS__ | name##__VA_ARGS__##name2 | name##__VA_ARGS__ | __VA_ARGS__##name2
#define test4(name,name2,...) #__VA_ARGS__ |  name,##__VA_ARGS__##,name2 | name,##__VA_ARGS__,name2 | name,__VA_ARGS__##,name2 | __VA_ARGS__##,name2

#define TOSTR2(...) #__VA_ARGS__
#define TOSTR(s) TOSTR2(s)

TOSTR(test(one,x,y,z))
TOSTR(test(one))
TOSTR(test1(one,x,y,z))
TOSTR(test1(one))
TOSTR(test3(red,blue,r,g,b,i,v))
TOSTR(test3(red,blue))
TOSTR(test4(red,blue,r,g,b,i,v))
TOSTR(test4(red,blue))

test(one,x,y,z)
test(one)
test1(one,x,y,z)
test1(one)
test3(red,blue,r,g,b,i,v)
test3(red,blue)
test4(red,blue,r,g,b,i,v)
test4(red,blue)

