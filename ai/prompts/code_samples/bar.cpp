#include <ripple.h>
#include <stdint.h>

uint8_t bar(int W, uint8_t a[W][W], uint8_t b[W][W]) {
  ripple_block_t BS = ripple_set_block_shape(0, 64, 64);
  size_t v_row = ripple_id(BS, 0);
  size_t v_col = ripple_id(BS, 1);
  size_t nrows = ripple_get_block_size(BS, 0);
  size_t ncols = ripple_get_block_size(BS, 1);

  for (int r0 = 0; r0 < W; r0 += (int)nrows) {
    int r = r0 + (int)v_row;
    if (r >= W) continue;
    for (int c0 = 0; c0 < W; c0 += (int)ncols) {
      int c = c0 + (int)v_col;
      if (c < W) {
        // Coalesced read from b[r][c] across lanes varying v_col
        a[c][r] = b[r][c];
      }
    }
  }

  return 0;
}