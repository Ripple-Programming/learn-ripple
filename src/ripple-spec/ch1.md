# Ripple

Ripple is a compiler-interpreted API to express
SPMD (Single Program, Multiple Data, as with CUDA(R) and OpenCL(R)) and
loop-annotation parallelism (as with OpenMP(R)).

__Implementation note__: The current Ripple implementation
only supports SIMD code generation.
<!-- While it is possible to generate code for other SIMD machines,
we have been focusing on Hexagon (HVX) code generation. -->

Ripple does not modify the underlying language syntax, but complements it
with parallel semantics.
Ripple has a C and C++ implementation through Clang/LLVM.

In the next sections, we provide a quick introduction
to both parallel programming models that are enabled by Ripple:
SPMD and loop annotations.
These are discussed in greater detail in the
[SPMD](./spmd.md) and [loop annotation](./loop-annotation.md) chapters.

# How to represent a set of parallel processing elements
Parallel processing elements (PEs) are organized as a "block",
i.e. an array of processing elements.
Each PE is an element of the array,
which can be accessed using as many integer indices
as there are dimensions in the array.
In this section, we will only consider one-dimensional blocks
to illustrate the parallel programming models enabled by Ripple.
Please refer to [the section on multi-dimensional blocks](./spmd.md#multi-dimensional-blocks) for additional details.

To express the amount of parallel processing elements
that we want to use,
we have to declare a block of these PEs, using
```C
ripple_block_t ripple_set_block_shape(int pe_id, size_t ... shape);
```
These PEs are a software view of the hardware PEs
that will actually run the code.
Ripple defines an implicit mapping from PEs in a block
to hardware processing elements.

__Implementation note__: The first parameter, `pe_id`,
has currently no effect,
since Ripple supports only a target with one SIMD engine.
In future Ripple versions, `pe_id` will correspond to representations of
parallel processing elements in a machine model.
For now, using `0` is fine.

For instance, the following function call tells Ripple that parallel code will
run on a one-dimensional block of 42 parallel processing elements.
The type of processing elements is "lanes of the default SIMD engine"
(the only one supported yet), represented by `pe_id=0`.
```C
ripple_block_t BS = ripple_set_block_shape(0, 42);
```

A better idea is to define a pre-processor variable to 0 now,
and modify that definition as the `pe_id` semantics evolve, as in:
```C
#define VECTOR_PE 0
...
  ripple_block_t BS = ripple_set_block_shape(VECTOR_PE, 32);
  size_t v = ripple_id(BS, 0);
```
We use either system interchangeably in this document.

In the future, Ripple may target a hierarchical parallel machine, and `pe_id`
will index that hierarchy.

# How to write a parallel program
Now that we have declared our targeted block of processing elements,
there are two ways Ripple allows us to express parallelism:
the *SPMD model* and the *loop annotation model*.

## SPMD model
In the SPMD model,
the function executes a mix of scalar and parallel computations.
Each parallel processing element (PE) is identified by its index in the block,
which we can represent using `ripple_id()`:
```C
size_t ripple_id(ripple_block_t block_shape, int dim);
```

Everything in the function that depends upon the index in a PE block is executed
by an element of the block.
In other words, it's executed in parallel.
Everything that does not depend upon a PE block index is scalar.

SIMD vector engines execute the function in a tightly-coupled way,
meaning that they synchronize at each instruction.

The following program defines its target to be a block of 42
processing elements.
Each element of the block loads one element of `a` and `b`,
sums them and stores them into `c`.

```c
void array_vadd(float a[42], float b[42], float sum[42]) {
  // Defines a one-dimensional block of size of 42
  ripple_block_t BS = ripple_set_block_shape(VECTOR_PE, 42);

  // Retrieves a block of indices:
  //    [0 ... 41]
  size_t ripple_index = ripple_id(BS, /* Tensor index */ 0);

  // Block load/store/addition by indexing arrays using the ripple index
  sum[ripple_index] = a[ripple_index] + b[ripple_index];
}
```
## Loop annotation model
In the loop annotation model, we tell Ripple to distribute all the
iterations of a loop onto the elements of the block.
This is done by calling `ripple_parallel(ripple_block_t block_shape, int dimension)`
right before the loop that needs to be distributed.
Using this principle, the `array_vadd` function above can be
written as follows.

```c
void array_vadd(float a[42], float b[42], float sum[42]) {
  // Defines a one-dimensional block of size of 42
  ripple_block_t BS = ripple_set_block_shape(VECTOR_PE, 42);

  // Distributes values of "i" (and anything that depends upon "i")
  // across dimension 0 of the block
  ripple_parallel(BS, 0);
  for (size_t i = 0; i < 42; ++i) {
    sum[i] = a[i] + b[i];
  }
}
```
Notice that the implementation of `array_vadd`
based on loop annotations is much closer to a sequential implementation,
which usually makes the code more readable.
However, loop annotations are only applicable to loop parallelism,
and subject to syntactic constraints.
Please consult the chapter on [loop annotations](./loop-annotation.md) for more detail.

# Ripple SIMD programming API
While both the SPMD and loop annotation APIs allows us to
write element-wise programs,
SIMD programs often involve operations across hardware vector elements,
such as reductions (applying a commutative operation across PEs) and
shuffles (reordering the data associated with PEs).
Ripple defines an API to perform these common operations,
detailed as part of the [Ripple API](./api.md) section.

# Command-line
The SPMD and loop annotation models are enabled by the use of the following
clang command-line argument:

```bash
-fenable-ripple
```
---
CUDA is a trademark of NVIDIA Corporation.

OpenCL is a trademark of Apple Incorporated.

---
*Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear*
