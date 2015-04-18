#define DEBUG
#include "loader.h"
#include "debug.h"
#include "stdlib.h"

void * getsym(const char * name) {
	return NULL;
}

int main(int argc, char const *argv[])
{

	WHEN(argc != 2, _InvalidArgs_, "Please provide a binary name");

	struct library *lib = library_load(argv[1], &getsym);

	int t;
	scanf("%d", &t);

	return 0;

	_InvalidArgs_:
	return 1;
}