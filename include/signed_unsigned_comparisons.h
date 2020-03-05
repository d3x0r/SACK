// these macros test the the range of integer and unsigned
// such that an unsigned > integer range is true if it is more than an maxint
// and signed integer < 0 is less than any unsigned value.

// the arguments passed are variable a and b and the respective types of those
// if( SUS_GT( 324, int, 545, unsigned int ) ) {
//    is > 
// }

#  define SUS_GT(a,at,b,bt)   (((a)<0)?0:(((bt)a)>(b)))
#  define USS_GT(a,at,b,bt)   (((b)<0)?1:((a)>((at)b)))

#  define SUS_LT(a,at,b,bt)   (((a)<0)?1:(((bt)a)<(b)))
#  define USS_LT(a,at,b,bt)   (((b)<0)?0:((a)<((at)b)))

#  define SUS_GTE(a,at,b,bt)  (((a)<0)?0:(((bt)a)>=(b)))
#  define USS_GTE(a,at,b,bt)  (((b)<0)?1:((a)>=((at)b)))

#  define SUS_LTE(a,at,b,bt)  (((a)<0)?1:(((bt)a)<=(b)))
#  define USS_LTE(a,at,b,bt)  (((b)<0)?0:((a)<=((at)b)))

#if 0
// simplified meanings of the macros
#  define SUS_GT(a,at,b,bt)   ((a)>(b))
#  define USS_GT(a,at,b,bt)   ((a)>(b))

#  define SUS_LT(a,at,b,bt)   ((a)<(b))
#  define USS_LT(a,at,b,bt)   ((a)<(b))

#  define SUS_GTE(a,at,b,bt)  ((a)>=(b))
#  define USS_GTE(a,at,b,bt)  ((a)>=(b))

#  define SUS_LTE(a,at,b,bt)  ((a)<=(b))
#  define USS_LTE(a,at,b,bt)  ((a)<=(b))
#endif
