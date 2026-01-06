# Hexagon (R) HVX Optimization

This section is structured as a list of coding recommendations
in order to get competitive performance out of your Ripple programs.

# Optimization level

Ripple has been most tested with the `-O2` option on HVX,
which seems to be the most common optimization level.

```bash
clang -O2 ...
```

# Vector size parameters
Current versions of HVX ISAs tend to come with a register file of 32Kb total.
Please check the appropriate programmer's manuals for any specific information,
such as number and bit-width of registers,
about the particular architecture version you are compiling for.
The following block sizes illustrate single-vector block sizes
assuming a vector width of 1024 bits.

  | element type | i8/u8 | i16/u16/f16 | i32/u32/f32 |
  |--|--|--|--|
  | single-vector block size | 128 | 64 | 32 |


Since HVX instruction packets can contain up to four instructions, it is usually not useful to make the size of your block larger than four vectors.
For more information about VLIW slot usage, please consult the Hexagon HVX Programmer's Reference Manual (available on `docs.qualcomm.com`).

# Make the type of constants ("immediates") explicit
__Performance impact__: High.

One thing to remember when coding in Ripple, is that it maintains all semantical aspects of its underlying language.
One aspect of C that can impact performance in C and C++ is their __implicit type conversions__, and the __default type__ for its constants.

__Example 1.__ Consider the following function, which doubles the value of a `float` array.

```c
1:void double_me(float * A, unsigned n) {
2:  ripple_block_t b = ripple_set_block_shape(HVX_LANES, 32);
3:  ripple_parallel(b, 0);
4:  for (int i = 0; i <n; ++i) {
5:    A[i] = 2.0 * A[i];
6:  }
7:}
```

__Problem__: The `2.0` immediate is by default a `double` in C and C++.
Also, type promotion rules in C/C++ indicate that the multiplication line 5 is done after promoting both operands to the same type, `double`.
Hence, Ripple tries to produce a SIMD multiplication of 32 doubles.
However, current versions of HVX do not support SIMD
double-precision instructions.
As a result, the generated code is sequentialized, resulting in an order of magnitude slower performance than if SIMD could have been used.

__Solution__: Make the `2.0` immediate explicitly of the `float` type, using the `f` suffix.
```c
5:     A[i] = 2.0f * A[i];
```

The same issue can happen with integers, for which the default type is `int`, as in the following example:

```c
1:void double_me(short * A, unsigned n) {
2:  ripple_block_t b = ripple_set_block_shape(HVX_LANES, 64);
3:  ripple_parallel(b, 0);
4:  for (int i = 0; i <n; ++i) {
5:    A[i] = 2 * A[i];
6:  }
7:}
```
Here, the `2` line 5 is an `int`, and so is the subsequent multiplication,
because of type promotion rules in the C programming language.
__Solution__: convert it to short explicitly.

```c
5:    A[i] = ((short) 2) * A[i];
```
If you are using C++, precomputing it and forcing it to be a constexpr
will make sure that there is no extra cost to this conversion, as in the following code:
```c++
1:void double_me(short * A, unsigned n) {
2:  ripple_block_t b = ripple_set_block_shape(HVX_LANES, 64);
3:  constexpr short two = (short) 2;
3:  ripple_parallel(b, 0);
4:  for (int i = 0; i <n; ++i) {
5:    A[i] = two * A[i];
6:  }
7:}
```


# Optimizing floating-point code
## Use floating point types up to 32-bit wide (`float`) only
__Performance impact__: High.

Hexagon supports two native floating-point types: float and _Float16.
__bf16 is partially emulated.
There isn't currently a native HVX `double` vector ISA.
Hence, avoid double-precision computations in your vector codes,
which will come out as sequentialized.

## Leverage compiler flags
### Extended precision
__Performance impact__: Medium.

HVX allows extended-precision computations to happen in sequences of floating-point operations happening between memory accesses.
These are in a way more precise than IEEE floating-point, but they will result in a different numerical result from the same computation performed on IEEE floating point numbers.
It is possible to forgo of extended precision and be fully compatible with IEEE. However, this comes at a performance cost.

