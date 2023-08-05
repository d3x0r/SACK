#define d(f,...)  a(10, f,##__VA_ARGS__)

#define extra __FILE__,__LINE__

void f(void){
    d(extra);
}
