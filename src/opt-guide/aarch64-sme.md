# 64-bit Arm(R) (AArch64) SME Optimization

Since the Arm(R)v9 architecture,
Arm ISAs include the Scalable Vector Extensions (SVE) and
the Scalable Matrix Extensions (SME).

SME includes native instructions to perform matrix operations
using a two-dimensional SIMD register.
This enables the optimization of matrix operations
including matrix multiplication (using the outer product algorithm),
convolutions, and transpositions.

Ripple allows us to write code run on a multi-dimensional block.
In two dimensions, that block is basically a matrix of SIMD processing elements.
This looks a lot like the SME engine.
Based on that observation, the AArch64 LLVM backend is able to recognize
two-dimensional patterns written in Ripple and render them as SME instructions.

We review these patterns in the next sections.

## Matrix multiplication pattern recognition from ripple_parallel annotations
Ripple enables users to write matrix multiplication in an OpenMP style
using the `ripple_parallel` and `ripple_parallel_full` loop annotations.
Both API operate similarly, with the key difference being that
`ripple_parallel_full` assumes only the full-vector loops
and does not handle leftover elements,
whereas `ripple_parallel` generates both the full-vector loop and the
epilogue to process any remaining elements.

To begin, user specifies a 32x32 block shape using the
`ripple_set_block_shape(SME_LANES, SME_SIZE, SME_SIZE)` API.
Then the `ripple_parallel` annotations are applied to two `for` loops along the
y and x dimensions.
This instructs the Ripple compiler to distribute the processing elements
across the y and x dimensions of 32x32 blocks.
This approach simplifies indexing into matrices A, B and C,
allowing them to be accessed using a flat, non-tiled pointer (e.g., `float *`).
For example, an element of matrix A can be accessed as `A[k * M + i]`,
where `i` is the y-coordinate of the processing element.

Since there is no full tile assumption,
Ripple AArch64 SME compiler generates SVE PSEL (predicate select) instructions
to handle partial tiles.

```C
#include <ripple.h>
#define SME_LANES 0
#define SME_SIZE 32

__arm_locally_streaming __arm_new("za")
void matmul_arg(float *A, float *B, float *C, int M, int N, int K) {
  ripple_block_t sme_block =
    ripple_set_block_shape(SME_LANES, SME_SIZE, SME_SIZE);
  ripple_parallel(sme_block, 1);
  for (int i = 0; i < M; i++) {
    ripple_parallel(sme_block, 0);
    for (int j = 0; j < N; j++) {
      __builtin_assume(K > 0);
      float tile = 0;
      for (int k = 0; k < K; k++) {
        tile += A[k * M + i] * B[k * N + j];
      }
      C[i * N + j] = tile;
    }
  }
}
```

The generated assembly is complex,
so we focus on a segment corresponding to tile 4,
which includes predicate handling for both matrices A and B.
In this segment, four SVE `whilelt` instructions are used
to construct predicate registers for A and B.
These are followed by SVE `psel` instructions,
which generate new predicate registers needed for storing data.
```asm
	...
	fmopa	za0.s, p0/m, p0/m, z17.s, z16.s
	fmopa	za1.s, p0/m, p0/m, z17.s, z19.s
	fmopa	za2.s, p0/m, p0/m, z18.s, z16.s
	fmopa	za3.s, p0/m, p0/m, z18.s, z19.s
	...
.LBB0_4520:                             // %store.preheader4
	...
	whilelt	p8.s, x10, x11
	whilelt	p1.s, x8, x11
	whilelt	p3.s, x8, x12
	whilelt	p2.s, x13, x12
	...
.LBB0_4521:                             // %store.body4
	mov	z0.b, p4/m, za0h.b[w12, 0]
	mov	z1.b, p4/m, za0h.b[w12, 1]
	mov	z2.b, p4/m, za0h.b[w12, 2]
	mov	z3.b, p4/m, za0h.b[w12, 3]
	psel	p5, p2, p8.s[w14, 0]
	psel	p6, p3, p8.s[w14, 0]
	psel	p7, p2, p1.s[w14, 0]
	psel	p0, p3, p1.s[w14, 0]
	...
	st1w	{ z0.s }, p5, [x8]
	st1w	{ z1.s }, p6, [x8, #1, mul vl]
	st1w	{ z2.s }, p7, [x9]
	st1w	{ z3.s }, p0, [x9, #1, mul vl]
	...
```

