
#if defined( _MSC_VER ) || (1)
// huh, apparently all compiles are messed the hell up.
#  define COMPILER_THROWS_SIGNED_UNSIGNED_MISMATCH
#endif

#ifdef COMPILER_THROWS_SIGNED_UNSIGNED_MISMATCH

#  define SUS_GT(a,at,b,bt)   (((a)<0)?0:(((bt)a)>(b)))
#  define USS_GT(a,at,b,bt)   (((b)<0)?1:((a)>((at)b)))

#  define SUS_LT(a,at,b,bt)   (((a)<0)?1:(((bt)a)<(b)))
#  define USS_LT(a,at,b,bt)   (((b)<0)?0:((a)<((at)b)))

#  define SUS_GTE(a,at,b,bt)  (((a)<0)?0:(((bt)a)>=(b)))
#  define USS_GTE(a,at,b,bt)  (((b)<0)?1:((a)>=((at)b)))

#  define SUS_LTE(a,at,b,bt)  (((a)<0)?1:(((bt)a)<=(b)))
#  define USS_LTE(a,at,b,bt)  (((b)<0)?0:((a)<=((at)b)))

#else
#  define SUS_GT(a,at,b,bt)   ((a)>(b))
#  define USS_GT(a,at,b,bt)   ((a)>(b))

#  define SUS_LT(a,at,b,bt)   ((a)<(b))
#  define USS_LT(a,at,b,bt)   ((a)<(b))

#  define SUS_GTE(a,at,b,bt)  ((a)>=(b))
#  define USS_GTE(a,at,b,bt)  ((a)>=(b))

#  define SUS_LTE(a,at,b,bt)  ((a)<=(b))
#  define USS_LTE(a,at,b,bt)  ((a)<=(b))
#endif
