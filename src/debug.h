#pragma once

#include <stdio.h>

#ifdef DEBUG
#define LOG(M)\
    fprintf(stderr, "[LOG] %d:\t" M "\n", __LINE__)
#define SLOG(M)\
    fprintf(stderr, "---------------------------------------\n[LOG] %d:\t" M "\n---------------------------------------\n", __LINE__)
#define LOGM(M, ...)\
    fprintf(stderr, "[LOG] %d:\t" M "\n", __LINE__, ##__VA_ARGS__)
#else
#define LOG(M, ...)  do { } while (0)
#define LOGM(M, ...) do { } while (0)
#define SLOG(M, ...) do { } while (0)
#endif

#define WHEN(A, L, M)\
    if(A) { LOG(M); goto L; }
#define WHENM(A, L, M, ...)\
    if(A) { LOG(M, ##__VA_ARGS__); goto L; }
