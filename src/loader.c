#include "loader.h"
#include "loader_private.h"

#include <stdlib.h>

/* Struct describing handle for a dynamic library */
struct library {
	int test;
};

struct library *library_load(const char *name, void *(*getsym)(char const *))
{
	return NULL;
}

void *library_getsym(struct library *lib, const char *name)
{
	return NULL;
}
