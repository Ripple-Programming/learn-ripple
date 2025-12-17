# Adapt SDK build system to support Ripple

1. Open `<SDK ROOT>/build/cmake/hexagon_toolchain.cmake`
2. Add `-fenable-ripple --save-temps`  switches to `COMMON_FLAGS`

![Apdate COMMON_FLAGS](./quick_start_0.png)

# Quick Start
![Open VS Code](./quick_start_1.png)
![Create Hexagon Project](./quick_start_2.png)
![C Project View](./quick_start_3.png)
![Run test](./quick_start_4.png)
![Debugging](./quick_start_5.png)