#define _GNU_SOURCE
#include <elf.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "loader.h"
#include "loader_private.h"
#include "debug.h"


struct library *library_load(const char *name, void *(*getsym)(char const *)) {

	struct library *lib = initLibStruct(getsym);
	WHEN(lib == NULL, _Fail_, "init failed");

	FILE *file = fopen(name, "r");
	WHEN(file == NULL, _FreeLib_, "open failed");

	int res = loadElfFile(lib, fileno(file));
	WHEN(res != 0, _CloseFd_, "failed to load Elf file");

	res = mapSegments(lib, file);
	WHEN(res != 0, _UnmapFile_, "failed to map program segments");

	res = fclose(file);
	WHEN(res != 0, _UnmapSegments_, "failed to close elf file");

	res = doRelocations(lib);
	WHEN(res != 0, _UnmapSegments_, "failed to perform relocations");

	LOGS("Done");

	return lib; // Success

	_UnmapSegments_:
		file = NULL;
		munmap(lib->pSMap, (size_t) lib->uSMapSize);

	_UnmapFile_:
		munmap(lib->pFile, (size_t) lib->uFileSize);

	_CloseFd_:
		if (file != NULL)
			fclose(file);

	_FreeLib_:
		free(lib);

	_Fail_:
		return NULL;
}

void *library_getsym(struct library *lib, const char *name) {

	lib = (void *) lib;
	name = (void *) name;
	for (size_t i = 0; i < lib->dtDynSymTabLength; ++i) {
		Elf32_Sym *sym = &lib->dtSymTab[i];
		if (strcmp(name, &lib->dtStrTab[sym->st_name]) == 0) {
			LOGM("found %s at address %p", name, (void *)(lib->pSMap + sym->st_value));
			return lib->pSMap + sym->st_value;
		}
	}
	return NULL;
}

struct library *initLibStruct(void *(*getsym)(char const *)) {

	LOGS("Initializing lib struct");

	struct library *lib = malloc(sizeof(struct library));
	WHEN(lib == NULL, _Fail_, "malloc failed");

	lib->pGetSym = getsym;
	return lib;

	_Fail_:
		return NULL;
}

