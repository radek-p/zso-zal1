#include "loader.h"
#include "hello.h"
#include <stdio.h>

const char *gethellostr() {
	return "world";
}

void *getsym_hello(const char *name) {
	if (!strcmp(name, "gethellostr"))
		return gethellostr;
	if (!strcmp(name, "printf"))
		return printf;
	return 0;
}
int main() {
	struct library *lib;
	lib = library_load("hello.so", getsym_hello);
	if (!lib) {
		printf("ERROR: Can't load hello.so\n");
		return 1;
	}
	void (*hello)();
	hello = library_getsym(lib, "hello");
	if (!hello) {
		printf("ERROR: Can't find hello\n");
		return 1;
	}
	hello();
	return 0;
}
