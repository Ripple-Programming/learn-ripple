# Hexagon-specific errors

## Stack overflow
__Effect__ a runtime segfault or error due to stack allocation
past the stack limit.

Several aspects can contribute to a stack overflow.

__Problem__: Complex vector loop epilogue mask computation
The calculation of complex enough masks can lead to predicate register spills,
which can increase the function's required stack size significantly.
Loops annotated with `ripple_parallel` are split into full-vector loops,
and an epilogue, which basically computes the last vector iteration
(the "epilogue").
When the loop's upper bound is unknown at compile time, the epilogue has to
determine a mask defining which vector lanes should be active.
The computation of the mask can result in extra required stack space,
in the following cases:
- the operations need more predicate registers than available in hardware.
  Spilling a predicate register requires its conversion to a regular vector register,
  which increases the size of the stack required to do the spilling.
- an operation that isn't supported on predicate registers is performed.
  This requires a conversion to regular HVX vectors,
  which increases register pressure, leading to more spilling, i.e., more stack.

__Solution__: `ripple_parallel` vectorizes the epilogue,
which causes these risky mask computations. Two options are available:
- if the loop body is an elementwise computation, simply replace `ripple_parallel`
with `ripple_parallel_peel`.
`ripple_parallel_peel` will execute the epilogue sequentially,
avoiding the mask computation.
If your computation is not elementwise, you can separate the full-tile code,
which you annotate with `ripple_parallel_full` to indicate that there is no epilogue,
and you write your own sequential version of the epilogue.

__Problem__: Heavy inlining increases expected stack size
The compiler is in charge of allocating the values that need to be spilled
on the stack.
The allocation algorithms used by the compiler are not guaranteed to be optimal.
Bigger functions have a higher chance of having suboptimal stack allocation.
As a consequence, functions in which calls to many other functions are inlined
by the compiler are also at risk of suboptimal stack allocation.

__Solution__:
Since Hexagon has a high function call overhead,
there is a need to tradeoff inlining, which reduces overhead due to function calls,
with the risk of stack overflow.