int loadElfFile(struct library *lib, int fd)
{
	LOGS("Mapping elf file to memory");

	off_t size;
	int res = fileSize(fd, &size);
	WHEN(res != 0, _Fail_, "fileSize failed");
	WHEN(size < 0, _Fail_, "invalid file size");

	lib->uFileSize = (Elf32_Off) size;

	lib->pFile = mmap(NULL, (size_t) lib->uFileSize, PROT_READ, MAP_PRIVATE, fd, 0);
	WHEN(lib->pFile == MAP_FAILED, _Fail_, "mmap failed");

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

int mapSegments(struct library *lib, FILE *file)
{
	LOGS("Mapping segments");

	int res = prepareMapInfo(lib);
	WHEN(res != 0, _Fail_, "couldn't prepare to do mmap");

	// TODO Change default protection level.
	lib->pSMap = mmap(NULL, (size_t) lib->uSMapSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, (off_t) lib->uSMapSize);
	WHEN(lib->pSMap == MAP_FAILED, _Fail_, "mmap failed");

	for (Elf32_Half i = 0; i < lib->pEhdr->e_phnum; ++i) {

		if (lib->pPhdrs[i].p_type != PT_LOAD)
			continue;

		Elf32_Phdr *phdr = &lib->pPhdrs[i];

		Elf32_Off  adjustment   = phdr->p_vaddr % lib->uPageSize;
		Elf32_Addr minAligned   = phdr->p_vaddr  - adjustment;
		Elf32_Off  memSzAligned = phdr->p_memsz  + adjustment;
		Elf32_Off  fileOffset   = phdr->p_offset - adjustment;
		memSzAligned = ((memSzAligned - 1) / lib->uPageSize + 1) * lib->uPageSize;

		LOGM("mmap fixed: addr: %p", (void *) (lib->pSMap + minAligned));
		LOGM("mmap fixed: begin: %x, size: %x, foffset: %x", minAligned, memSzAligned, fileOffset);

		int protectionLevel = 0;
		if (phdr->p_flags & PF_X) protectionLevel |= PROT_EXEC;
		if (phdr->p_flags & PF_W) protectionLevel |= PROT_WRITE;
		if (phdr->p_flags & PF_R) protectionLevel |= PROT_READ;

		void * res2 = mmap(lib->pSMap + minAligned, memSzAligned, protectionLevel, MAP_FIXED | MAP_PRIVATE, fileno(file), fileOffset);
		WHEN(res2 == MAP_FAILED, _UnmapSegments_, "mmap fixed failed");

		if (phdr->p_memsz > phdr->p_filesz) {
			Elf32_Word sizeDiff = phdr->p_memsz - phdr->p_filesz;

			// TODO error handling
			memset(lib->pSMap + phdr->p_vaddr + phdr->p_filesz, 0x0, sizeDiff);

			LOGM("p_memsz: %04x, p_filesz: %04x", phdr->p_memsz, phdr->p_filesz);
			LOGM("memset(%p, %04x, %04x)", (void *) (lib->pSMap + phdr->p_vaddr + phdr->p_filesz), 0x0, sizeDiff);
		}
	}

	return 0;

	_UnmapSegments_:
		munmap(lib->pSMap, lib->uSMapSize);

	_Fail_:
		perror("[ERR]    : ");
		return 1;
}

int prepareMapInfo(struct library *lib)
{
	WHEN(lib->pEhdr->e_phnum < 1, _Fail_, "file does not contain program header table");

	Elf32_Addr maxVA = lib->pPhdrs[0].p_vaddr + lib->pPhdrs[0].p_memsz;

	for (Elf32_Half i = 0; i < lib->pEhdr->e_phnum; ++i) {
		Elf32_Phdr *phdr = &lib->pPhdrs[i];
		// Only for PT_LOAD segments are taken into account.
		if (phdr->p_type == PT_LOAD) {
			if (phdr->p_vaddr + phdr->p_memsz > maxVA)
				maxVA = phdr->p_vaddr + phdr->p_memsz;

			LOGM("segment  : [%04x, %04x]", phdr->p_vaddr, phdr->p_memsz + phdr->p_vaddr);
		}
	}

	LOGM("map area : [%04x, %04x]", 0x0, maxVA);
	LOGM("page size: %p", (void *) sysconf(_SC_PAGE_SIZE));

	long tmp =  sysconf(_SC_PAGE_SIZE);
	WHEN(tmp == -1, _Fail_, "cannot get page size");
	lib->uPageSize = (Elf32_Off) tmp;

	maxVA = lib->uPageSize * ((maxVA - 1) / lib->uPageSize + 1);
	lib->uSMapSize = maxVA;

	LOGM("map area2: [%04x, %04x]", 0x0, maxVA);

	return 0;

	_Fail_:
		return 1;
}

int doRelocations(struct library *lib)
{
	LOGS("Finding dynamic info");

	int res = prepareDynamicInfo(lib);
	WHEN(res != 0, _Fail_, "failed to garther info from PT_DYNAMIC");

	if (lib->dtRel != NULL) {
		LOGS("Performing DT_REL relocations");
		size_t dtRelLength = lib->dtRelSz / sizeof(Elf32_Rel);
		for (Elf32_Half i = 0; i < dtRelLength; ++i) {
			res = relocate(lib, &lib->dtRel[i]);
			WHEN(res != 0, _Fail_, "relocation failed");
		}
	}

	if (lib->dtJmpRel != NULL)
	{
		LOGS("Performing DT_JMPREL relocations");
		size_t dtJmpRelLength = lib->dtPltRelSz / sizeof(Elf32_Rel);
		LOGM("relocations num: %zu", dtJmpRelLength);
		for (Elf32_Half i = 0; i < dtJmpRelLength; ++i)
		{
			res = relocate(lib, &lib->dtJmpRel[i]);
			WHEN(res != 0, _Fail_, "relocation failed");
		}
	}

	return 0;

	_Fail_:
		return 1;
}

int prepareDynamicInfo(struct library *lib)
{
	// Find PT_DYNAMIC segment, fail if the number of that segments is not 1.
	lib->pDyn = NULL;
	for (Elf32_Half i = 0; i < lib->pEhdr->e_phnum; ++i) {
		if (lib->pPhdrs[i].p_type == PT_DYNAMIC) {
			WHEN(lib->pDyn != NULL, _Fail_, "multiple PT_DYNAMIC sections");
			lib->pDyn = (Elf32_Dyn *) (lib->pSMap + lib->pPhdrs[i].p_vaddr);
			lib->uDynSize = lib->pPhdrs[i].p_memsz;
		}
	}
	WHEN(lib->pDyn == NULL, _Fail_, "failed to find PT_DYNAMIC segment");

	lib->dtRel = lib->dtJmpRel = NULL;

	size_t uDynLength = lib->uDynSize / (sizeof(Elf32_Dyn));
	for (size_t i = 0; i < uDynLength && lib->pDyn[i].d_tag != DT_NULL; ++i) {
		Elf32_Dyn *pDyn = &lib->pDyn[i];
		Elf32_Addr addr = pDyn->d_un.d_ptr;
		Elf32_Word val  = pDyn->d_un.d_val;

		switch (pDyn->d_tag) {
			case DT_PLTRELSZ: LOG("DT_PLTRELSZ"); lib->dtPltRelSz = val;                               break;
			case DT_JMPREL:   LOG("DT_JMPREL");   lib->dtJmpRel   = (Elf32_Rel *) (lib->pSMap + addr); break;
			case DT_PLTGOT:   LOG("DT_PLTGOT");   lib->dtPltGot   = lib->pSMap + addr;                 break;
			case DT_STRTAB:   LOG("DT_STRTAB");   lib->dtStrTab   = lib->pSMap + addr;                 break;
			case DT_SYMTAB:   LOG("DT_SYMTAB");   lib->dtSymTab   = (Elf32_Sym *) (lib->pSMap + addr); break;
			case DT_REL:      LOG("DT_REL");      lib->dtRel      = (Elf32_Rel *) (lib->pSMap + addr); break;
			case DT_RELSZ:    LOG("DT_RELSZ");    lib->dtRelSz    = val;                               break;
			case DT_HASH:     LOG("DT_HASH");
				// The Oracle docs say that hash chain number is equal to dynsymtab length.
				lib->dtDynSymTabLength = *((Elf32_Off *)(lib->pSMap + addr + 0x4));
				LOGM("symtab length (from Hash): %04x", lib->dtDynSymTabLength);
				break;
			// Assertions:
			case DT_PLTREL:
				WHEN(val != DT_REL, _Fail_, "unsupported DT_PLTREL value");
				break;
			case DT_RELENT:
				WHEN(val != 0x08, _Fail_, "unsupported DT_RELENT");
				break;
			case DT_SYMENT:
				WHEN(val != 0x10, _Fail_, "unsupported DT_SYMENT");
				break;
			default:
				LOG("unknown symbol DT_*");
				break;
		}
	}
	return 0;

	_Fail_:
		return 1;
}

int relocate(struct library *lib, Elf32_Rel *rel)
{
	// TODO checks
	Elf32_Half symIdx = (Elf32_Half) ELF32_R_SYM(rel->r_info);
	Elf32_Sym *sym = &lib->dtSymTab[symIdx];

	switch (ELF32_ST_TYPE(sym->st_info)) {
		case STT_NOTYPE:
			LOG("STT_NOTYPE");
			break;
		case STT_FUNC:
			LOG("STT_FUNC");
			break;
		case STT_OBJECT:
			LOG("STT_OBJECT");
			break;
		default:
			LOG("IGNORED A SYMBOL!");
			return 1;
	}

	LOGM("defined in section: %x", sym->st_shndx);

	if (sym->st_shndx == SHN_UNDEF) {
		LOG("SHN_UNDEFINED");
		return 0;
	}

	if (sym->st_shndx >= SHN_LORESERVE) {
		LOG("> SHN_LORESERVE");
		return 1;
	}

	char *name = &lib->dtStrTab[sym->st_name];
	LOGM("relocating %p", (void *)name);
//	LOGM("relocating %s", name);
	LOGM("rel addr: %p", rel);

	switch (ELF32_R_TYPE(rel->r_info)) {
		case R_386_32:
			LOG("R_386_32");
			*(void **)(lib->pSMap + rel->r_offset) = lib->pGetSym(name);
			break;
		case R_386_JMP_SLOT:
			*(void **)(lib->pSMap + rel->r_offset) = lib->pGetSym(name);
			LOG("R_386_JMP_SLOT");
			break;
		case R_386_PC32:
			LOG("R_386_PC32");
			break;
		case R_386_GLOB_DAT:
			LOG("R_386_GLOB_DAT");
			break;
		case R_386_RELATIVE:
			LOG("R_386_RELATIVE");
			break;
		default:
			LOG("unsupported relocation type");
			return 1; //TODO
	}

	LOG("Finished");

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