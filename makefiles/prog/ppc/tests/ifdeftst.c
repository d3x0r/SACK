

#  define __THROW
#define __STRING(n) #n

# define __REDIRECT(name, proto, alias) name proto __asm__ (__ASMNAME (#alias))
# define __ASMNAME(cname)  __ASMNAME2 (__USER_LABEL_PREFIX__, cname)
# define __ASMNAME2(prefix, cname) __STRING (prefix) cname

extern FILE *__REDIRECT (tmpfile, (void) __THROW, tmpfile64);


#if __STDC_VERSION__ >= 199901L
#endif

#define str(n) #n
#define strsym(n) str(n)
#define NOTE(s) message __FILE__"("strsym(__LINE__)") Note: " s
#define ERROR(s) message __FILE__"("strsym(__LINE__)") Error: " s
#define WARNING(s) message __FILE__"("strsym(__LINE__)") Warning: " s

#pragma NOTE("Test me!!!! " __DATE__ " at " __TIME__ );

#if (3*4 == 12 )
#pragma NOTE( WIDE("multiply") )
#endif

#if ( 15 & 12 == 12 )
#pragma NOTE( WIDE("and") )
#endif

#if ( 4/5 >= 0 )
#pragma NOTE( WIDE("divide") )
#endif

#if ( 'a' == 97 )
#pragma NOTE( WIDE("character equates") )
#endif

#if ( 1 * 15 / 3 & 1 | 0xff + 4 ^ 8 - 2 > 0 || !0 == 1 && ~0 <= 0xFF )
#endif

#if ( 5?(3,2):1 || 0xFFFFFFFFFFFFFFFF == 1 )
#endif

str(__LINE__)
strsym(__LINE__)

str( WIDE("Hello") );
str( WIDE("He\")llo" );

//#define a(n) x(n)

#define f(n) a(n),n

f(x(n))


#pragma NOTE("Text goes here...")

char *p1 = "/*   has   a       comment...   buhahah";
char *p2 = "and another\" // ";
char *p3 = \" blah \" /*  */ // ";


/* // */
/* // junk here
// more junk
*/

// char stuff; /* junk here
*/

/* this " comment */ char *p = "string";

#define __ONE__ 1
#define __ONE__ 2

#ifdef __ONE__ /* a comment
begins there and doesn't influence the if*/
int one;
#ifdef __TWO__
int _one;
#ifdef __THREE__
int __one;
#else
int __two;
#endif
#else
int _two;
#endif
#else
int two;
#endif


#undef __ONE__

#ifdef __ONE__
int one;
#else
#ifdef __TWO__
int _two;
#ifdef __THREE__
int __two;
#else
int __three;
#endif
#else
int _three;
#endif

int two;
#endif

#define __ONE__ 1
#if (__ONE__==1)
#pragma NOTE("Text goes here..." );
#endif

/* it so happens that we only need one flag - for the next
thing we are looking for - if not find else or find endif
then we can process this line....
if we find an else to process with neither flag set
then set find endif....
if( find else or findendif )
skip this line... if type statements increment the number of ifs
and endif decrements the number of ifs...

//

*/
__FILE__ 
__LINE__

#define f(a,b) a = b + 1

f(baker, banana)
f(one,
  two )
f( WIDE("one string, some more text, one param")
  , WIDE("Another String") )
f((1,baker), apple)

//#define str(n) #n
//#define strsym(n) str(n)
str(__LINE__)
strsym(__LINE__)

#define DBG_SRC , __FILE__, __LINE__

#define test(f,...) fprintf( f DBG_SRC, ... )

   test( WIDE("%d %d %d"), 1,2,3 )
