#include "loader.h"
#include "hello.h"
#include <stdio.h>
#include <string.h>

const char *gethellostr() {
	return "world";
}

void *getsym_hello(const char *name) {
	if (!strcmp(name, "gethellostr"))
		return gethellostr;
	if (!strcmp(name, "strlen"))
		return strlen;
	return 0;
}
int main() {
	struct library *lib;
	lib = library_load("textrel.so", getsym_hello);
	if (!lib) {
		printf("ERROR: Can't load textrel.so\n");
		return 1;
	}
	int (*get_var)();
	get_var = library_getsym(lib, "get_var");
	if (!get_var) {
		printf("ERROR: Can't find get_var\n");
		return 1;
	}
    int val = get_var();
    if (val != 3) {
        printf("ERROR: %d != 3\n", val);
        return 1;
    }
	void (*call_and_set)();
	call_and_set = library_getsym(lib, "call_and_set");
	if (!call_and_set) {
		printf("ERROR: Can't find call_and_set\n");
		return 1;
	}
    call_and_set();
    val = get_var();
    if (val != 5) {
        printf("ERROR: %d != 5\n", val);
        return 1;
    }
    printf("OK\n");
	return 0;
}
