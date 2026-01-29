#include <ripple.h>

#define VECTOR_PE 0

static size_t tile_transpose_src_index(size_t dst_index, size_t /*total*/) {
  constexpr size_t W = 8; // tile width/height
  const size_t r = dst_index / W;
  const size_t c = dst_index % W;
  return c * W + r; // transpose: (r, c) -> (c, r)
}

static int16_t transpose_tile(int16_t *tile_base, size_t v) [[gnu::always_inline]] {
  ripple_block_t BS = ripple_set_block_shape(VECTOR_PE, 64);
  (void)BS; // BS is used to define the SPMD block; index v drives parallelism
  // Each lane loads its element; collectively forms a 64-lane block
  int16_t x = tile_base[v];
  // Shuffle across lanes to produce the transposed layout
  int16_t xT = ripple_shuffle(x, tile_transpose_src_index);
  return xT;
}

void permute(int16_t A[N][N][8][8]) {
  ripple_block_t BS = ripple_set_block_shape(VECTOR_PE, 64);
  size_t v = ripple_id(BS, 0);
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j <= i; ++j) {
      // load a tile, i.e. a contiguous set of 64 elements
      int16_t transposed_1 = transpose_tile(&A[i][j][0][0], v);
      int16_t transposed_2 = transpose_tile(&A[j][i][0][0], v);
      (&A[j][i][0][0])[v] = transposed_1;
      if (i != j)
        (&A[i][j][0][0])[v] = transposed_2;
    }
  }
}
