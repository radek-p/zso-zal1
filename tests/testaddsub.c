#include "loader.h"
#include <stdio.h>

void *getsym_none(const char *name) {
	return 0;
}

int main() {
	struct library *lib;
	lib = library_load("addsub.so", getsym_none);
	int res = 0;
	if (!lib) {
		printf("ERROR: Can't load addsub.so\n");
		return 1;
	}
	int (*add)(int, int);
	int (*sub)(int, int);
	add = library_getsym(lib, "add");
	if (!add) {
		printf("ERROR: Can't find add\n");
		return 1;
	}
	sub = library_getsym(lib, "sub");
	if (!sub) {
		printf("ERROR: Can't find sub\n");
		return 1;
	}
	int ok = 1;
	int sum = add(2, 2);
	if (sum != 4) {
		printf("ERROR: 2 + 2 = %d\n", sum);
		return 1;
	}
	int diff = sub(0xc0000000, 0x12345678);
	if (diff != 2915805576) {
		printf("ERROR: sub doesn't work\n");
		return 1;
	}
	printf("OK\n");
	return 0;
}
