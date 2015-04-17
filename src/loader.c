#include <elf.h>
#include "loader.h"
#include "loader_private.h"
#include "debug.h"


struct library *library_load(const char *name, void *(*getsym)(char const *)) {

	FILE *elfHandle;
	struct library *lib = NULL;
	struct library *ret = NULL;
	Elf32_Ehdr *elfHeader = NULL;
	Elf32_Phdr *elfPHTable = NULL;
	Elf32_Off elfSize;
	Elf32_Half i;
	Elf32_Addr minVAddr, maxVAddr;
	int res;

	elfHandle = fopen(name, "r");
	WHEN(elfHandle == NULL, _ElfOpenFailed_, "Failed to open ELF file.");

	res = fileSize(elfHandle, &elfSize);
	WHEN(res != 0, _ElfReadFailed_, "cannot get file size");

	res = readHeader(elfHandle, elfSize, &elfHeader);
	WHEN(res != 0, _ElfReadFailed_, "invalid ELF header");
	WHEN(elfHeader->e_phnum <= 0, _ElfPHTableReadFailed_, "no entries in PH table");

	/* TODO move this to separate procedure */
	res = readProgramHeaderTable(elfHandle, elfSize, elfHeader, &elfPHTable);
	WHEN(res != 0, _ElfReadFailed_, "invalid ELF segment table");

	LOGM("There are %zu program headers:", elfHeader->e_phnum);

	minVAddr = elfPHTable[0].p_vaddr;
	maxVAddr = elfPHTable[0].p_vaddr + elfPHTable[0].p_memsz;

	for (i = 0; i < elfHeader->e_phnum; ++i) {
		/* TODO What about PT_DYNAMIC ? */
		if (elfPHTable[i].p_type == PT_LOAD) {
			if (elfPHTable[i].p_vaddr < minVAddr)
				minVAddr = elfPHTable[i].p_vaddr;

			if (elfPHTable[i].p_vaddr + elfPHTable[i].p_memsz > maxVAddr)
				maxVAddr = elfPHTable[i].p_vaddr + elfPHTable[i].p_memsz;

			LOGM("      [%zu, %zu] (size: %zu)", elfPHTable[i].p_vaddr, elfPHTable[i].p_vaddr + elfPHTable[i].p_memsz, elfPHTable[i].p_memsz);
		}
		switch ((elfPHTable + i)->p_type) {
			case PT_LOAD:
				LOGM("  #%d: PT_LOAD", i);
				break;
			case PT_DYNAMIC:
				LOGM("  #%d: PT_DYNAMIC", i);
				break;
			default:
				LOGM("  #%d: %zu", i, elfPHTable[i].p_type);
		}
	}

	LOGM("map size: [%zu, %zu]", minVAddr, maxVAddr);

	/* TODO */
	/* Map ELF to memory */
	/* preform needed relocations */
	/* ... */
	lib = malloc(sizeof(struct library));
	lib->resolveSymbol = getsym;


	ret = lib;
	free(elfPHTable);

	_ElfPHTableReadFailed_:
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

/* Oblicza rozmiar pliku i wykonuje fseek na początek pliku.
 * Przy braku błędów zwraca 0. */
int fileSize(FILE *handle, Elf32_Off *size) {
	int res;
	long tmpSize;

	res = fseek(handle, 0L, SEEK_END);
	WHEN(res != 0, _Fail_, "fseek failed"); // OK

	tmpSize = ftell(handle);
	WHEN(tmpSize < 0, _Fail_, "ftell failed"); // OK

	res = fseek(handle, 0L, SEEK_SET);
	WHEN(res != 0, _Fail_, "fseek failed"); // OK

	*size = (Elf32_Off) tmpSize;
	return 0;

	_Fail_:
	return 1;
}

/* Sprawdza poprawność nagłówka pliku ELF
 * Jeśli nagłówek jest poprawny, zwraca 1, wpp. 0 */
int isHeaderValid(Elf32_Ehdr *header) {
	int i;

	for (i = 0; i < SELFMAG; ++i)
		WHEN(header->e_ident[i] != ELFMAG[i], _Fail_, "wrong magic number");

	WHEN(header->e_ident[EI_CLASS  ] != ELFCLASS32   , _Fail_, "unsupported architecture (not 32 bit)");
	WHEN(header->e_ident[EI_OSABI  ] != ELFOSABI_SYSV, _Fail_, "unsupported operating system");
	WHEN(header->e_ident[EI_DATA   ] != ELFDATA2LSB  , _Fail_, "unsupported endianess");
	WHEN(header->e_ident[EI_VERSION] != EV_CURRENT   , _Fail_, "unsupported elf version");

	WHEN(header->e_machine != EM_386    , _Fail_, "e_machine != EM_386");
	WHEN(header->e_type    != ET_DYN    , _Fail_, "wrong elf type (not ET_DYN)");
	WHEN(header->e_version != EV_CURRENT, _Fail_, "wrong elf version");

	return 1;

	_Fail_:
	return 0;
}

/* Wczytuje nagłówek pliku ELF do pamięci. */
int readHeader(FILE *elfHandle, Elf32_Off elfSize, Elf32_Ehdr **elfHeader) {
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

int readProgramHeaderTable(FILE *elfHandle, Elf32_Off elfSize, Elf32_Ehdr *elfHeader, Elf32_Phdr **elfSegmentsTable) {
	size_t rowsRead;
	Elf32_Phdr *tmpElfSegmentsTable;
	Elf32_Off end;
	int res;

	res = fseek(elfHandle, elfHeader->e_phoff, SEEK_SET);
	WHEN(res != 0, _InvalidSize_, "fseek failed");

	/* All values are unsigned, should not overflow */
	end = elfHeader->e_phoff + elfHeader->e_phnum * elfHeader->e_phentsize;

	LOGM("elf size: %zu, program section: [%zu; %zu]", elfSize, elfHeader->e_phoff, end);
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
