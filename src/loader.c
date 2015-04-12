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
	Elf32_Off elfSize;
	int res;
	Elf32_Half i;

	elfHandle = fopen(name, "r");
	WHEN(elfHandle == NULL, _ElfOpenFailed_, "Failed to open ELF file.");

	res = fileSize(elfHandle, &elfSize);
	WHEN(res != 0, _ElfReadFailed_, "cannot get file size");

	res = readHeader(elfHandle, elfSize, &elfHeader);
	WHEN(res != 0, _ElfReadFailed_, "invalid ELF header");

	/* TODO move this to separate procedure */
	res = readSegmentTable(elfHandle, elfSize, elfHeader, &elfSegmentTable);
	WHEN(res != 0, _ElfReadFailed_, "invalid ELF segment table");

	LOGM("%zu", (size_t)elfHeader);
	LOGM("E phnum: %zu", elfHeader->e_phnum);


	for (i = 0; i < elfHeader->e_phnum; ++i) {
//		LOGM("&elfSegmentTable[%zu] = %zu", i, (size_t)&elfSegmentTable[i]);
//		LOGM("type: %zu", elfSegmentTable[i].p_type);
		switch ((elfSegmentTable + i)->p_type) {
			case PT_LOAD:
				LOGM("#%d -> PT_LOAD", i);
				break;
			case PT_DYNAMIC:
				LOGM("#%d -> PT_DYNAMIC", i);
				break;
			default:
				LOGM("#%d -> %zu", i, elfSegmentTable[i].p_type);
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

int fileSize(FILE *handle, Elf32_Off *size) {
	int res;
	int ret;
	long tmpSize;

	ret = 1;

	res = fseek(handle, 0, SEEK_END);
	WHEN(res != 0, _Fail_, "fseek failed");

	tmpSize = ftell(handle);
	WHEN(tmpSize < 0, _Fail_, "ftell failed");

	rewind(handle);

	*size = (Elf32_Off) tmpSize;
	ret = 0;

	_Fail_:
	return ret;
}

int isHeaderValid(Elf32_Ehdr *header) {
	int ret, i;

	ret = 0;

	for (i = 0; i < SELFMAG; ++i)
		WHEN(((unsigned char *) header)[i] != ELFMAG[i], _Fail_, "Invalid magic number");

	WHEN(((char *)header)[EI_CLASS] != ELFCLASS32, _Fail_, "This is not a 32-bit ELF file");

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

	bytesRead = fread(tmpElfHeader, 1, sizeof(Elf32_Ehdr), elfHandle);
	LOGM(" bytes read: %zu", bytesRead);
	LOGM("header size: %zu", sizeof(Elf32_Ehdr));
	WHEN(bytesRead != sizeof(Elf32_Ehdr), _Fail_, "fread failed");

	WHEN(!isHeaderValid(tmpElfHeader), _Fail_, "elf header is not valid");

	*elfHeader = tmpElfHeader;
	ret = 0;

	_Fail_:
	if (ret != 0)
		free(tmpElfHeader);

	_InvalidSize_:
	return ret;
}

int readSegmentTable(FILE *elfHandle, Elf32_Off elfSize, Elf32_Ehdr *elfHeader, Elf32_Phdr **elfSegmentsTable) {
	size_t rowsRead;
	Elf32_Phdr *tmpElfSegmentsTable;
	Elf32_Off end;

	fseek(elfHandle, elfHeader->e_phoff, SEEK_SET);
	/* TODO error handling */

	/* All values are unsigned, should not overflow */
	end = elfHeader->e_phoff + elfHeader->e_phnum * elfHeader->e_phentsize;

	LOGM("elf size: %zu, program section: [%zu; %zu]", elfSize, elfHeader->e_phoff, end);
	LOGM("phnum: %zu", elfHeader->e_phnum);
	LOGM("%zu", (size_t)elfHeader);
	WHEN(elfSize < end, _InvalidSize_, "ELF file too small");

	tmpElfSegmentsTable = malloc(elfHeader->e_phentsize * elfHeader->e_phnum);
	WHEN(tmpElfSegmentsTable == NULL, _Fail_, "malloc failed");

	rowsRead = fread(tmpElfSegmentsTable, elfHeader->e_phentsize, elfHeader->e_phnum, elfHandle);
	WHEN(rowsRead != elfHeader->e_phnum, _Fail_, "cannot read ELF segments table");

	*elfSegmentsTable = tmpElfSegmentsTable;

	return 0;

	_Fail_:
		free(tmpElfSegmentsTable);
	_InvalidSize_:
		return 1;
}
