#include "loader.h"
#include "loader_private.h"

#include <assert.h>
#include <elf.h>

int test_align() {
	struct library lib = { .uPageSize = 0x1000 };
	Elf32_Phdr phdr = { .p_vaddr = 0x999, .p_memsz = 0x1001, .p_offset = 0x999 };
	Elf32_Addr min; Elf32_Off size; Elf32_Off offset;

	align(&lib, &phdr, &min, &size, &offset);
	assert(min == 0x0 && size == 0x2000 && offset == 0x0);

	phdr.p_offset = 0x998;
	phdr.p_vaddr  = 0x998;
	align(&lib, &phdr, &min, &size, &offset);
	assert(min == 0x0 && size == 0x2000 && offset == 0x0);

	return 0;
}

int main() {

	test_align();

	return 0;
}
