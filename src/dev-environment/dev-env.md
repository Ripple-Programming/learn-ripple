# Integrated Development Environment for Ripple (IDE)
This IDE is built on Visual Studio Code and enhanced with the Hexagon Plug-in,
providing a powerful environment for developing and debugging C/C++ applications.

- Supports both simulator and target hardware debugging.
- Enables cross-platform development for Hexagon DSP and Arm CPUs.
- Compatible with Android, Linux, and QNX operating systems.

# Installation guide
1. Option 1 - install docker environment.
   See <https://github.qualcomm.com/syakubov/ScopGpt_Build_System/tree/main>  for details

2. Option 2 - manual installation.
All components versions mentioned above valid for 5/6/2025. Highly recomended to use managed docker environment.

2.1 Install 6.2.0.1 Hexagon SDK with Android NDK. Use QPM GUI or command line.

2.2 Install VS Code 1.98.  Pay attention higher versions do not support Hexagon Plug-in properly.
    Download this version from Microsoft repository.

2.3 Install Hexagon Plug-in. Use QPM GUI or command line. Pay attention, #2.1 and #2.2 must be pre-installed.
    See <https://github.qualcomm.com/pages/hexagonsdk/gohexagonsdk/addons/hexagon_ide/1.4.0/index.html>  for details

2.4 Copy RIPPLE Hexagon tools insteed SDK ones

  2.4.1 Rename SDK tools folder:   `<SDK ROOT>/tools/HEXAGON_Tools/8.8.06/Tools`  to `<SDK ROOT>/tools/HEXAGON_Tools/8.8.06/Tools_orig`

  2.4.2 Copy RIPPLE tools folder instead:  `.\21.0-alpha3\Tools`  to `<SDK ROOT>/tools/HEXAGON_Tools/8.8.06/`

  2.4.3 Copy missing lldb-vscode.exe (missing in RIPPLE RELEASE):  `<SDK ROOT>/tools/HEXAGON_Tools/8.8.06/Tools_orig/bin/lldb-vscode.exe`
        to `<SDK ROOT>/tools/HEXAGON_Tools/8.8.06/Tools/bin`
