# Training materials for LLVM-Ripple.

This repository hosts training materials for **Ripple**, an LLVM-based project.

## Documentation

- [Ripple User Manual](src/SUMMARY.md), built and hosted here:

  <https://ripple-programming.github.io/learn-ripple/>.
- [Ripple HVX Optimization Guide](opt/hexagon/src/SUMMARY.md),
  built and hosted here:

  <https://ripple-programming.github.io/learn-ripple/opt/hexagon/>.

- [Ripple Troubleshooting Guide](troubleshooting/src/SUMMARY.md),
  built and hosted here:

  <https://ripple-programming.github.io/learn-ripple/troubleshooting/>.


If you are looking for the Ripple source code (an LLVM fork),
go [here](https://github.com/Ripple-Programming/llvm-project).

### HOWTO: Installing a Ripple Hexagon development environment
This is an example building the Hexagon version of LLVM-Ripple.

- Download and unzip the Hexagon SDK. For instance, the Community Edition 6.5.0.0 available [here](https://qpm.qualcomm.com/#/main/tools/details/Hexagon_SDK?version=6.5.0.0)
For this example, let's assume it's unzipped in the `$PWD/Hexagon_SDK/6.5.0.0` folder.
```bash
$ export HEXAGON_SDK_ROOT=$PWD/Hexagon_SDK/6.5.0.0
```
- Get a toolchain, i.e. the set software tools that are usually used with a compiler (linker, etc.).
In this example, we'll use the toolchain available in the SDK. Each SDK contains a toolset (in the `$HEXAGON_SDK_ROOT/tools/HEXAGON_Tools<toolchain-version>/Tools` folder). 
- Copy the toolchain to a separate folder, say `$HEXAGON_TOOLS_ROOT`.
```shell
$ cp -R $HEXAGON_SDK_ROOT/tools/HEXAGON_Tools/<toolchain-version>/Tools .
$ export HEXAGON_TOOLS_ROOT=$PWD/Tools
```
- Override the compiler in the toolchain copy, by installing the Ripple compiler in `$HEXAGON_TOOLS_ROOT`. This is presented in the next section.

### Building the LLVM-Ripple compiler
This is an example of building LLVM-Ripple (i.e. a clang compiler that supports Ripple) and installing it in `$HEXAGON_TOOLS_ROOT`, using cmake. There are two components to build and install: the compiler and the runtime (which contains the Ripple Vector Library).

#### Building the compiler
This example was tested on a Linux Ubuntu 22.04 machine.

Building clang requires a C++ compile, a C++ standard library development package and a linker. In this example we're using clang-17, libstdc++-12 and lld (Ubuntu packages `clang-17`, `libstdc++-12-dev` and `lld`).

Clone the [repository](https://github.com/Ripple-Programming/llvm-project). You can pick a release or the latest commit, depending upon the features and stability you desire. Releases are found [here](https://github.com/Ripple-Programming/llvm-project/tags).

Change the current directory to the main folder of the downloaded repo (`llvm-project`).

Configure the project using cmake:

```shell
llvm-project$ mkdir build
llvm-project$ cmake -G Ninja -S llvm -B build \
            -DCMAKE_BUILD_TYPE=Release \
            -DLLVM_ENABLE_ASSERTIONS=ON \
            -DCMAKE_C_COMPILER=clang-17 \
            -DCMAKE_CXX_COMPILER=clang++-17 \
            -DLLVM_USE_LINKER=lld \
            -DLLVM_ENABLE_PROJECTS="clang" \
            -DLLVM_TARGETS_TO_BUILD=Hexagon \
            -DLLVM_DEFAULT_TARGET_TRIPLE=hexagon-unknown-unknown-elf \
            -DLLVM_PARALLEL_LINK_JOBS=1 \
            -DCMAKE_INSTALL_PREFIX=$HEXAGON_TOOLS_ROOT
```

Build the LLVM-Ripple compiler:
```bash
llvm-project$ cmake --build build
```
Note: we recommend running this build step on a machine with a lot of threads since LLVM is a large codebase.

Install the LLVM-Ripple compiler:

```bash
llvm-project$ cmake --build -t install
```

#### Building the compiler runtime & vector lib
Since some of the Ripple Vector Library is written using Ripple, we need a LLVM-Ripple compiler to build the `compiler-rt` component.
In this example, we use the one we just built.

```bash
llvm-project$ mkdir build-rt
llvm-project$ cmake -G Ninja -S compiler-rt -B build-rt \
  -DCMAKE_BUILD_TYPE=Release \
            -DLLVM_ENABLE_ASSERTIONS=ON \
            -DCMAKE_C_COMPILER=$HEXAGON_TOOLS_ROOT/bin/clang \
            -DCMAKE_CXX_COMPILER=$HEXAGON_TOOLS_ROOT/bin/clang++ \
            -DCOMPILER_RT_BUILD_BUILTINS=OFF \
            -DCOMPILER_RT_BUILD_CRT=OFF \
            -DCOMPILER_RT_BUILD_CTX_PROFILE=OFF \
            -DCOMPILER_RT_BUILD_GWP_ASAN=OFF \
            -DCOMPILER_RT_BUILD_LIBFUZZER=OFF \
            -DCOMPILER_RT_BUILD_MEMPROF=OFF \
            -DCOMPILER_RT_BUILD_ORC=OFF \
            -DCOMPILER_RT_BUILD_PROFILE=OFF \
            -DCOMPILER_RT_BUILD_SANITIZERS=OFF \
            -DCOMPILER_RT_BUILD_XRAY=OFF \
            -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON \
            -DCMAKE_C_COMPILER_TARGET=hexagon \
            -DCMAKE_CXX_COMPILER_TARGET=hexagon \
            -DCMAKE_EXE_LINKER_FLAGS="-G0" \
            -DCMAKE_SYSROOT=$HEXAGON_TOOLS_ROOT/target/hexagon \
            -DLLVM_PARALLEL_LINK_JOBS=1 \
            -DCMAKE_INSTALL_PREFIX=$HEXAGON_TOOLS_ROOT
```
# hexagon-unknown-unknown-elf

### HOWTO: Build the docs

```bash
$ cargo install mdbook
$ mdbook build -d build
```

## Hexagon Optimization guide

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
