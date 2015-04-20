#pragma once

#include <stdio.h>

#ifdef DEBUG_MESSAGES
#define LOG(M)\
    do { fprintf(stderr, "[LOG] %d:\t" M "\n", __LINE__); } while (0)
#define LOGS(M)\
    do { fprintf(stderr, \
    "---------------------------------------"\
    "\n[LOG] %d:\t" M \
    "\n---------------------------------------\n", __LINE__); } while (0)
#define LOGM(M, ...)\
    do { fprintf(stderr, "[LOG] %d:\t" M "\n", __LINE__, ##__VA_ARGS__); } while (0)
#else
#define LOG(...)  do { } while (0)
#define LOGM(...) do { } while (0)
#define LOGS(...) do { } while (0)
#endif

#define ERR(M)\
    fprintf(stderr, "[ERR] %d:\t" M "\n", __LINE__)

#define WHEN(A, L, M)\
    if(A) { ERR(M); goto L; }

#ifdef HITTEST
#undef WHEN

static size_t hittest_counter_42 = HITTEST;
#define WHEN(A, L, M)\
    if (hittest_counter_42-- == 0) {\
        goto L;\
    } else {\
        LOGM("hittest_counter: %zu", hittest_counter_42);\
        if (A) { ERR(M); goto L; }\
    }

#endif