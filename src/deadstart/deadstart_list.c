
#include <sack_types.h>
#include <deadstart.h>

#ifdef __GNUC__
#ifndef TARGET_LABEL
#error NO TARGET_LABEL - required for library target name differentiation
#endif
#define paste(a,b) a##b
#define paste2(a,b) paste(a,b)
#define DeclareList(n) paste2(n,TARGET_LABEL)
#ifdef __cplusplus
#else
struct rt_init DeclareList(begin_deadstart_) __attribute__((section("deadstart_list"))) = { DEADSTART_RT_LIST_START };
#endif
#endif
