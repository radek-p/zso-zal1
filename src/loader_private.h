#pragma once

#include <elf.h>
#include <stdlib.h>
#include <stdio.h>

#define DEBUG

// Struct describing handle for a dynamic library
struct library {
	// Whole Elf file mapped to memory.
	char *pFile;

	// Elf file size.
	Elf32_Off uFileSize;

	// Libraty's segments mapping.
	char *pSMap;

	// Mapping size
	Elf32_Off uSMapSize;

	// Virtual address corresponding to pSMap
	// If minimal virt. addr. of prog. header is < page size, it will be 0.
	Elf32_Addr uSMapVA;

	// Address of Elf header (equal to pFile, but different type)
	Elf32_Ehdr *pEhdr;

	// Array of program headers
	Elf32_Phdr *pPhdrs;

	// Pointer to PT_DYNAMIC table
	Elf32_Dyn *pDyn;

	// Length of PT_DYNAMIC table
	Elf32_Word uDynSize;

	// System's page size
	Elf32_Off uPageSize;

	// User provided function for symbol resolution.
	void *(*pGetSym)(char const *);
};

struct library* initLibStruct(void *(*getsym)(char const *));

int loadElfFile(struct library *lib, int fd);

int checkElfHeader(struct library *lib);

int mapSegments(struct library *lib, FILE *file);

int prepareMapInfo(struct library *lib);

int doRelocations(struct library *lib);

int prepareDynamicInfo(struct library *lib);

int fileSize(int fd, off_t *size);