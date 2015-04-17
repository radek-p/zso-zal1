#pragma once

#include <elf.h>
#include <stdlib.h>
#include <stdio.h>

#define DEBUG

// Struct describing handle for a dynamic library
struct library {
	// Whole ELF file mapped to memory.
	void *elfFile;

	// Libraty's segments mapping.
	void *sgmMapp;

	// User provided function for symbol resolution.
	void *(*resolveSymbol)(char const *);
};

int fileSize(FILE *handle, Elf32_Off *size);

int readHeader(FILE *elfHandle, Elf32_Off elfSize, Elf32_Ehdr **elfHeader);

int readProgramHeaderTable(FILE *elfHandle, Elf32_Off elfSize, Elf32_Ehdr *elfHeader, Elf32_Phdr **elfSegmentsTable);

int isHeaderValid(Elf32_Ehdr *header);
