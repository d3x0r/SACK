
#if defined( _MSC_VER ) || (1)
// huh, apparently all compiles are mess the hell up.
#define COMPILER_THROWS_SIGNED_UNSIGNED_MISMATCH
#endif

#ifdef COMPILER_THROWS_SIGNED_UNSIGNED_MISMATCH

// if cplusplus, some compilers I can definatly strongly typecast check what was passed as the types
// and the values passed.  C I'm not so fortunate.
#if defined( __cplusplus ) && 0
#define _sus_paste3(a,b,c) a##b##c
#define sus_paste3(a,b,c) _sus_paste3(a,b,c)

static void anyfunction() { }
#define SUS_GT(a,at,b,bt)   (void(*f)(at,bt) sus_paste3(tmp,at,__LINE__) = a, bt sus_paste3(tmp,bt,__LINE__) = b }),(((a)<0)?0:(((bt)a)>(b))))
#define USS_GT(a,at,b,bt)   (at sus_paste3(tmp,at,__LINE__) = a, bt sus_paste3(tmp,bt,__LINE__) = b ),(((b)<0)?1:((a)>((at)b))))

#define SUS_LT(a,at,b,bt)   (at sus_paste3(tmp,at,__LINE__) = a, bt sus_paste3(tmp,bt,__LINE__) = b ),(((a)<0)?1:(((bt)a)<(b))))
#define USS_LT(a,at,b,bt)   (at sus_paste3(tmp,at,__LINE__) = a, bt sus_paste3(tmp,bt,__LINE__) = b ),(((b)<0)?0:((a)<((at)b))))

#define SUS_GTE(a,at,b,bt)  (at sus_paste3(tmp,at,__LINE__) = a, bt sus_paste3(tmp,bt,__LINE__) = b ),(((a)<0)?0:(((bt)a)>=(b))))
#define USS_GTE(a,at,b,bt)  (at sus_paste3(tmp,at,__LINE__) = a, bt sus_paste3(tmp,bt,__LINE__) = b ),(((b)<0)?1:((a)>=((at)b))))

#define SUS_LTE(a,at,b,bt)  (at sus_paste3(tmp,at,__LINE__) = a, bt sus_paste3(tmp,bt,__LINE__) = b ),(((a)<0)?1:(((bt)a)<=(b))))
#define USS_LTE(a,at,b,bt)  (at sus_paste3(tmp,at,__LINE__) = a, bt sus_paste3(tmp,bt,__LINE__) = b ),(((b)<0)?0:((a)<=((at)b))))
#else
#define SUS_GT(a,at,b,bt)   (((a)<0)?0:(((bt)a)>(b)))
#define USS_GT(a,at,b,bt)   (((b)<0)?1:((a)>((at)b)))

#define SUS_LT(a,at,b,bt)   (((a)<0)?1:(((bt)a)<(b)))
#define USS_LT(a,at,b,bt)   (((b)<0)?0:((a)<((at)b)))

#define SUS_GTE(a,at,b,bt)  (((a)<0)?0:(((bt)a)>=(b)))
#define USS_GTE(a,at,b,bt)  (((b)<0)?1:((a)>=((at)b)))

#define SUS_LTE(a,at,b,bt)  (((a)<0)?1:(((bt)a)<=(b)))
#define USS_LTE(a,at,b,bt)  (((b)<0)?0:((a)<=((at)b)))
#endif

#else
#define SUS_GT(a,at,b,bt)   ((a)>(b))
#define USS_GT(a,at,b,bt)   ((a)>(b))

#define SUS_LT(a,at,b,bt)   ((a)<(b))
#define USS_LT(a,at,b,bt)   ((a)<(b))

#define SUS_GTE(a,at,b,bt)  ((a)>=(b))
#define USS_GTE(a,at,b,bt)  ((a)>=(b))

#define SUS_LTE(a,at,b,bt)  ((a)<=(b))
#define USS_LTE(a,at,b,bt)  ((a)<=(b))
#endif
