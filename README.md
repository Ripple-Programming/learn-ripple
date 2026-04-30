# Training materials for LLVM-Ripple.

This repository hosts training materials for **Ripple**, an LLVM-based project.

## Documentation

- [Ripple User Manual](https://ripple-programming.github.io/learn-ripple), built form [source]((src/SUMMARY.md).

- [Ripple HVX Optimization Guide](https://ripple-programming.github.io/learn-ripple/opt/hexagon), built from [source](opt/hexagon/src/SUMMARY.md).

- [Ripple Troubleshooting Guide](https://ripple-programming.github.io/learn-ripple/troubleshooting), built from [source](troubleshooting/src/SUMMARY.md).


## Getting started (Hexagon)
- This [docker image](./containers) builds a Ubuntu-based Hexagon development environment for you, including the Hexagon SDK, the Ripple-based toolchain, based on the current Ripple tip, and the ripple test suite. The toolchain includes the Ripple Hexagon cross-compiler (clang) and the simulator (hexagon-sim).
- [Ripple source code](https://github.com/Ripple-Programming/llvm-project) (an LLVM fork).
- [December source release](https://github.com/Ripple-Programming/llvm-project/releases/tag/ripple_2025-12-19)


## HOWTOs

### Build the docs

```bash
$ cargo install mdbook
$ mdbook build -d build
```

### Build the Hexagon Optimization guide

Build:
```bash
$ cd opt/hexagon
$ mdbook build -d build
```

## AI assistance
We provide example prompts that can be used by AI agents to help you code with
Ripple [here](ai/prompts).

## License
Unless explicitly specified otherwise,
the materials under this repository are distributed
under the Clear 3-Clause BSD License.
