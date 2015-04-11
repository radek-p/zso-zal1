#pragma once

#include <stdio.h>

#ifdef DEBUG
#define LOG(M)\
    fprintf(stderr, "[LOG] %d:\t" M "\n", __LINE__)
#define LOGM(M, ...)\
    fprintf(stderr, "[LOG] %s:%d:\t" M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG(M, ...)
#define LOGM(M, ...)
#endif

#define WHEN(A, L, M)\
    if(A) { LOG(M); goto L; }
#define WHENM(A, L, M, ...)\
    if(A) { LOG(M, ##__VA_ARGS__); goto L; }
