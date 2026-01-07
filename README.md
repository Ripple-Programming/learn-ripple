## Documentation source for Ripple.

This repository hosts the documentation for **Ripple**, and LLVM-based project.

- [Ripple User Manual](src/SUMMARY.md), built and hosted here:

  <https://ripple-programming.github.io/learn-ripple/>.
- [Ripple HVX Optimization Guide](opt/hexagon/src/SUMMARY.md),
  built and hosted here:

  <https://ripple-programming.github.io/learn-ripple/opt/hexagon/>.


If you are looking for the Ripple source code (an LLVM fork),
go [here](https://github.com/Ripple-Programming/llvm-project).

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