The use of fast, extended precision vs. slower, IEEE-compliant precision, is controlled by the following flag:
```bash
-mhvx-qfloat=<mode>
```

We refer to the HVX manual to describe the exact behavior of the following four modes:
- `strict-ieee` will give you full compatibility with IEEE floating point.
- `ieee` uses extended precision for some operations.
- `lossy` uses extended precision for more operations.
- `legacy` uses extended precision for almost all operations.

As of clang 21.0, the default is set to `lossy`.
__Warning__: the `lossy` option also implies other flags that impact precision, such as `-ffast-math`, which declares that floating-point (+, *) operations are commutative and associative, and that the data is free of `NaN`s and infinity.

### Working with finite values
__Performance impact__: Low.

clang is able to optimize some codes when you know that your floating-point values are always defined (they are not a NaN) and never infinite.
If you are in that case, use the `-ffinite-only` command-line flag.

### Relaxing the ordering of computations
__Performance impact__: Medium.

Addition and multiplication are commutative and associative for real numbers. This means that you can change the order in which a sequence of (for instance) additions (often called "reductions") are computed without changing the result.
Being able to change this order allows compilers to execute such sequences of operations faster.

Floating point numbers are an approximation for real numbers, and as a consequence, their operations aren't associative strictly speaking.
Changing the order of floating-point computations typically introduces some amount of precision error.

__Solution__ If your computation tolerates some amount of floating-point error, or if semantically that order does not matter for the resulting computation, you can let Ripple know it, using the following command-line flag:

```bash
clang -fassociative-math
```
This will speed up some computations, including reductions (expressed through the `ripple_reduce_*()` ripple API).

### Fast conversions from floating-point to integer
__Performance impact__: Low.

Conversion from floating-point (`Float` and `float` types) to integer
(`int16_t` and `int32_t`) can be slow,
as clang tries to implement it as precisely as possible.

__Solution__ Enable fast floating-point conversion,
which is very close semantically, but uses a single instruction.
To enable it, use the following command-line flag:

```bash
clang -mllvm -hexagon-fp-fast-convert=true ...
```

# Using ripple_shuffle on vector pairs
__Performance impact__: Medium.

We empirically noticed that shuffles on blocks of 2 vectors tend to
generate more efficient machine code.

# Inlining functions
__Performance impact__: High.

