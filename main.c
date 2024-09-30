#include <gst/gst.h>
#include <syslog.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include "videodecode.h"
//
void get_data(_frame_data_ *d, int n) {
  char name[128];
  snprintf(name, sizeof(name)-1, "%d.bin", n);
  struct stat stb;
  if (stat(name, &stb) != 0) {
    return;
  }
  char buff[65535];
  int fd = open(name, O_RDONLY);
  if (fd != -1) {
    ssize_t len = read(fd, buff, sizeof(buff));
    syslog(LOG_DEBUG, "read %ld\n", len);
    close(fd);
  }
  switch (n) {
  case 1:
    d->sps = new uint8_t [stb.st_size];
    memcpy(d->sps, buff, stb.st_size);
    d->sps_size = stb.st_size;
    break;
  case 2:
    d->pps = new uint8_t [stb.st_size];
    memcpy(d->pps, buff, stb.st_size);
    d->pps_size = stb.st_size;
    break;
  case 3:
    d->idr = new uint8_t [stb.st_size];
    memcpy(d->idr, buff, stb.st_size);
    d->idr_size = stb.st_size;
    break;
  default:
    return;
  }
}
//
int main(int argc, char **argv) {
  openlog(argv[0], LOG_PERROR|LOG_CONS, LOG_USER);
//
  gst_init (&argc, &argv);
  _frame_data_ d;
  get_data(&d, 1); // SPS
  get_data(&d, 2); // PPS
  get_data(&d, 3); // I frame 
//
  CVideoDecode *p = new CVideoDecode();
  if (p) {
    cv::Mat m;
    p->Convert(&d, m);
    delete p;
//
    while (true) {
      cv::imshow("Image", m);
      if (cv::waitKey(1) == 27) {
        break;
      }
    }
  }
  gst_deinit();
  exit(0);
}
