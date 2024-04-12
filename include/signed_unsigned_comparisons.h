// these macros test the the range of integer and unsigned
// such that an unsigned > integer range is true if it is more than an maxint
// and signed integer < 0 is less than any unsigned value.

// the macro prefix SUS  or USS is the comparison type
// Signed-UnSigned and UnSigned-Signed  depending on the 
// operand order.

// the arguments passed are variable a and b and the respective types of those
// if( SUS_GT( 324, int, 545, unsigned int ) ) {
//    is > 
// }

/* Compare two numbers a>b, the first being signed and the second
   being unsigned. Compares only within overlapping ranges else
   \returns condition of non-overlap.                           
   at is the type of a and bt is the type of b
*/
#  define SUS_GT(a,at,b,bt)   (((a)<0)?0:(((bt)a)>(b)))
/* Compare two numbers a>b, the first being unsigned and the second
   being signed. Compares only within overlapping ranges else
   \returns condition of non-overlap.
   at is the type of a and bt is the type of b
*/
#  define USS_GT(a,at,b,bt)   (((b)<0)?1:((a)>((at)b)))

/* Compare two numbers, the first being unsigned and the second
   being signed. Compares only within overlapping ranges else
   \returns condition of non-overlap.
   at is the type of a and bt is the type of b
*/
#  define SUS_LT(a,at,b,bt)   (((a)<0)?1:(((bt)a)<(b)))

/* Compare two numbers, the first being unsigned and the second
   being signed. Compares only within overlapping ranges else
   \returns condition of non-overlap.
   at is the type of a and bt is the type of b
*/
#  define USS_LT(a,at,b,bt)   (((b)<0)?0:((a)<((at)b)))

/* Compare two numbers a>=b, the first being signed and the second
   being unsigned. Compares only within overlapping ranges else
   \returns condition of non-overlap.
   at is the type of a and bt is the type of b
*/
#  define SUS_GTE(a,at,b,bt)  (((a)<0)?0:(((bt)a)>=(b)))
/* Compare two numbers a>=b, the first being unsigned and the second
   being signed. Compares only within overlapping ranges else
   \returns condition of non-overlap.
   at is the type of a and bt is the type of b
*/
#  define USS_GTE(a,at,b,bt)  (((b)<0)?1:((a)>=((at)b)))

/* Compare two numbers a<=b, the first being signed and the second
   being unsigned. Compares only within overlapping ranges else
   \returns condition of non-overlap.
   at is the type of a and bt is the type of b
*/
#  define SUS_LTE(a,at,b,bt)  (((a)<0)?1:(((bt)a)<=(b)))
/* Compare two numbers a<=b, the first being unsigned and the second
   being signed. Compares only within overlapping ranges else
   \returns condition of non-overlap.
   at is the type of a and bt is the type of b
*/
#  define USS_LTE(a,at,b,bt)  (((b)<0)?0:((a)<=((at)b)))

#if 0
// simplified meanings of the macros
#  define SUS_GT(a,at,b,bt)   ((a)>(b))
#  define USS_GT(a,at,b,bt)   ((a)>(b))

#  define SUS_LT(a,at,b,bt)   ((a)<(b))
/* Compare two numbers, the first being unsigned and the second
   being signed. Compares only within overlapping ranges else
   \returns condition of non-overlap.                           */
#  define USS_LT(a,at,b,bt)   ((a)<(b))

#  define SUS_GTE(a,at,b,bt)  ((a)>=(b))
#  define USS_GTE(a,at,b,bt)  ((a)>=(b))

#  define SUS_LTE(a,at,b,bt)  ((a)<=(b))
#  define USS_LTE(a,at,b,bt)  ((a)<=(b))
#endif