For an HVX target, there are significant overheads associated with function calls.
The overheads mostly arise from saving the register frame onto the stack
before the function call and loading the register frame from the stack
after the call returns.
Consequently, we advise the programmer to utilize clang's
["always_inline"](https://clang.llvm.org/docs/AttributeReference.html#always-inline-force-inline)
function attribute.
Avoid these overheads while maintaining code-readability as follows:

```C
 static int foo(int a, int b) __attribute__((always_inline)) {
    /// ...
  }
```

or in C++

```C++
static int foo(int a, int b) [[gnu::always_inline]] {
    /// ...
  }
```

# Functions optimized for HVX

## High-throughput data reordering (scatter/gather)
__Performance impact__: High.

Loading from a collection of arbitrary addresses into a vector
is by construction a long-latency operation,
as it can result in bank conflicts.
Hence, we do not recommend the direct use of non-coalesced access functions.

However, the Ripple vector library includes a
high-bandwidth memory-to-memory copying API for HVX,
which can move large amounts of data in parallel
(still with the same type of latency).

In cases where we cannot change our computations
to create coalesced data accesses, we can still rearrange our data into a set
that can be accessed in a coalesced way a bit later,
using the `hvx_gather/hvx_scatter` API.

Use cases for scatter-gather include indirections (`A[B[i]]`),
large lookup tables, sparse-dense array computations (for instance sparse matrix dense vector multiplication), and strided data accesses, particularly when the data access stride is large (e.g. when accessing element of a large array along columns).
We refer to the Ripple manual for a more complete description of
`hvx_scatter` and `hvx_gather`.

## Explicit bfloat16 conversions
__Performance impact__: High.

Ripple supports three types of conversions from `float` to `__bf16`:
- `to_bf_trunc`, a direct truncation
- `to_bf_nan`, a truncation that avoids incorrect NaN conversions (they can become an infinity using the direct shift)
- `to_bf_round`, a conversion that performs an even rounding and avoids incorrect Nan conversions. The behavior of this conversion is compatible with x86 bfloat conversion.

## Dynamic rotations
__Performance impact__: Low.

`hvx_rotate(x, n)` will consider x flat (irrespective of its shape) and rotate the elements towards n lower indices
(e.g. the element at position 3+n goes to position 3).
One valuable aspect of this function is that `n` doesn't need to be
a compile-time constant.
`n` could be a loop counter, for instance.

## Narrowing shifts
__Performance impact__: Medium.

### Syntax
```C
narrow_t hvx_narsh[[_rnd]_sat][_noshuff](wide_t odd, wide_t even, uint32 shift);
```

A narrowing shift (or "narsh") is an operation on integers,
which performs a right shift followed by a conversion
to a smaller integer type.
The conversion can be done by truncation or using saturation (using the function name suffix `_sat`).
When saturation is used, we can also request rounding up, using the
`_sat_rnd` suffix.
The shift amount sh is the same for all elements of the block.

Two forms are supported. The first, faster form takes two inputs, performs a narrowing shift, and shuffles the resulting block.
The second form, suffixed with `_noshuff`, doesn't shuffle the result.

### Supported source and destination types __

  | wide (source) type | narrow (destination) type|
  |--------------------|--------------------------|
  | i32 (int32_t)      | i16, u16                 |
  | u32 (uint32_t)     | u16                      |
  | i16 (int16_t)      | i8, u8                   |
  | u16                | u8                       |


An interesting aspect of this function is that it packs
the narrowed by pairs, i.e. two 32-bit integers get narrowed and packed into one 32-bit integer.

### Examples
```C
int16_t hvx_narsh_sat_i16toi8(int16_t odd, int16_t even, uint32_t shift);
uint32_t hvx_narsh_rnd_sat_u32tou16_noshuff(uint32_t odd, uint32_t even, uint32_t shift);
```

### Constraints
hvx_narsh works on block sizes that occupy a full HVX vector.

## Chunked zipping and unzipping for pairs of HVX vectors
__Performance impact__: Medium.
### Syntax
```C
T hvx_pair_chunked_zip(T x, size_t chunk_size);
T hvx_pair_chunked_unzip(T x, size_t chunk_size);
T hvx_pair_chunked_zip_list(T x, size_t chunk_size, size_t list_size);
T hvx_pair_chunked_unzip_list(T x, size_t chunk_size, size_t list_size);
```

`hvx_pair_chunked_zip` and `hvx_pair_chunked_unzip`
are useful data-dependent data reindexing functions
for block sizes that correspond to 256 bytes.
For example, if the chunk size parameter `chunk_size` is used in a loop.
Data reorganization following a constant patterns can always be achieved using
`ripple_shuffle()` or `ripple_shuffle_pair()`.

The `zip` and `unzip` terms come from python.
- `hvx_pair_chunked_zip` considers the input `x` to be a pair
  of sequences of chunks of size `chunk_size.
  It returns a corresponding ("zipped") sequence of pairs (of chunks).

  Explained from a tensor perspective,
  `x` can be seen as a `chunk_size x n x 2` tensor,
  whose last two dimensions gets transposed, into a `chunk_size x 2 x n` tensor.

- `hvx_pair_chunked_unzip` is the opposite operation:
  the input block is considered as a sequence of pairs of chunks,
  and the output block is the corresponding pair of sequences of chunks.

  Explained from a tensor perspective,
  `x` can be seen as a `chunk_size x 2 x n` tensor,
  whose last two dimensions gets transposed, into a `chunk_size x n x 2` tensor.

- `hvx_pair_chunked_zip_list` considers `x` to be a pair of _lists_ of sequences
  of chunks.
  The sequence size `S` inside each list is defined by `chunk_size` and `list_size` as
  `128 / sizeof(T) / chunk_size / list_size`.
  For each list index, it zips both indexed input list elements
  into a list of sequences of `S` zipped pairs of `chunk_size`-sized chunks.

  Seen as a tensor, the input shape would be `chunk_size x S x list_size x 2`,
  while the output shape would be `chunk_size x 2 x S x n_lists`.

- `hvx_pair_chunked_unzip_list` performs the opposite operation.
  It considers `x` to be a _list_ of sequences of pairs of chunks.
  The input is a list of sequences of pairs of chunks, and
  the output is a pair of lists of sequences of chunks.
  The sequence size `S` inside each list is defined by `chunk_size` and
  `list_size` as `128 / sizeof(T) / chunk_size / list_size`.
  For each list index, `hvx_pair_chunked_unzip_list` unzips pairs of chunks in
  the indexed input sequence into two sequences that are indexed in each element
  of the output pair of lists.

  Seen as a tensor, the input shape would be `chunk_size x 2 x S x list_size`,
  while the output shape would be `chunk_size x S x n_lists x 2`.

### Constraints
- `x`'s shape must fit exactly a pair of HVX vectors.
- `chunk_size` has to be a power of 2 and can be up to `32 / sizeof(T)`.
- `list_size` must also be a power of two.

## 2x2 subtensor transposition for pairs of HVX vectors
__Performance impact__: Medium.

### Syntax
```C
T hvx_pair_2x2_transpose(T x, size_t chunk_size);
T hvx_pair_2x2 transpose_inc(T x, size_t chunk_sizes);
T hvx_pair_2x2_transpose_dec(T x, size_t chunk_sizes);
```

- `hvx_pair_2x2_transpose` considers `x` as a `chunk_size x 2 x n x 2` tensor `A`,
  where `chunk_size` is a power of two.
 The returned block is obtained by transposing the second and fourth dimensions,
 i.e., transposing `2x2` sub-tensors of element size `chunk_size`.
 `n = 64 / chunk_size`.

- `hvx_pair_2x2_transpose_inc` and `hvx_pair_2x2_transpose_dec`
  accept a non-power of two `chunk_sizes` parameter.
  However, it does not interpret `chunk_sizes` as a single chunk size,
  but instead as a sequence of chunk sizes for which the subtensor transposition
  defined by `hvx_pair_2x2_transpose` must be applied.
  Each bit set in `chunk_sizes` defines a power-of-two chunk_size to be applied
  in that sequence.
  - `hvx_pair_2x2_transpose_inc` applies the transpositions hierarchically
    by increasing order of chunk sizes, while `hvx_pair_2x2_transpose_dec`
    applies the transpositions hiearchically by decreasing order of chunk sizes.

### Constraints
- `x`'s shape must fit exactly a pair of HVX vectors.
- In `hvx_pair_2x2_transpose()`, `chunk_size` must be a power of two,
  and is limited to `32 / sizeof(T)`.

## Vector splicing
__Performance impact__: High.

### Syntax
```C
T hvx_splice(T high, T low, size_t n);
T hvx_lsplice(T high, T low, size_t n);
```

This is a _flat_ operator, which means that it considers a block as being
one-dimensional.
Hence, we describe blocks here as "vector"s.
`hvx_splice` forms a vector from two vectors `high` and `low`.
Let's call `N` the number of elements in the vector.
The returned vector contains
the `n` lowest elements of `high` as its highest elements,
and the `N-n` highest elements of `low` as its lowest elements.

`hvx_lsplice` ("low splice") forms a vector from two vectors `high` and `low`.
The returned vector contains
the `n` highest elements of `low` as its lowest elements,
and the `N-n` lowest elements of `high` as its highest elements.

This behavior is illustrated on Figure H1.
![hvx_splice and hvx_lsplice function behavior](./hvx_splice.png "hvx_splice behavior")
__Figure H1.__ hvx_splice/hvx_lsplice behavior

__Note__: An implicit "modulo N" is applied to `n`, to ensure sound semantics.

### Examples
Typical uses for `hvx_splice` are codes that use overlapping windows of tensors,
for example in stencils, convolutions and pooling.
Example 2 will be a sliding window, a 1-dimensional Gaussian blur.

`hvx_splice` is also sometimes used to implement padding, as shown in Example 1.

__Example 1__: We need to add two arrays, but only one of them has a size
that is a multiple of the HVX vector size (here called `HVX_SIZE_U16`).

```C++
#define HVX_SIZE_U16 64
void padded_add(size_t n, uint16_t * in1_padded, uint16_t * in2_unpadded,
                uint16_t * out_padded) {
  ripple_block_t B = ripple_set_shape(HVX_LANES, HVX_SIZE_U16);
  size_t v = ripple_id(B, 0);
  size_t i;
  // full-vector loop: we know we have full vectors from in1 and in2
  for (i = 0; i < n; i += HVX_SIZE_U16) {
    out_padded[i + v] = in1_padded[i + v] + in2_unpadded[i + v];
  }
  // epilogue: There aren't enough elements in in2_unpadded to add with.
  //           We're splicing that partial vector with a vector of zeros
  out_padded[i + v] = in1_padded[i + v] +
    // we want to specify the #elements taken from the lower vector --> lsplice
                      hvx_lsplice(0, in2_unpadded[i + v], n - i);
}
```

Note that the same effect can be obtained using conditionals in this case.
The epilogue code in `padded_add` below could be replaced with the following
code to obtain the same result.
```C++
  out_padded[i + v] = in1_padded[i + v] + (i + v < n) ? in2_unpadded[i + v] : 0;
```
The difference between the two lies in the use of predicate registers
in the latter.
The sequence of instructions for the original padded_add epilogue are roughly:
- Compute `n - i` (a scalar).
- Top off `in2_unpadded` with zeros using `hvx_lsplice`.
- Add `in1_padded` with the combined vector

The sequence of instruction for the conditional-based version is roughly:
- Compute the vector predicate for the expression `i + v < n`.
- Select the elements from the zero-vector and `in2_unpadded`
  as specified in the vector predicate.
- Add `in1_padded` with the resulting vector

While the number of steps seems comparable,
computing predicates can be more expensive than computing non-predicates.
Hence in this case, the version based on `hvx_lsplice` is generally preferred.

__Example 2__: Stencil operations are often used to write filters,
or iteratively solve partial differential equations.

The following code is a simple gaussian blur operating on a 1-d signal:

```C++
void gaussian_blur(size_t n, float * in, float * out) {
  constexpr float one_third = 1.f/3.f;
  for (size_t i = 1; i < n - 1; ++i) {
    out[i] = (in[i - 1] + in[i] + in[i + 1]) * one_third;
  }
}
```
A naïve vectorization can be obtained by decorating the `i` loop with
`ripple_parallel`.
No matter how `in` and `out` are align, at least two of the loads of `in`
will be unaligned.
We can do better by using only aligned loads and stores.

```C++
void gaussian_blur(size_t n, float * in, float * out) {
  constexpr float one_third = 1.f/3.f;
  constexpr size_t nv = 32;
  constexpr size_t hvx_bytes = nv * sizeof(float);
  ripple_block_t B = ripple_set_shape(HVX_LANES, nv);
  size_t v = ripple_id(B, 0);
  float * in_ptr_lower, * in_ptr, * in_ptr_upper;

  // prologue
  in_ptr = ripple_ptr_alignment(&in[0], hvx_bytes);
  in_ptr_upper = ripple_ptr_alignment(&in[nv], hvx_bytes);
  float * out_ptr = ripple_ptr_alignment(&out[i], hvx_bytes);
  out_ptr[v] = (hvx_lsplice(0, in_ptr, 1) +
               in_ptr[v] +
               hvx_lsplice(in_ptr, in_ptr_upper, 1)) * one_third;

  // steady-state loop
  size_t i;
  for (i = nv; i < n - nv; i += nv) {
    in_ptr = ripple_ptr_alignment(&in[i], hvx_bytes);
    in_ptr_lower = ripple_ptr_alignment(&in[i - nv], hvx_bytes);
    in_ptr_upper = ripple_ptr_alignment(&in[i + nv], hvx_bytes);
    float * out_ptr = ripple_ptr_alignment(&out[i], hvx_bytes);

    out_ptr[v] = // take one extra element from the lower vector
                 (hvx_lsplice(in_ptr_lower, in_ptr, 1) +
                  in[v] +
                  // take one extra element from the upper vector
                  hvx_splice(in_ptr, in_ptr_upper, 1)) * one_third;
  }

  // epilogue
  if (i + v < n) {
    in_ptr = ripple_ptr_alignment(&in[i], hvx_bytes);
    in_ptr_lower = ripple_ptr_alignment(&in[i - nv], hvx_bytes);
    float * out_ptr = ripple_ptr_alignment(&out[i], hvx_bytes);

    out_ptr[v] = (hvx_lsplice(in_ptr_lower, in_ptr, 1) +
                  in_ptr[v] +
                  hvx_lsplice(in_ptr, 0, 1)) * one_third;
  }
}
```

The code above is faster than the original code,
because it only involves aligned loads and stores.

We can make it even faster by noticing that consecutive iterations of `i`
reuse two of the three input vectors.
Hence, instead of re-loading these two vectors,
we can just reuse them from the previous iteration.
This reuse technique, often called "register rotation,"
is applied to `gaussian_blur` below.

```C++
void gaussian_blur(size_t n, float * in, float * out) {
  constexpr float one_third = 1.f/3.f;
  constexpr size_t nv = 32;
  constexpr size_t hvx_bytes = nv * sizeof(float);
  ripple_block_t B = ripple_set_shape(HVX_LANES, nv);
  size_t v = ripple_id(B, 0);
  float * in_ptr_lower, * in_ptr, * in_ptr_upper;

  // prologue
  in_ptr = ripple_ptr_alignment(&in[0], hvx_bytes);
  in_ptr_upper = ripple_ptr_alignment(&in[nv], hvx_bytes);
  float * out_ptr = ripple_ptr_alignment(&out[i], hvx_bytes);
  out_ptr[v] = (hvx_lsplice(0, in_ptr, 1) +
                in_ptr[v] +
                hvx_lsplice(in_ptr, in_ptr_upper, 1)) * one_third;

  // steady-state loop
  size_t i;
  for (i = nv; i < n - nv; i += nv) {
    // register rotation: in_ptr -> in_ptr_lower; in_ptr_upper -> in_ptr
    in_ptr_lower = ripple_ptr_alignment(in_ptr, hvx_bytes);
    in_ptr = ripple_ptr_alignment(in_ptr_upper, hvx_bytes);
    // in_ptr_upper is new, still need to load it
    in_ptr_upper = ripple_ptr_alignment(&in[i + nv], hvx_bytes);
    float * out_ptr = ripple_ptr_alignment(&out[i], hvx_bytes);

    out_ptr[v] = // take one extra element from the lower vector
                 (hvx_lsplice(in_ptr_lower, in_ptr, 1) +
                  in[v] +
                  // take one extra element from the upper vector
                  hvx_splice(in_ptr, in_ptr_upper, 1)) * one_third;
  }

  // epilogue
  if (i + v < n) {
    // register rotation applied here as well
    in_ptr_lower = ripple_ptr_alignment(in_ptr, hvx_bytes);
    in_ptr = ripple_ptr_alignment(in_ptr_upper, hvx_bytes);
    float * out_ptr = ripple_ptr_alignment(&out[i], hvx_bytes);

    out_ptr[v] = (hvx_lsplice(in_ptr_lower, in_ptr, 1) +
                  in_ptr[v] +
                  hvx_splice(in_ptr, 0, 1)) * one_third;
  }
}
```

Let `V` the number of HVX vectors in `in`.
With this transformation, the total number of vector loads from `in` went from
about `3*V` to `V`, without adding any work, a clear performance win.
Note that the versions of `gaussian_blur` above that use `hvx_splice` assume
that it's safe to read up to one vector past the upper boundary of `in`.
It is possible to remove that limitation by writing slightly more complex code.

### Constraints
`high` and `low` shapes must correspond to
the number of elements in one HVX Vector.

---
*Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear*
