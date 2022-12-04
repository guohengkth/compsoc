#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <platform.h>
#include <libuserchannel-arm.h>

int main(int argc, char ** argv) 
{
  if (argc < 3) {
    fprintf(stderr, "Usage: %s [tileid] [pid]\n" , argv[0]);
    return EXIT_FAILURE;
  }
  int tileid = 0;
  if (argc > 1) tileid = atoi(argv[1]);
  if (tileid < 0 || tileid >= NUM_TILES) {
    fprintf(stderr, "Invalid tile id %d\n", tileid );
    return EXIT_FAILURE;
  }

  int pid = 0;
  if (argc > 2) pid = atoi(argv[2]);
  if (pid < 1 || pid >= NUM_PARTITIONS) {
    fprintf(stderr, "Invalid partition id %d\n", pid);
    return EXIT_FAILURE;
  }

  on_arm_initialise (tileid,pid);
  printf("echo mode:\n");
  int c;
  while ((c = getchar()) != EOF) {
    on_arm_snd_from_arm_to_riscv (tileid, pid, (uint8_t) c);
    putchar (on_arm_rcv_from_riscv_to_arm (tileid, pid));
  }
  on_arm_cleanup (tileid,pid);
}

