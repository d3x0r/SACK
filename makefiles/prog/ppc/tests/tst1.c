#define mac(a,b,c,d,e,f) printf( WIDE("Hmm got some things..."), a, b, c, d, e, f )

mac(1,2,,,5,6)

#define __CONCAT(a,b) a##b
#define _Mdouble_ 		double
#define __MATH_PRECNAME(name,r)	__CONCAT(name,r)
# define _Mdouble_BEGIN_NAMESPACE __BEGIN_NAMESPACE_STD
# define _Mdouble_END_NAMESPACE   __END_NAMESPACE_STD

#define __MATHCALL(function,suffix, args)	\
  __MATHDECL (_Mdouble_,function,suffix, args)
#define __MATHDECL(type, function,suffix, args) \
  __MATHDECL_1(type, function,suffix, args); \
  __MATHDECL_1(type, __CONCAT(__,function),suffix, args)
#define __MATHCALLX(function,suffix, args, attrib)	\
  __MATHDECLX (_Mdouble_,function,suffix, args, attrib)
#define __MATHDECLX(type, function,suffix, args, attrib) \
  __MATHDECL_1(type, function,suffix, args) __attribute__ (attrib); \
  __MATHDECL_1(type, __CONCAT(__,function),suffix, args) __attribute__ (attrib)
#define __MATHDECL_1(type, function,suffix, args) \
  extern type __MATH_PRECNAME(function,suffix) args __THROW

_Mdouble_BEGIN_NAMESPACE
/* Arc cosine of X.  */
__MATHCALL (acos,, (_Mdouble_ __x));
/* Arc sine of X.  */
__MATHCALL (asin,, (_Mdouble_ __x));
/* Arc tangent of X.  */
__MATHCALL (atan,, (_Mdouble_ __x));
/* Arc tangent of Y/X.  */
__MATHCALL (atan2,, (_Mdouble_ __y, _Mdouble_ __x));


#define LIBMAIN() int WINAPI LibMain( HINSTANCE hInst, DWORD dwReason, void *unused )

LIBMAIN()
{
   printf( WIDE("Entrered") );
}


#define three banana
mac( test, one
	, two
	, three
	 four five, six seven,
	 eight );

mac( test, one , two , three  four five, six seven,  eight );

defined(__ANYTHING__ ) okay.



#  define __THROW
#define __STRING(n) #n

//#define __USER_LABEL_PREFIX__ __
# define __REDIRECT(name, proto, alias) name proto __asm__ (__ASMNAME (#alias))
# define __ASMNAME(cname)  __ASMNAME2 (__USER_LABEL_PREFIX__, cname)
# define __ASMNAME2(prefix, cname) __STRING (prefix) cname

extern FILE *__REDIRECT (tmpfile, (void) __THROW, tmpfile64);

extern FILE *__REDIRECT (freopen, (__const char *__restrict __filename,
				   __const char *__restrict __modes,
				   FILE *__restrict __stream) __THROW,
			 freopen64);

#define A(n) test __STRING(n)
#define B(n) A(test __STRING(n))
#define C(n) B(test __STRING(n))
#define D(n) C(test __STRING(n))

A(hello)
B(world)
C(okay)
D(bye)

