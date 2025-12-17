# Abstract

Ripple is an API implemented in the C and C++ programming languages
designed to allow Qualcomm HTP programmers to concisely express
efficient vector kernels across application domains.
It utilizes a long-established and target-independent approach of
expressing an iteration space over rectilinear,
aligned multidimensional arrays (aka tensors)
with implicit reshaping and dimensionality changes through
broadcasts and reductions.
While the basic scalar optimizations of the LLVM compiler,
such as register allocation and instruction scheduling, are available,
the philosophy otherwise is to let the Ripple programmer directly indicate,
through this API, the key decisions on how the kernels should be vectorized.
So, while the core of the API is target-independent,
<!-- for the target hardware HTP, -->
clear rules allow the programmer to reason about the way that their expression
of the algorithm in Ripple results in that vectorization.
Ripple automates secondary decisions and bookkeeping;
the programmer does not need to fuss with those details.

Since Ripple is accessed in the native language form of function calls,
there is both an aesthetic smoothness and the impact and implementation of
Ripple in the tool chain (debuggers, compilers, etc.) is simple.
While automated mapping exists in other compiler systems,
and in research papers,
Ripple's place in the programming tool spectrum is on the side of
enabling the programmer to tell the compiler to "do what I want"
and to do so in terms of the symmetries and mathematic structures
of the application, and minimized details of the hardware.
<!-- While cleanly specificized and implemented, it is also highly pragmatic. -->
In some cases, target-specific concepts are introduced congruent to Ripple
that allow the programmer to directly get at target hardware features
(e.g., memory permutation instructions).
While the goal of Ripple is to allow programmers to rapidly get kernels
that run close to the speed of hand-written code,
in some cases, we see performance greater than hand code due to
the opportunities for optimization that clean expression enables.

Ripple is closest to CUDA (R), OpenCL (R) and OpenMP (R) and
users who are familiar with those languages will find Ripple straightforward.
However, it is not necessary to know any of those languages to program in Ripple.
The core of Ripple is a SPMD expression of iterations similar to CUDA (R)
and OpenMP (R).
This core is extended with for expression parallel application over a tensor,
as loops.
This loop parallelism capability is entirely syntactic sugar
for SPMD expressions.

<!-- The focus of Ripple 1.0 has been for programming the current V79 HTP for the
HVX vector instructions.
Our intent is that kernels expressed in Ripple will be largely
portable and performance-portable to future versions of HTP.
When we are successful with HTP,
future versions of Ripple may target processors beyond HTP -
 such as Arm, Adreno (GPU) and RISC-V,
with associated ML vector and matrix instructions.
This manual, for now, focuses explicitly on Ripple 1.0 and HTP HVX. -->

---
CUDA is a trademark of NVIDIA Corporation.

OpenCL is a trademark of Apple Incorporated.

OpenMP is a registered trademark of the OpenMP Architecture Review Board.

---
*Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear*