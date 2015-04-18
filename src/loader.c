#define _GNU_SOURCE
#include <elf.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
//#include <sys/types.h>
#include <fcntl.h>
#include "loader.h"
#include "loader_private.h"
#include "debug.h"


//struct library *library_load2(const char *name, void *(*getsym)(char const *)) {
//
//	FILE *elfHandle;
//	struct library *lib;
//	Elf32_Ehdr *elfHeader = NULL;
//	Elf32_Phdr *elfPHTable = NULL;
//	Elf32_Off elfSize;
//	Elf32_Half i;
//	Elf32_Addr minVAddr, maxVAddr;
//	int res;
//
//	elfHandle = fopen(name, "r");
//	WHEN(elfHandle == NULL, _ElfOpenFailed_, "Failed to open ELF file.");
//
//	res = fileSize(elfHandle, &elfSize);
//	WHEN(res != 0, _ElfReadFailed_, "cannot get file size");
//
//	res = readHeader(elfHandle, elfSize, &elfHeader);
//	WHEN(res != 0, _ElfReadFailed_, "invalid ELF header");
//	WHEN(elfHeader->e_phnum <= 0, _ElfPHTableReadFailed_, "no entries in PH table");
//
//	/* TODO move this to separate procedure */
//	res = readProgramHeaderTable(elfHandle, elfSize, elfHeader, &elfPHTable);
//	WHEN(res != 0, _ElfReadFailed_, "invalid ELF segment table");
//
//	LOGM("There are %zu program headers:", elfHeader->e_phnum);
//
//	minVAddr = elfPHTable[0].p_vaddr;
//	maxVAddr = elfPHTable[0].p_vaddr + elfPHTable[0].p_memsz;
//
//	for (i = 0; i < elfHeader->e_phnum; ++i) {
//		/* TODO What about PT_DYNAMIC ? */
//		if (elfPHTable[i].p_type == PT_LOAD) {
//			if (elfPHTable[i].p_vaddr < minVAddr)
//				minVAddr = elfPHTable[i].p_vaddr;
//
//			if (elfPHTable[i].p_vaddr + elfPHTable[i].p_memsz > maxVAddr)
//				maxVAddr = elfPHTable[i].p_vaddr + elfPHTable[i].p_memsz;
//
//			LOGM("      [%zu, %zu] (size: %zu)", elfPHTable[i].p_vaddr, elfPHTable[i].p_vaddr + elfPHTable[i].p_memsz, elfPHTable[i].p_memsz);
//		}
//		switch ((elfPHTable + i)->p_type) {
//			case PT_LOAD:
//				LOGM("  #%d: PT_LOAD", i);
//				break;
//			case PT_DYNAMIC:
//				LOGM("  #%d: PT_DYNAMIC", i);
//				break;
//			default:
//				LOGM("  #%d: %zu", i, elfPHTable[i].p_type);
//		}
//	}
//
//	LOGM("map size: [%zu, %zu]", minVAddr, maxVAddr);
//
//	/* TODO */
//	/* Map ELF to memory */
//	/* preform needed relocations */
//	/* ... */
//	lib = malloc(sizeof(struct library));
//	lib->pGetSym = getsym;
//
//
//	free(elfPHTable);
//
//	_ElfPHTableReadFailed_:
//	free(elfHeader);
//
//	_ElfReadFailed_:
//	fclose(elfHandle);
//
//	_ElfOpenFailed_:
//	return lib;
//}

struct library *library_load(const char *name, void *(*getsym)(char const *)) {

	struct library *lib = initLibStruct(getsym);
	WHEN(lib == NULL, _Fail_, "init failed");

	int fd = open(name, O_RDONLY);
	WHEN(fd == -1, _FreeLib_, "open failed");

	int res = loadElfFile(lib, fd);
	WHEN(res != 0, _CloseFd_, "failed to load Elf file");

	res = mapSegments(lib, fd);
	WHEN(res != 0, _UnmapFile_, "failed to map program segments");

	res = doRelocations(lib);
	WHEN(res != 0, _UnmapSegments_, "failed to perform relocations");

	return lib; // Success

	_UnmapSegments_:
		munmap(lib->pSMap, (size_t) lib->uSMapSize);

	_UnmapFile_:
		munmap(lib->pFile, (size_t) lib->uFileSize);

	_CloseFd_:
		close(fd);

	_FreeLib_:
		free(lib);

	_Fail_:
		return NULL;
}

void *library_getsym(struct library *lib, const char *name) {
	/* Return pointer to symbol
	 * It will be either:
	 *  a) constant,
	 *  b) pointer to function
	 *  */
	return NULL;
}

