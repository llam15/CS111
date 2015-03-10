#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include "ospfs.h"

int main(int argc, char ** argv) {

  int fd = open("./test/.crash.txt", O_RDONLY | O_CREAT);
  ioctl(fd, IOCTL_NWRITES, atoi(argv[1]));

  close(fd);
}
