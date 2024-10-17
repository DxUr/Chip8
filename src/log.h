#include <stdio.h>

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)

#ifdef DEBUG
#define LOG(STR) puts("[" __FILE__ ":" LINE_STRING "] " STR);
#define LOGF(FMT, ...) printf("[" __FILE__ ":" LINE_STRING "] " FMT, __VA_ARGS__);
#else
#define LOG(STR) puts(STR);
#define LOGF(FMT, ...) printf(FMT, __VA_ARGS__);
#endif
