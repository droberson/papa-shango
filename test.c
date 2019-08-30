/* test.c -- by Daniel Roberson (daniel @ planethacker . net)
 *        -- Reads shellcode from a file, outputs in C format, and attempts
 *        -- to execute it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

int main (int argc, char *argv[]) {
  int i;
  int fd;
  int len;
  unsigned char *shellcode;
  struct stat s;


  if (!argv[1]) {
    fprintf (stderr, "usage: %s <file>\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  printf ("Testing shellcode contained in %s\n\n", argv[1]);

  if (stat(argv[1], &s) == -1) {
    perror("stat");
    exit (EXIT_FAILURE);
  }

  fd = open (argv[1], O_RDONLY);
  if (fd == -1) {
    perror ("open");
    exit (EXIT_FAILURE);
  }

  shellcode = malloc (s.st_size);

  if (read (fd, shellcode, s.st_size) != s.st_size) {
    fprintf (stderr, "Unable to read %ld bytes from %s\n", s.st_size, argv[1]);
    exit (EXIT_FAILURE);
  }

  close (fd);

  for (i = 0; i < s.st_size; i++) {
    if (shellcode[i] == 0x00) {
      printf ("SHELLCODE CONTAINS NULL BYTES!\n\n");
      break;
    }
  }

  /* display shellcode in C format */
  printf ("char shellcode[] = \"");

  for (i = 0; i < s.st_size; i++) {
    if ((i % 10 == 0) && (i != 0)) {
      printf ("\"\n");
      printf ("                   \"");
    }

    printf ("\\x%.2x", shellcode[i]);
  }

  printf ("\";\n\n");

  printf ("Length: %ld\n\n", s.st_size);

  printf ("Executing shellcode..\n");
  void (*run) () = (void(*)())shellcode;
  run ();
}
