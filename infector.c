#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "debug.h"

int get_shdr_table(void *elf_header, Elf64_Shdr **shdr_table)
{
	Elf64_Ehdr *header = (Elf64_Ehdr *)elf_header;
	if (header->e_shnum == 0)
		return 0;

	*shdr_table = (Elf64_Shdr *)((unsigned char *)header
						+ header->e_shoff);
	return header->e_shnum;
}


int get_phdr_table(void *elf_header, Elf64_Phdr **phdr_table)
{
	Elf64_Ehdr *header = (Elf64_Ehdr *)elf_header;
	if (header->e_phnum == 0)
		return 0;

	*phdr_table = (Elf64_Phdr *)((unsigned char *)header
						+ header->e_phoff);
	return header->e_phnum;
}

int get_file_size(const char *file_path)
{
	struct stat st;
	if (stat(file_path, &st) != 0) {
		perror("stat");
		return 0;
	}
	return st.st_size;
}


Elf64_Phdr *get_last_exec_segment(void *elf)
{
	int i;
	int phdr_size;
	Elf64_Phdr *phdr_table;

	int last_seg_index = -1;
	unsigned long max_vaddr = 0;

	phdr_size = get_phdr_table(elf, &phdr_table);
	if (phdr_size == 0)
		return NULL;

	for ( i = 0; i < phdr_size; i++) {
		Elf64_Phdr tmp = phdr_table[i];
		if (tmp.p_type != PT_LOAD)
			continue;
		if ((tmp.p_flags & PF_X) == 0)
			continue;
		if (tmp.p_vaddr > max_vaddr)
			last_seg_index = i;
		debug("p_vaddr is %lx\n", tmp.p_vaddr);
	}

	if (last_seg_index == -1)
		return NULL;
	return &phdr_table[last_seg_index];
}


unsigned long  get_align_size(unsigned long old_size, unsigned add_size, unsigned align)
{
	
}

void generate_virus_elf(void *elf, int elf_size, void *virus, int virus_size)
{
	unsigned long offset;
	unsigned long align;
	Elf64_Phdr *target_segment;
	void *virus_elf;

	if(
	target_segment = get_last_exec_segment(elf);
	if (target_segment == NULL)
		return 0;

	align = target_segment->p_align;
	return;
}


int main(int argc, char **argv)
{
	int fd;
	int file_size;
	int virus_size;

	void *elf_handle;
	void *virus_handle;

	if (argc !=2 ) {
		printf("please input the elf to be infected!\n");
		return -1;
	}

	file_size = get_file_size(argv[1]);
	if (file_size == 0)
		return -1;
	debug("file %s size is %d\n", argv[1], file_size);
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	elf_handle = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (elf_handle == MAP_FAILED) {
		perror("mmap");
		return -1;
	}

	// virus_size = get_file_size();
	virus_size = 0x200200;
	//
	virus_handle = NULL;
	debug("elf_handle is %p\n", elf_handle);

	generate_virus_elf(elf_handle, file_size, virus_handle, virus_size);

	munmap(elf_handle, file_size);
	close(fd);
	return 0;
}