## Matrix multiplication in SPMD style
The SME pattern-matching also works on SPMD-style matrix multiplications.
To keep the code concise here, we assume that both M and N are divisible by 32,
the user first calls `ripple_set_block_shape`
to set a 2D block shape of 32x32 for the processing elements (SME_LANES).
In this example, we have also optimized the C matrix for stores from the SME
vector register, by tiling its data to the size of that register.
```C
#include <ripple.h>
#include <assert.h>
#define SME_LANES 0
#define SME_SIZE 32

__arm_locally_streaming __arm_new("za")
void gen_matmul_tiled(float A[K][M], float B[K][N],
                      float C[M / SME_SIZE][N / SME_SIZE][SME_SIZE][SME_SIZE]) {
  assert (M % SME_SIZE == 0);
  assert (N % SME_SIZE == 0);
  ripple_block_t sme_block =
    ripple_set_block_shape(SME_LANES, SME_SIZE, SME_SIZE);
  size_t x = ripple_id(sme_block, 0);
  size_t y = ripple_id(sme_block, 1);
  for (int i = 0; i < M / SME_SIZE; i++) {
    for (int j = 0; j < N / SME_SIZE; j++) {
      float tile = 0;
      for (int k = 0; k < K; k++) {
        tile += A[k][SME_SIZE * i + y] * B[k][SME_SIZE * j + x];
      }
      C[i][j][y][x] = tile;
    }
  }
}
```
Using the Ripple AArch64 SME compiler, SME floating-point outer product
and accumulate instructions (FMOPA) are generated,
along with the necessary load and store instructions to transfer data
between memory and the ZA matrix.
For floating-point data types,
the ZA matrix provides four tiles (ZA0 to ZA3) for computation.
The outer product operations are performed across all four tiles.
When storing results from the ZA matrix back to memory,
the data is accessed in byte format using ZA0 as the base index.
The contents of Z0 and Z1 are stored contiguously in memory, as are Z2 and Z3.
```asm
    ...
    zero	{za}
.LBB0_5:                                // %for.body8
    ld1b	{ z0.b }, p0/z, [x0, x18]
    ld1b	{ z1.b }, p0/z, [x17, x18]
    ldr	z2, [x12, #1, mul vl]
    ldr	z3, [x4, #1, mul vl]
    ...
    fmopa	za0.s, p1/m, p1/m, z0.s, z1.s
    fmopa	za1.s, p1/m, p1/m, z0.s, z3.s
    fmopa	za2.s, p1/m, p1/m, z2.s, z1.s
    fmopa	za3.s, p1/m, p1/m, z2.s, z3.s
    b.ne	.LBB0_5
    ...
.LBB0_7:                                // %store.body
    mov	z0.b, p0/m, za0h.b[w12, 0]
    mov	z1.b, p0/m, za0h.b[w12, 1]
    mov	z2.b, p0/m, za0h.b[w12, 2]
    mov	z3.b, p0/m, za0h.b[w12, 3]
    st1b	{ z0.b }, p0, [x13, x18]
    st1b	{ z1.b }, p0, [x14, x18]
    st1b	{ z2.b }, p0, [x15, x18]
    st1b	{ z3.b }, p0, [x16, x18]
    ...
    b.ne	.LBB0_7
    ...
```

## Tile-based transpose pattern recognition
We also taught LLVM to recognize transposition
-- another simple two-dimensional pattern coming out of Ripple code --
and express it in SME.

In the example below, the `transpose_tile` function performs
a tile-wise transpose of a matrix block.
This is achieved using the `ripple_shuffle` API,
where the `row_idx` and `offset` are swapped to perform a tile transpose.
The `transpose_ripple` function leverages this by looping over 32x32 tiles
and applying different row strides to the source and destination matrices.
This enables the correct reordering of elements during the transpose.
Specifically, the input matrix is indexed using row stride `k`,
whereas the output matrix is indexed using row stride `TILE_SIZE 32`.

```C
#include <ripple.h>
#include <assert.h>
#define TILE_SIZE 32

static __attribute__((always_inline)) float transpose_tile(float *tile_addr,
                                                           size_t v) {
  auto transpose = [](size_t k, size_t block_size) -> size_t {
    unsigned offset = k / TILE_SIZE;
    unsigned row_idx = k % TILE_SIZE;
    return row_idx * TILE_SIZE + offset;
  };
  return ripple_shuffle(tile_addr[v], transpose);
}

__arm_locally_streaming __arm_new("za")
void transpose_ripple(float *dest, float *src, unsigned m, unsigned k) {
  assert(m % TILE_SIZE == 0);
  assert(k % TILE_SIZE == 0);
  ripple_block_t sme_block =
    ripple_set_block_shape(0, TILE_SIZE, TILE_SIZE);
  size_t x = ripple_id(sme_block, 0);
  size_t y = ripple_id(sme_block, 1);
  for (int i = 0; i < m; i += TILE_SIZE)
    for (int j = 0; j < k; j += TILE_SIZE) {
      dest[y * TILE_SIZE + x] = transpose_tile(&src[i * k + j], y * k + x);
      dest += TILE_SIZE * TILE_SIZE;
    }
}
```

