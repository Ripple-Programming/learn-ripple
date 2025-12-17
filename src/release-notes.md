# Ripple Release notes (21.0-alpha3) #

- This release adds support for bfloat16 (__bf16 C/C++ type) vectorization through Ripple.

- Ripple only supports a machine with one type of SIMD
processing elements.
This means that in `ripple_set_block_shape(pe_id, shape)`,
the PE id argument is unused, and Ripple always assumes
that it refers to the default, unique SIMD engine.
However, we expect that Ripple can be used jointly with other parallel programming systems.
For instance, OpenMP, where supported, could be used to add thread-level parallelism.

- Prefer using block sizes that correspond to native hardware vector sizes.
 While support for a more general block size exists, native-compatible block
 sizes are easier on the compiler, and they often result in better performance.

- NaN's are propagated in through the floating-point ripple_reducemax and
ripple_reducemin functions, in ways that break their associative semantics.
We are working on a fix.

## Release meant for -O2 ##
Some of the Ripple API won't work in -O0 for this release.
The most tested optimization level is -O2.


## Hexagon-specific functions ##
- Direct access to some specific HVX instructions is not available yet in Ripple,
including the ones based on lookup tables and mini-convolutions.
- hvx_gather & hvx_scatter are only supported for 16-bit and 32-bit data types.
- hvx_rotate is unsupported.
These instructions remain available, even in Ripple functions, through the
use of intrinsics (check out the hvx_to_ripple and ripple_to_hvx functions).


# Bugs and support #
Please provide feedback and report any issues at `ripple.support@qualcomm.com`.

Please provide feedback using the following [form](https://jira-dc.qualcomm.com/jira/plugins/servlet/issueCollectorBootstrap.js?collectorId=4e06f7dd&locale=en_US)

---
*Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear*
