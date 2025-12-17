# Hexagon(R) HVX Optimization

In this section, we provide some tips to help optimize Ripple programs when
targeting the Hexagon(R) HVX ISA.

# Inline functions
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

# When using shuffles, favor permutations
Shuffles in Ripple take a vector and a _source function_,
which determines which element of the input vector becomes the `i`-th element
of the output vector.
When the block size corresponds to one hardware vector, and the source function
is a permutation, the shuffle is guaranteed to be
performed in two HVX instructions (per hardware HVX vector).
If not a permutation, it can be one, but also more than two.

So, if the source function doesn't need to be defined
for all of our vector elements,
and if it can be completed to become a permutation,
it then provides the 2-instruction guarantee (which is good!).

# High-throughput data reordering
Loading from a collection of arbitrary addresses into a vector is
by construction a long-latency operation, as it can result in bank conflicts.
Hence, we do not recommend the direct use of non-coalesced access functions.

However, HVX offers a memory-to-memory copying mechanism,
which can move large amounts of data in parallel
(still with the same type of latency).

In cases where we cannot change our computations
to create coalesced data accesses, we can still rearrange our data into a set
that can be accessed in a coalesced way a bit later,
using the `hvx_gather/hvx_scatter` API.


# Interacting with HVX vectors
We already know the `ripple_to_vec` and `vec_to_ripple`
[API](../ripple-spec/api.md) to convert between Ripple blocks and C vectors.
The `ripple_hvx.h` header exposes utility functions that make it easier to
write functions in which Ripple code interacts
with plain C HVX vector code.
It defines types for a single, a pair, and a quad of HVX vectors for all standard element types.

## Vector type definitions

Type naming follows the LLVM convention: `v<num_els><t><pred>`, where:
- `num_els` is the number of elements in the vector
- `t` represents the element type (`u` for unsigned integer,
`i` for signed integer, `f` for floating-point)
- `prec` represents the precision associated with the type.

For instance:
- `v32f32` is a 32-element vector of `float`s (which fits on one HVX vector).
- `v256u8` is a 256-element vector of `uint8_t`s (a pair of HVX vectors)
- `v256f16` is a 256-element vector of `_Float16`s (a quad of HVX vectors)

## Dealing with the HVX_Vector type when mixing HVX vectors and Ripple
HVX intrinsics work with a single vector type, `HVX_Vector`, which is basically `v32i32`.
Intrinsics working on single vectors are all defined
using HVX_Vector and `HVX_VectorPair`,
which can create type mismatches with type-richer Ripple code.

We introduced explicit "casting" functions, which don't actually change the actual bits in our vectors, but just modify their type:

```C++
vXXi32 hvx_cast_to_i32(<vec_type> x);
```

```C++
vXXy hvx_cast_from_i32(<v32i32 x, <scalar_type> y>);
```
Where XX is the amount of elements used to represent the same bits as `x` in the source (`y` for `hvx_cast_from_i32`) or destination (element type of `x` for `hvx_cast_to_i32`) vector or vector pair.

__Implementation Note__: in C++, these conversion function are available for both `HVX_Vector` and `HVX_VectorPair`. In C, they are only available for `HVX_Vector`.

Using these conversion functions, we can take the norm example and write it for float elements as follows.

```C++
#include <ripple.h>
#include <ripple_hvx.h>

HVX_Vector norm_f32(HVX_VectorPair pair) {
  ripple_block_t BS = ripple_set_block_shape(0, 32, 2);
  float float_type;
  v64f32 pair_as_floats = hvx_cast_from_i32(pair, float_type);
  float x = hvx_to_ripple_2d(BS, 64, f32, pair_as_floats);
  // This "zip" shuffle turned a sequence of pairs into a pair of sequences
  float even_odds = ripple_shuffle(x, rzip::shuffle_unzip<2, 0, 0>);
  float evens = ripple_slice(even_odds, -1, 0); // 32x1 shape
  float odds = ripple_slice(even_odds, -1, 1); // 32x1 shape
  // go back to explicit C vectors of i32's (HVX_Vector)
  v32f32 result = ripple_to_hvx(BS, 32, f32, evens * odds);
  return hvx_cast_to_i32(result);
}
```

To write the same function for `_Float16`, we would have to consider that the number of elements in a `HVX_VectorPair` is actually 128, and use the appropriate `(64x2)` block size, as follows:

```C++
#include <ripple.h>
#include <ripple_hvx.h>

HVX_Vector norm_f16(HVX_VectorPair pair) {
  auto BS = ripple_set_block_shape(0, 64, 2); // 128 elements to process
  _Float16 f16_type;
  v128f16 pair_as_floats = hvx_cast_from_i32(pair, f16_type);
  _Float16 x = hvx_to_ripple_2d(BS, 128, f16, pair_as_floats);
  // This "zip" shuffle turned a sequence of pairs into a pair of sequences
  _Float16 even_odds = ripple_shuffle(x, rzip::shuffle_unzip<2, 0, 0>);
  _Float16 evens = ripple_slice(even_odds, -1, 0); // 64x1 shape
  _Float16 odds = ripple_slice(even_odds, -1, 1); // 64x1 shape
  v64f16 result = ripple_to_hvx(BS, 64, f16, evens * odds);
  // go back to explicit C vectors of i32's (HVX_Vector)
  return hvx_cast_to_i32(result);
}
```
# Fast Conversion from `_Float16` to `int16_t` 

The -`hexagon-fp-fast-convert` flag enables optimized conversion from `_Float16` to `int16_t`, and from `float` to `int32_t`.
on Hexagon devices version 73 and above.

**Usage**:
To enable this optimization, add the following to your clang or clang++
```Bash
-mllvm -hexagon-fp-fast-convert=true
```

Effect on Assembly:
With this flag enabled, the following C++ code:
```C++
_Float16 xhf;
int16_t xh = static_cast<int16_t>(xhf);
```
Will be compiled into a single assembly instruction:
```
v8.h = v13.hf
```
This significantly improves conversion performance by reducing its code generation to a single instruction.

---
Hexagon is a registered trademark of Qualcomm Incorporated.

---
*Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear*
