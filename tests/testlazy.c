#include "loader.h"
#include <stdio.h>

int fun_called = 0;
int get_called = 0;

void lazy() {
	fun_called++;
}

void *getsym_lazy(const char *name) {
	if (!strcmp(name, "lazy")) {
		get_called++;
		return lazy;
	}
	return 0;
}
int main() {
	struct library *lib;
	lib = library_load("lazy.so", getsym_lazy);
	if (!lib) {
		printf("ERROR: Can't load lazy.so\n");
		return 1;
	}
	void (*call_lazy)();
	call_lazy = library_getsym(lib, "call_lazy");
	if (!call_lazy) {
		printf("ERROR: Can't find call_lazy\n");
		return 1;
	}
	if (fun_called) {
		printf("ERROR: lazy not called yet...\n");
		return 1;
	}
	int is_lazy;
	is_lazy = !get_called;
	call_lazy();
	if (fun_called != 1) {
		printf("ERROR: lazy should be called once...\n");
		return 1;
	}
	if (is_lazy && get_called != 1) {
		printf("ERROR: getsym should be called once...\n");
		return 1;
	}
	call_lazy();
	if (fun_called != 2) {
		printf("ERROR: lazy should be called twice...\n");
		return 1;
	}
	if (is_lazy && get_called != 1) {
		printf("ERROR: getsym should be called once...\n");
		return 1;
	}
	if (is_lazy) {
		printf("lazy OK\n");
	} else {
		printf("non-lazy OK\n");
	}
	return 0;
}
