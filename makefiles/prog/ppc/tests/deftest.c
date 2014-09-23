
#define min(a,b) (((a)<(b))?(a):(b))

#define stuffit(a,b,c) (a b c)

stuffit("one",two,three);

#define StrInt(i) #i
#define StrSym(s) StrInt(s)

#define note(message) message __FILE__"("StrSym(__LINE__)")" message

#define warning(message) message __FILE__"("StrSym(__LINE__)") Warning:" message

#pragma note("Okay this is a note..." )

#pragma warning( WIDE("NO warning really just a test...") )

#define blah(a,b,c) (#a b##c )

blah( string, label, symbol )
#define blah2( a, b, c ) #a b##count ... p##c##s  "b##a"

blah2( thing line wall )

min( 5, 6 )
min( j, k )

#define a g // define this to g for massive loop define
#define b a + 1
#define c b + 1
#define d c + 1
#define e d + 1
#define f e + 1
#define g f + 1

#define A 1
#define B A+1
#undef A
#define A B+1
#define C B+1

int i = a + a;
int j = c + a;
int k = e;
int l = g;

#define fprintf if( 0 ) fprintf

fprintf( stderr, WIDE("this string out") );

//int I = A;
//int J = B;
//int K = C;
