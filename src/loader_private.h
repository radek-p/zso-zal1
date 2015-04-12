#pragma once

#include <elf.h>
#include <stdlib.h>
#include <stdio.h>

#define DEBUG

/* Struct describing handle for a dynamic library */
struct library {
	/* Library's elf file mapped to memory. If it is not NULL, the header will be already present. */
	char *elfFile_buffer;

	/* Function resolving local program symbols */
	void *(*resolveSymbol)(char const *);
};


/* Pseudo code
 *
 * 1. Open a given elf file
 *
 */

/* Elf32_Ehdr * mapElfFile(const char * name); */
int fileSize(FILE *handle, Elf32_Off *size);

int readHeader(FILE *elfHandle, size_t elfSize, Elf32_Ehdr **elfHeader);

int readSegmentTable(FILE *elfHandle, Elf32_Off elfSize, Elf32_Ehdr *elfHeader, Elf32_Phdr **elfSegmentsTable);

int isHeaderValid(Elf32_Ehdr *header);