struct library *initLibStruct(void *(*getsym)(char const *)) {

	LOG("Initializing lib struct");

	struct library *lib = malloc(sizeof(struct library));
	WHEN(lib == NULL, _Fail_, "malloc failed");

	lib->pGetSym = getsym;
	lib->pDyn = NULL;
	return lib;

	_Fail_:
		return NULL;
}

int loadElfFile(struct library *lib, int fd)
{
	LOG("Mapping elf file to memory");

	off_t size;
	int res = fileSize(fd, &size);
	WHEN(res != 0, _Fail_, "fileSize failed");
	WHEN(size < 0, _Fail_, "invalid file size");

	lib->uFileSize = (Elf32_Off) size;

	lib->pFile = mmap(NULL, (size_t) lib->uFileSize, PROT_READ, MAP_PRIVATE, fd, 0);
	WHEN(lib->pFile == MAP_FAILED, _Fail_, "mmap failed");

	res = close(fd);
	WHEN(res != 0, _UnmapFile_, "close failed");

	lib->pEhdr = (Elf32_Ehdr *) lib->pFile;

	res = checkElfHeader(lib);
	WHEN(res != 0, _UnmapFile_, "invalid file format");

	Elf32_Off phdrsOffset = lib->pEhdr->e_phoff + lib->pEhdr->e_phentsize * lib->pEhdr->e_phnum;
	WHEN(phdrsOffset > lib->uFileSize, _UnmapFile_, "file too small for program header table");

	lib->pPhdrs = (Elf32_Phdr *) (lib->pFile + lib->pEhdr->e_phoff);

	return 0;

	_Fail_:
		return 1;

	_UnmapFile_:
		// Do not check return value, we already have problems.
		munmap(lib->pFile, (size_t) lib->uFileSize);
		return 2;
}

int checkElfHeader(struct library *lib)
{
	WHEN(lib->uFileSize < (sizeof(Elf32_Ehdr)), _Fail_, "file too small for elf header");

	for (int i = 0; i < SELFMAG; ++i)
		WHEN(lib->pEhdr->e_ident[i] != ELFMAG[i], _Fail_, "wrong magic number");

	WHEN(lib->pEhdr->e_ident[EI_CLASS  ] != ELFCLASS32   , _Fail_, "unsupported architecture (not 32 bit)");
	WHEN(lib->pEhdr->e_ident[EI_OSABI  ] != ELFOSABI_SYSV, _Fail_, "unsupported operating system");
	WHEN(lib->pEhdr->e_ident[EI_DATA   ] != ELFDATA2LSB  , _Fail_, "unsupported endianess");
	WHEN(lib->pEhdr->e_ident[EI_VERSION] != EV_CURRENT   , _Fail_, "unsupported elf version");
	WHEN(lib->pEhdr->e_machine != EM_386    , _Fail_, "e_machine != EM_386");
	WHEN(lib->pEhdr->e_type    != ET_DYN    , _Fail_, "wrong elf type (not ET_DYN)");
	WHEN(lib->pEhdr->e_version != EV_CURRENT, _Fail_, "wrong elf version");

	return 0;

	_Fail_:
	return 1;
}

int mapSegments(struct library *lib, int fd)
{
	LOG("Mapping segments");

	int res = prepareMapInfo(lib);
	WHEN(res != 0, _Fail_, "couldn't prepare to do mmap");

	// TODO Change default protection level.
	lib->pSMap = mmap(NULL, (size_t) lib->uSMapSize, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, (off_t) lib->uSMapSize);
	WHEN(lib->pSMap == MAP_FAILED, _Fail_, "mmap failed");

	for (Elf32_Half i = 0; i < lib->pEhdr->e_phnum; ++i) {
		Elf32_Off  adjustment   = lib->pPhdrs[i].p_vaddr % lib->uPageSize;
		Elf32_Addr minAligned   = lib->pPhdrs[i].p_vaddr - adjustment;
		Elf32_Off  memSzAligned = lib->pPhdrs[i].p_memsz + adjustment;
		Elf32_Off  fileOffset   = lib->pPhdrs[i].p_offset - adjustment;
		memSzAligned = ((memSzAligned - 1) / lib->uPageSize + 1) * lib->uPageSize;

		LOGM("mmap fix: (%p), %p, %p, %p", lib->pSMap + (minAligned - lib->uSMapVA), minAligned, memSzAligned, fileOffset);

		int protectionLevel = 0;
		if (lib->pPhdrs[i].p_flags & PF_X) protectionLevel |= PROT_EXEC;
		if (lib->pPhdrs[i].p_flags & PF_W) protectionLevel |= PROT_WRITE;
		if (lib->pPhdrs[i].p_flags & PF_R) protectionLevel |= PROT_READ;

		void * res2 = mmap(lib->pSMap + (minAligned - lib->uSMapVA), memSzAligned, protectionLevel, MAP_FIXED, fd, fileOffset);
		WHEN(res2 == MAP_FAILED, _Fail_, "mmap fix failed");
	}


	return 0;
	_Fail_:
		return 1;
}