The compiler code generation for transpose operation is still a
work in progress.
The assembly shown here is generated based on
the expected compiler-transformed IR,
utilizing all four tiles for horizontal loads and vertical stores.
```asm
.LBB0_8:                                // %load.loop
	...
	ld1w	{za0h.s[w15, 0]}, p0/z, [x6]
	ld1w	{za1h.s[w15, 0]}, p0/z, [x19]
	ld1w	{za2h.s[w15, 0]}, p0/z, [x20]
	ld1w	{za3h.s[w15, 0]}, p0/z, [x21]
	...
	b.ne	.LBB0_8
	...
.LBB0_10:                               // %store.loop
	...
	st1w	{za0v.s[w15, 0]}, p0, [x6]
	st1w	{za1v.s[w15, 0]}, p0, [x19]
	st1w	{za2v.s[w15, 0]}, p0, [x20]
	st1w	{za3v.s[w15, 0]}, p0, [x19]
	...
	b.ne	.LBB0_10
```

# Limitations

The work presented in this section is based on pattern matching,
which enables a gentle-slope increase of the set of optimizable codes.
In this section,
we denote some of the known limitations to the current implementation.

## Declare the function to use SME (so-called "streaming mode")
Currently, in order to activate the SME code generation from matrix
multiplication codes in a given function, we need to annotate the function
with
```C
__arm_locally_streaming __arm_new("za")
```
as illustrated in the above examples.

## Enforce zero-initialized muladd accumulator

Unfortunately, there is no single SVE or SME instruction available
to initialize the entire ZA matrix with non-zero values.
As a result, the compiler must generate a loop structure
to load non-zero values from memory into the ZA matrix tiles, slice by slice.
Since the Ripple AArch64 SME compiler already emits a loop structure
to store the final results from the multiply-accumulate operation back to memory,
it's more efficient to reuse this loop and
perform the necessary operations outside the ZA matrix.

The recommended approach is to initialize the accumulator (e.g., `temp`) to zero,
then add the result back to output matrix using:
`C[it * IT + i][jt * JT + j] += temp`.
This avoids initializing values from `C[it * IT + i][jt * JT + j]`
into the ZA matrix.
The operation `+=` is performed outside the ZA matrix,
which aligns better with the current compiler design.
```C
  ...
  if (k == 0)
    C[it * IT + i][jt * JT + j] = 0;
  float temp = 0;
  for (int k = 0; k < KT; ++k) {
    temp += A[kt * KT + k][it * IT + i] * B[kt * KT + k][jt * JT + j];
  }
  C[it * IT + i][jt * JT + j] += temp;
  ...
```
Currently, the support for moving operations outside of the ZA matrix
for non-zero initial values is not yet implemented.
The Ripple AArch64 SME compiler temporarily reject such cases.

Below is an example of initializing non-zero values from
`C[it * IT + i][jt * JT + j]` into accumulator `temp`.
Such usage is not recommended and will be rejected by the
Ripple AArch64 SME compiler.

```C
  ...
  float temp = (kt == 0) ? 0 : C[it * IT + i][jt * JT + j];
  for (int k = 0; k < KT; ++k) {
    temp += A[kt * KT + k][it * IT + i] * B[kt * KT + k][jt * JT + j];
  }
  C[it * IT + i][jt * JT + j] = temp;
  ...
```

## Limitations on using constant matrix sizes in compilation

- __Ripple_parallel annotated matrix multiplication__: Constant matrix sizes are
not supported. Use parameterized sizes instead.

- __SPMD-style matrix multiplication__: Small constant matrix sizes are not
supported. Ensure __M__ and __N__ are greater than 32, and __K__ is greater
than 1.

# Some useful clang command-line flags
The SME codegen presented in this section is available for Armv9.2-A.
You can specify this to clang using the following flags:
```bash
-march=armv9.2-a+sme --target=aarch64-linux-gnu
```
We need to also specify the targeted streaming vector length
using the following flag:
```bash
-msve-streaming-vector-bits=512
```
Unrolling some of the loops can obfuscate the pattern matched by the SME
optimization. To avoid this, use
```bash
-fno-unroll-loops
```
... and of course you need to enable Ripple.
```bash
-fenable-ripple
```
In some compiler settings, auto-vectorization can interfere with Ripple.
If that happens, auto-vectorization can be turned off using
```bash
-fno-vectorize
```
At last, SME pattern matching and optimization are enabled at O2 and above.
You will need:
```bash
-O2
```
---
Arm is a registered trademark of Arm Limited (or its subsidiaries).

---
*Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear*
