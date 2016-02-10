#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>

#define KEY_PACKET_LEN 14
#define KEY_LEN 10

static void keyreader_process(unsigned char *key) {
  unsigned int key_num;
  char text_key[KEY_LEN + 1];
  if (key[0] != 2) {
    printf("Invalid start byte: %c\n", key[0]);
    return;
  }
  if (key[13] != 3) {
    printf("Invalid end byte: %c\n", key[0]);
    return;
  }
  memset(text_key, 0, sizeof(text_key));
  memcpy(text_key, key + 1, KEY_LEN);
  key_num = strtol(text_key, NULL, 16);
  /*printf("KEY HEX: %.*s DECIMAL: %u\n", KEY_LEN, text_key, key_num);*/
  unsigned long long int now;
  int i;
  int num;
  int packet_checksum;
  int calc_checksum;
  char buf[4];
  calc_checksum = 0;
  packet_checksum = 0;
  for (i = 0; i < 10; i = i + 2) {
     memset(buf, 0, sizeof(buf));
     buf[0] = key[1 + i];
     buf[1] = key[1 + i + 1];
     num = strtol(buf, NULL, 16);
     calc_checksum = calc_checksum ^ num;
  }
  buf[0] = key[11];
  buf[1] = key[12];
  packet_checksum = strtol(buf, NULL, 16);
  if (packet_checksum != calc_checksum) {
    printf("Packet checksum and calculated checksum do not match!\n");
    printf("Calc Checksum: %d Packet Checksum: %d\n", calc_checksum, packet_checksum);
    exit(1);
  }
  now = time(NULL);
  printf("KEY SCAN AT %llu: %d\n", now, key_num);
}

static void usage(char *name) {
  printf("Usage: %s [serial device]\n", name);
}

static int keyreader_close(int fd) {
  return close(fd);
}

static int keyreader_read(int fd) {
  int pos;
  int ret;
  unsigned char buf[KEY_PACKET_LEN];
  pos = 0;
  memset(buf, 0, sizeof(buf));
  for (;;) {
    ret = read(fd, buf + pos, KEY_PACKET_LEN - pos);
    /*printf("Read %d bytes\n", ret);*/
    if (ret < 0) return -1;
    pos += ret;
    if (pos == 14) {
      keyreader_process(buf);
      pos = 0;
    }
  }
  return -1;
}

static int keyreader_open(char *device) {
  int fd;
  printf("Opening: %s\n", device);
  fd = open(device, O_RDWR | O_NOCTTY);
  if (fd == -1) {
    perror(device);
    return -1;
  }
  printf("Opened: %s\n", device);
  return fd;
}

int main(int argc, char *argv[]) {
  int fd;
  int ret;
  ret = 0;
  fd = -1;
  if (argc != 2) {
    usage(argv[0]);
    return 1;
  }
  fd = keyreader_open(argv[1]);
  if (fd == -1) {
    printf("Failed to open serial port.\n");
    return 1;
  }
  keyreader_read(fd);
  keyreader_close(fd);
  return 0;
}