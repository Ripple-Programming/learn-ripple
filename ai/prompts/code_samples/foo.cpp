#include <ripple.h>
#include <stdint.h>

// Coalesced lane-wise copy from b to a; returns a scalar reduction of last values.
uint8_t foo(int W, uint8_t a[1][W], uint8_t b[1][W]) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  size_t v0 = ripple_id(BS, 0);
  size_t nv0 = ripple_get_block_size(BS, 0);

  uint8_t last = 0;
  for (size_t u = 0; u < (size_t)W; u += nv0) {
    size_t idx = u + v0;
    if (idx < (size_t)W) {
      // Coalesced read/write across lanes: consecutive v0 map to consecutive indices
      uint8_t z = b[0][idx];
      a[0][idx] = z;
      last = z;
    }
  }

  // Produce a scalar deterministically from per-lane values
  uint8_t res = ripple_reduceadd(0b01, last);
  return res;
}