int prepareMapInfo(struct library *lib)
{
	WHEN(lib->pEhdr->e_phnum < 1, _Fail_, "file does not contain program header table");

	Elf32_Addr minVA = lib->pPhdrs[0].p_vaddr;
	Elf32_Addr maxVA = lib->pPhdrs[0].p_vaddr + lib->pPhdrs[0].p_memsz;

	// Only for PT_LOAD segments are taken into account.
	for (Elf32_Half i = 0; i < lib->pEhdr->e_phnum; ++i) {
		if (lib->pPhdrs[i].p_type == PT_LOAD) {
			if (lib->pPhdrs[i].p_vaddr < minVA)
				minVA = lib->pPhdrs[i].p_vaddr;

			if (lib->pPhdrs[i].p_vaddr + lib->pPhdrs[i].p_memsz > maxVA)
				maxVA = lib->pPhdrs[i].p_vaddr + lib->pPhdrs[i].p_memsz;

			LOGM("segment: [%p, %p]", (void *) lib->pPhdrs[i].p_vaddr, (void *) lib->pPhdrs[i].p_memsz + lib->pPhdrs[i].p_vaddr);
		}

		if (lib->pPhdrs[i].p_type == PT_DYNAMIC) {
			WHEN(lib->pDyn != NULL, _Fail_, "multiple PT_DYNAMIC sections");
			lib->pDyn = (Elf32_Dyn *) lib->pFile + lib->pPhdrs[i].p_offset;
		}
	}

	LOGM("Segment boundaries: [%p, %p]", (void *) minVA, (void *) maxVA);
	LOGM("Page size: %p", (void *) sysconf(_SC_PAGE_SIZE));

	long tmp =  sysconf(_SC_PAGE_SIZE);
	WHEN(tmp == -1, _Fail_, "cannot get page size");
	lib->uPageSize = (Elf32_Off) tmp;

	minVA = lib->uPageSize * ( minVA      / lib->uPageSize    );
	maxVA = lib->uPageSize * ((maxVA - 1) / lib->uPageSize + 1);

	lib->uSMapVA = minVA;
	lib->uSMapSize = maxVA - minVA;

	LOGM("mmap boundaries: [%p, %p]", (void *) minVA, (void *) maxVA);

	return 0;

	_Fail_:
		return 1;
}

int doRelocations(struct library *lib)
{
	LOG("Performing relocations");
	return 0;
}

/* Oblicza rozmiar pliku.
 * Przy braku błędów zwraca 0. */
int fileSize(int fd, off_t *size) {

	struct stat st;

	int res = fstat(fd, &st);
	WHEN(res != 0, _Fail_, "fstat failed");

	*size = st.st_size;
	return 0;

	_Fail_:
		return 1;
}

/* Wczytuje nagłówek pliku ELF do pamięci. */
//int readHeader(FILE *elfHandle, Elf32_Off elfSize, Elf32_Ehdr **elfHeader) {
//	int ret;
//	size_t bytesRead;
//	Elf32_Ehdr *tmpElfHeader;
//
//	ret = 1;
//
//	WHEN(elfSize < sizeof(Elf32_Ehdr), _InvalidSize_, "elf header too small");
//
//	tmpElfHeader = malloc(sizeof(Elf32_Ehdr));
//	WHEN(tmpElfHeader == NULL, _Fail_, "malloc failed");
//
//	bytesRead = fread(tmpElfHeader, 1, sizeof(Elf32_Ehdr), elfHandle);
//	LOGM(" bytes read: %zu", bytesRead);
//	LOGM("header size: %zu", sizeof(Elf32_Ehdr));
//	WHEN(bytesRead != sizeof(Elf32_Ehdr), _Fail_, "fread failed");
//
////	WHEN(!isHeaderValid(tmpElfHeader), _Fail_, "elf header is not valid");
//
//	*elfHeader = tmpElfHeader;
//	ret = 0;
//
//	_Fail_:
//	if (ret != 0)
//		free(tmpElfHeader);
//
//	_InvalidSize_:
//	return ret;
//}

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