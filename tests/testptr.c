#include "loader.h"
#include <stdio.h>

int x[] = {23, 5};

void *getsym_ptr(const char *name) {
	if (!strcmp(name, "x"))
		return &x;
	return 0;
}

int main() {
	struct library *lib;
	lib = library_load("ptr.so", getsym_ptr);
	if (!lib) {
		printf("ERROR: Can't load ptr.so\n");
		return 1;
	}
	int **ppx0, **ppx1;
	ppx0 = library_getsym(lib, "px0");
	if (!ppx0) {
		printf("ERROR: Can't find var px0\n");
		return 1;
	}
	ppx1 = library_getsym(lib, "px1");
	if (!ppx1) {
		printf("ERROR: Can't find var px1\n");
		return 1;
	}
	if (**ppx0 != x[0]) {
		printf("ERROR: px0 contents invalid\n");
		return 1;
	}
	if (**ppx1 != x[1]) {
		printf("ERROR: px1 contents invalid\n");
		return 1;
	}
	printf("OK\n");
	return 0;
}
