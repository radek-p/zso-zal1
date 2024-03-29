#pragma once

#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <elf.h>
#include <stdio.h>


// Struct describing a handle for a dynamic library
struct library {
	// Libraty's segments mapping.
	char *pSMap;

	// Mapping size
	Elf32_Off uSMapSize;

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

	// Info form PT_DYNAMIC segment
	Elf32_Word dtPltRelSz;
	Elf32_Rel  *dtJmpRel;
	Elf32_Word *dtPltGot;
	char *dtStrTab;
	Elf32_Sym *dtSymTab;
	Elf32_Rel *dtRel;
	Elf32_Word dtRelSz;
	Elf32_Word dtDynSymTabLength;

	// User provided function for symbol resolution.
	void *(*pGetSym)(char const *);
};

#pragma GCC visibility push(hidden)

void *libraryGetSymAll(struct library *lib, const char *name);

struct library* initLibStruct(void *(*getsym)(char const *));

int loadElfFile(struct library *lib, FILE *file);

int checkElfHeader(struct library *lib);

int mapSegments(struct library *lib, FILE *file);

int prepareMapInfo(struct library *lib);

int doRelocations(struct library *lib);

int doRelocationsFrom(struct library *lib, Elf32_Rel *table, size_t length);

int relocate(struct library *lib, Elf32_Rel *rel);

extern void outerLazyResolve();

void *lazyResolve(struct library *lib, Elf32_Addr relOffset);

int prepareDynamicInfo(struct library *lib);

int fileSize(int fd, off_t *size);

int shouldIgnoreSymbol(Elf32_Sym *sym);

int fixPermissions(struct library *lib);

void align(struct library *lib, Elf32_Phdr *phdr, Elf32_Addr *minAligned,
		   Elf32_Off *memSzAligned, Elf32_Off *fileOffset);

char *mallocAndRead(FILE *file, size_t fileSize, size_t fileOffset, size_t length);

#pragma GCC visibility pop