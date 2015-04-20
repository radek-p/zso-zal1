#include "loader.h"
#include <stdio.h>

void *getsym_none(const char *name) {
	return 0;
}

int main() {
	struct library *lib;
	lib = library_load("regparm.so", getsym_none);
	int res = 0;
	if (!lib) {
		printf("ERROR: Can't load regparm.so\n");
		return 1;
	}
    __attribute__((regparm(3)))
	int (*add)(int, int, int);
    __attribute__((regparm(3)))
	int (*xor)(int, int, int);
	add = library_getsym(lib, "add");
	if (!add) {
		printf("ERROR: Can't find add\n");
		return 1;
	}
	xor = library_getsym(lib, "xor");
	if (!xor) {
		printf("ERROR: Can't find xor\n");
		return 1;
	}
	int sum = add(13, 17, 65);
	if (sum != 13+17+65) {
		printf("ERROR: 13 + 17 + 65 = %d\n", sum);
		return 1;
	}
	int xor_res = xor(0xfefefefe, 0x12121212, 0x12345678);
	if (xor_res != (0xfefefefe^0x12121212^0x12345678)) {
		printf("ERROR: xor doesn't work\n");
		return 1;
	}
	printf("OK\n");
	return 0;
}
