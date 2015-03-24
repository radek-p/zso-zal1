#include <stdio.h>
#include <elf.h>
#include <stdlib.h>

int readElfFile(const char* path);
void printElfHeader32(Elf32_Ehdr* header);
void printElfHeader64(Elf64_Ehdr* header);

int readElfFile(const char* path)
{

	FILE*  elfFile_handle;
	size_t elfFile_size;
	char*  elfFile_buffer;

	printf("Reading file '%s'\n", path);

	/* Open file */
	elfFile_handle = fopen(path, "r");
	if (elfFile_handle == NULL)
		goto _OpenFailed;

	/* Obtain file size */
	if (fseek(elfFile_handle, 0, SEEK_END) != 0)
		goto _FseekFailed;
	elfFile_size = (size_t) ftell(elfFile_handle);
	rewind(elfFile_handle);

	/* Allocate space for file contents in buffer */
	elfFile_buffer = malloc(sizeof(char) * elfFile_size);
	if (elfFile_buffer == NULL)
		goto _MallocFailed;

	/* Read file to buffer */
	if (fread((void *)elfFile_buffer, 1, elfFile_size, elfFile_handle) != elfFile_size)
		goto _FreadFailed;

	/* Check whether this is 32 or 64bit file */
	if (elfFile_size <= EI_CLASS)
		goto _ElfTooShort;

	switch (elfFile_buffer[EI_CLASS]) {
		case ELFCLASS32:
			printf("This is 32-bit elf file.\n");
			if (elfFile_size < sizeof(Elf32_Ehdr))
				goto _ElfTooShort;
			printElfHeader32((Elf32_Ehdr*) elfFile_buffer);
			break;
		case ELFCLASS64:
			printf("This is 64-bit elf file.\n");
			printElfHeader64((Elf64_Ehdr*) elfFile_buffer);
			break;
		default:
			goto _InvalidElfClass;
	}

	free(elfFile_buffer);
	fclose(elfFile_handle);
	return EXIT_SUCCESS;

	/* Error handling */
	_InvalidElfClass:
	_ElfTooShort:
	_FreadFailed:
	free(elfFile_buffer);
	_MallocFailed:
	_FseekFailed:
	fclose(elfFile_handle);
	_OpenFailed:

	return EXIT_FAILURE;
}

void printElfHeader32(Elf32_Ehdr* header)
{
	printf("Elf:\n\ttype:\t%d\n", header->e_type);
}

void printElfHeader64(Elf64_Ehdr* header)
{
	printf("Elf:\n\ttype:\t%d\n", header->e_type);
	printf("64-bit elf files are not supported. Sorry.");
}

int main(int argc, char const *argv[]) {

	if (argc != 2)
		goto _BadArgs;

	if (readElfFile(argv[1]) != 0)
		goto _ReadFailed;

	return 0;

	_ReadFailed:
		return -1;
	_BadArgs:
		printf("Usage: %s <elf path>\n", argv[0]);
		return -1;
}