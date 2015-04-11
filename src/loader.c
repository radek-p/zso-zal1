#include <elf.h>
#include "loader.h"
#include "loader_private.h"
#include "debug.h"


struct library *library_load(const char *name, void *(*getsym)(char const *)) {

	FILE *elfHandle;
	struct library *lib = NULL;
	struct library *ret = NULL;
	Elf32_Ehdr *elfHeader = NULL;
	Elf32_Phdr *elfSegmentTable = NULL;
	size_t elfSize;
	int res;
	unsigned int i;

	elfHandle = fopen(name, "r");
	WHEN(elfHandle == NULL, _ElfOpenFailed_, "Failed to open ELF file.");

	res = fileSize(elfHandle, &elfSize);
	WHEN(res != 0, _ElfReadFailed_, "cannot get file size");

	res = readHeader(elfHandle, elfSize, &elfHeader);
	WHEN(res != 0, _ElfReadFailed_, "invalid ELF header");

	/* TODO move this to separate procedure */
	res = readSegmentTable(elfHandle, elfSize, elfHeader, &elfSegmentTable);
	WHEN(res != 0, _ElfReadFailed_, "invalid ELF segment table");

	for (i = 0; i < elfHeader->e_phnum; ++i) {
		switch (elfSegmentTable[i].p_type) {
			case PT_LOAD:
				LOGM("#%d -> PT_LOAD segment", i);
				break;
			case PT_DYNAMIC:
				LOGM("#%d -> PT_DYNAMIC segment", i);
			default:
				LOGM("#%d -> unknown type segment", i);
		}
	}

	/* TODO */
	/* Map ELF to memory */
	/* preform needed relocations */
	/* ... */
	lib = malloc(sizeof(struct library));
	lib->resolveSymbol = getsym;

	ret = lib;
	free(elfHeader);

	_ElfReadFailed_:
	fclose(elfHandle);

	_ElfOpenFailed_:
	return ret;
}

void *library_getsym(struct library *lib, const char *name) {
	/* Return pointer to symbol
	 * It will be either:
	 *  a) constant,
	 *  b) pointer to function (LAZY!)
	 *  */
	return NULL;
}

int fileSize(FILE *handle, size_t *size) {
	int res;
	int ret;
	long tmpSize;

	ret = 1;

	res = fseek(handle, 0, SEEK_END);
	WHEN(res != 0, _Fail_, "fseek failed");

	tmpSize = ftell(handle);
	WHEN(tmpSize < 0, _Fail_, "ftell failed");

	*size = (size_t) tmpSize;
	ret = 0;

	_Fail_:
	return ret;
}

int isHeaderValid(Elf32_Ehdr *header) {
	int ret, i;

	ret = 0;

	for (i = 0; i < SELFMAG; ++i)
		WHEN(((unsigned char *) header)[i] != ELFMAG[i], _Fail_, "Invalid magic number");

	WHEN(((char *)header)[EI_CLASS] != EI_CLASS, _Fail_, "This is not a 32-bit ELF file");

	/*TODO More checks*/

	ret = 1;

	_Fail_:
	return ret;
}

int readHeader(FILE *elfHandle, size_t elfSize, Elf32_Ehdr **elfHeader) {
	int ret;
	size_t bytesRead;
	Elf32_Ehdr *tmpElfHeader;

	ret = 1;

	WHEN(elfSize < sizeof(Elf32_Ehdr), _InvalidSize_, "elf header too small");

	tmpElfHeader = malloc(sizeof(Elf32_Ehdr));
	WHEN(tmpElfHeader == NULL, _Fail_, "malloc failed");

	LOGM("elfHandleAdress: %d", elfHandle);
	bytesRead = fread(tmpElfHeader, sizeof(Elf32_Ehdr), 1, elfHandle);
	LOGM("bytes read: %zu", bytesRead);
	WHEN(bytesRead != sizeof(Elf32_Ehdr), _Fail_, "cannot read elf header");

	WHEN(isHeaderValid(tmpElfHeader), _Fail_, "elf header is not valid");

	*elfHeader = tmpElfHeader;
	ret = 0;

	_Fail_:
	free(tmpElfHeader);

	_InvalidSize_:
	return ret;
}

int readSegmentTable(FILE *elfHandle, size_t elfSize, Elf32_Ehdr *elfHeader, Elf32_Phdr **elfSegmentsTable) {

	int ret;
	size_t bytesRead;
	Elf32_Phdr *tmpElfSegmentsTable;
	Elf32_Off end;

	ret = 1;
	/* All values are unsigned, should not overflow */
	end = elfHeader->e_phoff + elfHeader->e_phnum * elfHeader->e_phentsize;

	LOGM("check: %zu < %d", elfSize, end);
	WHEN(elfSize < end, _InvalidSize_, "ELF file too small");

	tmpElfSegmentsTable = malloc(sizeof(Elf32_Ehdr));
	WHEN(tmpElfSegmentsTable == NULL, _Fail_, "malloc failed");

	bytesRead = fread(tmpElfSegmentsTable, 1, sizeof(Elf32_Ehdr), elfHandle);
	WHEN(bytesRead != sizeof(Elf32_Ehdr), _Fail_, "cannot read ELF segments table");

	*elfSegmentsTable = tmpElfSegmentsTable;
	ret = 0;

	_Fail_:
	free(tmpElfSegmentsTable);

	_InvalidSize_:
	return ret;
}
