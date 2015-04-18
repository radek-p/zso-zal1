#pragma once

#include <elf.h>
#include <stdlib.h>
#include <stdio.h>

#define DEBUG

// Struct describing handle for a dynamic library
struct library {
	// Whole Elf file mapped to memory.
	void *pFile;

	// Elf file size.
	Elf32_Off uFileSize;

	// Libraty's segments mapping.
	void *pSMap;

	// Mapping size
	Elf32_Off uSMapSize;

	// Virtual address corresponding to pSMap
	// If minimal virt. addr. of prog. header is < page size, it will be 0.
	Elf32_Addr uSMapVA;

	// Address of Elf header (equal to pFile, but different type)
	Elf32_Ehdr *pEhdr;

	// Array of program headers
	Elf32_Phdr *pPhdrs;

	Elf32_Dyn *pDyn;

	// System's page size
	Elf32_Off uPageSize;

	// User provided function for symbol resolution.
	void *(*pGetSym)(char const *);
};

struct library* initLibStruct(void *(*getsym)(char const *));

int loadElfFile(struct library *lib, int fd);

int checkElfHeader(struct library *lib);

int mapSegments(struct library *lib, int fd);

int prepareMapInfo(struct library *lib);

int doRelocations(struct library *lib);

int fileSize(int fd, off_t *size);

//int readHeader(FILE *elfHandle, Elf32_Off elfSize, Elf32_Ehdr **elfHeader);

int readProgramHeaderTable(FILE *elfHandle, Elf32_Off elfSize, Elf32_Ehdr *elfHeader, Elf32_Phdr **elfSegmentsTable);
