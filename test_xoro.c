/**
 * @file   test_xoro.c
 * @author Dave McEwan
 * @date   2019-11-13
 * @version 0.1
 * @brief  Basic test for reading from /dev/xoroshiro128p.
*/
#include <errno.h> // errno, perror
#include <fcntl.h> // open, O_RDONLY
#include <stdio.h> // printf
#include <stdint.h> // uint64_t
#include <stdlib.h> // size_t
#include <unistd.h> // ssize_t

#define MAX_BYTES_PER_READ 8
static unsigned char rx[MAX_BYTES_PER_READ]; // Receive buffer from the LKM.

void
zero_rx(void)
{
  for (int b_idx=0; b_idx < MAX_BYTES_PER_READ; b_idx++) {
    rx[b_idx] = 0;
  }
}

int
main(int argc, char *argv[])
{

  printf("Starting device test code example...\n");

  int fd = open("/dev/xoroshiro128p", O_RDONLY);
  if (0 > fd){
    perror("Failed to open the device.");
    return errno;
  }

  // Test reading different numbers of bytes.
  for (int n_bytes=0; n_bytes < 10; n_bytes++) {

    zero_rx(); // Clear/zero the buffer before copying in read data.

    ssize_t n_bytes_read = read(fd, rx, n_bytes); // Read the response from the LKM

    if (0 > n_bytes_read){
      perror("Failed to read all bytes.");

      return errno;
    } else {

      uint64_t value_ = 0;
      for (int b_idx=0; b_idx < n_bytes_read; b_idx++) {
        unsigned char b = rx[b_idx];
        value_ |= ((uint64_t)b << (8*b_idx));
        //printf("rx[%d]=%02x\n", b_idx, b);

      }
      printf("n_bytes=%d n_bytes_read=%ld value=%016lx\n",
              n_bytes,   n_bytes_read,    value_);
    }
  }

  return 0;
}
