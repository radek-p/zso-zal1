#include "loader.h"
#define DEBUG
#include "debug.h"
#include "stdlib.h"

int main(int argc, char const *argv[])
{

	WHEN(argc != 2, _InvalidArgs_, "Please provide a binary name");

	struct library *lib = library_load(argv[1], NULL);

	int t;
	scanf("%d", &t);

	return 0;

	_InvalidArgs_:
	return 1;
}