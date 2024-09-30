#ifndef _CFRAME_DATA_H_INCLUDED
#define _CFRAME_DATA_H_INCLUDED
#include <stdint.h>

typedef struct _frame_data_ {
  uint8_t *sps;
  size_t sps_size;
  uint8_t *pps;
  size_t pps_size;
  uint8_t *idr;
  size_t idr_size;
  _frame_data_() {
    sps = pps = idr = NULL;
    sps_size = pps_size = idr_size = 0;
  }
  ~_frame_data_() {
    if (sps) {
      delete [] sps;
    }
    if (pps) {
      delete [] pps;
    }
    if (idr) {
      delete [] idr;
    }
  }
  void set_sps(void *p, size_t sz) {
    if (0 < sz) {
      if (sps) {
        delete [] sps;
      }
      sps = new uint8_t [sz];
      memcpy(sps, p, sz);
      sps_size = sz;
    }
  }
  void set_pps(void *p, size_t sz) {
    if (0 < sz) {
      if (pps) {
        delete [] pps;
      }
      pps = new uint8_t [sz];
      memcpy(pps, p, sz);
      pps_size = sz;
    }
  }
  void set_idr(void *p, size_t sz) {
    if (0 < sz) {
      if (idr) {
        delete [] idr;
      }
      idr = new uint8_t [sz];
      memcpy(idr, p, sz);
      idr_size = sz;
    }
  }
} S_frame_data;

#endif

