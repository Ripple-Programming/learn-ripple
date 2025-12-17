# Ripple vs. CUDA(R) and OpenCL(R)

Like CUDA(R) and OpenCL(R), Ripple implements an SPMD parallel programming abstraction
by defining block sizes and indices.
These concepts have a match in Ripple, although in CUDA and OpenCL their syntax
makes them more specialized to CUDA's "threads" and OpenCL's "local" PEs.
On an NVIDIA GPU, these correspond to the "core" and "SM" hardware
processing elements (an SM, or "Streaming Multiprocessor", contains several cores).

To perform a comparison, in Ripple we must look at a target machine
whose structure is that of a GPU,
in that the target machine has two kinds of PEs.
To push the comparison, let's call them VECTOR
(which, in the comparison, corresponds to the CUDA cores level)
and MULTICORE (which corresponds the Streaming Multiprocessors).


  | CUDA(R)        | OpenCL(R)           | Ripple             |
  | :--: | :--: | :--: |
  | threadIdx.x | get_local_id(0)  | ripple_id(vector_block, 0) |
  | threadIdx.y | get_local_id(1)  | ripple_id(vector_block, 1) |
  | blockIdx.x  | get_group_id(0)  | ripple_id(multicore_block, 0)   |
  | blockIdx.y  | get_group_id(1)  | ripple_id(multicore_block, 1)   |
  | blockDim.x  | get_local_size(0)| ripple_get_block_size(vector_block, 0) |
  | __syncthreads() | barrier(CLK_LOCAL_MEM_FENCE) | ripple_sync(vector_block) |


The table above shows that the Ripple API is able to target
a wider range of target machines,
since it does not intrinsically make an assumption about the
targeted architecture.
Although only a machine with SIMD is supported by the compiler for now,
in theory the API allows us to write code targeting
many different types of machine in Ripple.

The idea is that the hierarchy of processing elements,
as well as how they interact (synchronization, shared memories),
will be described in a machine model, driving the compiler's rendering of the
Ripple code to the targeted machine.

## SIMD vs SIMT
On SIMD engines, hardware vector lanes execute in a tightly-coupled fashion,
hence they are already as synchronized with each other as they can get.
As a result, SIMD vector and matrix PEs targeted by Ripple
don't need any calls to `ripple_sync()`.

## Multi-dimensional blocks
One of Ripple's objectives is to enable the expression of mixed scalar, vector
and tensor codes in the same function.

In CUDA(R) and OpenCL(R), where a two-dimensional block is used,
all computations in the function are executed by
a two-dimensional block of PEs.
As a result, the GPU cores may perform a lot of redundant computations,
which could advantageously be done by a single scalar engine
on hardware architectures that include one.

Non-GPUs often contain a variety of scalar, vector and matrix engines.
While distributing computations on the elements of a matrix engine
would typically require a two-dimensional block,
vector computations in that same function
may be best represented by a one-dimensional block.
There is also no need to have scalar computations be duplicated across
a vector or matrix block.
Ripple gives us control over the number of
dimensions in which computations should be performed.

As a result, Ripple programs written for an architecture that includes
scalar, vector and matrix engines are made of computations whose shapes
vary from 0 to at least 2.

## Shape consistency
CUDA(R) and OpenCL(R) assume that the shape of all computations is the
shape they have defined for the entire function.
Shape consistency is guaranteed by the simple fact that
all operations have the same shape.

Ripple enables the expression of shapes of different dimensions in a function,
so the various shapes must be consistent with each other.
Because implicit broadcasting removes
a lot of potential shape inconsistency issues,
the shape inconsistency problem tends to crystalize around writes.

While the following code is legal (although an undefined behavior) in CUDA:
```C
  A[0][0] = B[threadIdx.y][threadIdx.x];
```
the corresponding Ripple code is illegal,
because it results in shape inconsistency:

```C
  A[0][0] = B[ripple_id(vector_block, 0)]
```
The shape of the write to `A` is zero-dimensional,
while the read from `B` is one-dimensional.

## Shuffling
The shuffle API offered by Ripple applies to the full block
they take as an input. Hence it is more general than
the ones proposed by CUDA(R) and OpenCL(R), which only apply to subgroups.

## Performance portability
Ripple offers the same promise of performance portability as CUDA(R) and OpenCL(R):
a fairly weak one.
While the same Ripple code can be recompiled
to and run a variety of SIMD architectures as is,
the resulting performance compared to peak will
vary greatly across architectures.

As with CUDA and OpenCL, Ripple users can easily tune their implementation
by adapting the block size.
However, CUDA, OpenCL and Ripple programs that are optimized for a given
hardware architecture will generally not be optimal
for a different architecture.

However, Ripple offers a single API for all the architectures it supports,
making it _possible_ to write performance-portable programs.
Such programs will be written explicitly as a function of architectural features
of the targeted machine (e.g. SIMD width, cache sizes)
to achieve performance portability.

## Porting CUDA(R) and OpenCL(R) to Ripple
Developers desiring to port existing CUDA(R) and OpenCL(R) programs will
have to take the above differences into account when doing so.
Depending upon the input, starting from a sequential implementation
and using [Ripple loop annotations](./loop-annotation.md) might be
more effective than trying to port code from their CUDA or OpenCL version.

---

CUDA is a trademark of NVIDIA Corporation.

OpenCL is a trademark of Apple Incorporated.

---
*Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear*
