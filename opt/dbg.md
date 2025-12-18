# Debugging

Being part of clang, Ripple is compatible with LLVM's debugger, LLDB.
This section explains how to set up the LLDB plugin on VS Code.

## From Microsoft VS Code
VS Code(R) is a modern integrated development environment (IDE) published by Microsoft.
It supports LLDB through an extension called "LLDB DAP".

To install the extension:
- go to the `Extensions` menu (typically on the left hand side)
- search for `LLDB DAP`
- An `LLDB DAP` extension should appear, published by `LLVM`. Click on its `install` button.
- Open settings (File->Preferences->Settings), in the Workspace tab, and change Lldb-dap: Executable-path to point to the lldb-dap[.exe] you want to use.


Set up .vscode/launch.json as appropriate in your project, as for example:
```json
{
    // Sample launch.json for Hexagon tools
    "version": "0.2.0",
    "configurations": [
        {
            "name": "(lldb-dap) Launch",
            "type": "lldb-dap",
            "request": "launch",
            "program": "${workspaceFolder}/a.out",
            "args": [],
            "stopOnEntry": false,
            "cwd": "${workspaceFolder}",
            "env": [],
        }
    ]
}
```
More examples are available in the Hexagon clang release, `Examples/vscode/*/.vscode`

More info about launch.json is at https://code.visualstudio.com/docs/debugtest/debugging-configuration .

More info about launch.json options for the LLDB DAP extension is in the readme at https://github.com/llvm/vscode-lldb.

---
*Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear*
