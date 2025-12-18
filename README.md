## Documentation source for Ripple.

This repository hosts the documentation for **Ripple**, and LLVM-based project.
The main documentation is hosted
here: <https://github.com/pages/Ripple-Programming/learn-ripple/>.

If you are looking for the Ripple source code (an LLVM fork),
go [here](https://github.com/Ripple-Programming/llvm-project).

### HOWTO: Build the docs

1. `cargo install mdbook`
2. `mdbook build -d build`

## Hexagon Optimization guide

Build:
```bash
$ cd opt/hexagon
$ mdbook build -d build
```

