# API not implemented yet

```C
/// \brief barrier among elements of the block for processing element \p pe_id
///        Note that it is not necessary for SIMD vector parallelism.
/// Note: NIY as it is not necessary for SIMD targets
void ripple_sync(int pe);

/// \brief defines that code aliasing with addr lives in memory mem_id
///        memory IDs are defined in the machine model and ripple.h
/// Note: NIY
void ripple_set_mem(int mem_id, void * addr);

/// \brief declares that all memory accesses (loads, stores)
///        dominated by this API call and aliasing with \p addr are aligned.
/// Note: NIY
void ripple_aligned_start(void * addr);

/// \brief declares that all memory accesses (loads, stores)
///        dominated by this API call
///        and aliasing with \p addr stop being aligned.
/// Note: NIY
void ripple_aligned_end(void * addr);
```


---
*Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear*
