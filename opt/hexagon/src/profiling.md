# Profiling on Hexagon(R)

## Counting cycles in the code
The simplest way to measure performance is by reading cycle counters,
using clang's standard builtin:

```C
unsigned long long __builtin_readcyclecounter();
```

For instance, the following code measures the cycle count of a 2-d nested loop:
```C
#include<ripple.h>
/* We assume the existence of some function float f(float) in this module */
void foo(float * in, float * out, size_t n) {
  ripple_block_t B = ripple_set_block_shape(HVX_LANES, 32);
  unsigned long long start, end;
  start = __builtin_readcyclecounter();
  for (size_t i = 0; i < n; ++i) {
    ripple_parallel(B, 0);
    for (size_t j = 0; j < n; ++j) {
      a[n*i + j] = f(b[n*i + j]);
    }
  }
  end = __builtin_readcyclecounter();
  printf("2-d loop took %llu cycles.\n", end - start);
}
```

## Profiling influences performance
As often, observing a system changes it.
In this section, we share insights about how and when using the profiling API could reduce the performance of the profiled code.

### Instruction scheduling overhead
While it is tempting to try and profile small portions of code,
we need to consider that the calls to `__builtin_readcyclecounter()` themselves result in instructions.
As a result, the compiler has to schedule these profiling instructions along with the instructions you want to profile.
This can result in a slightly higher number of VLIW packets,
which will increase the number of cycles used for your computation.

### Risk of performance degradation from instruction pipelining
If we are trying to accumulate cycle counts in a specific region of code (for instance within a loop), we need to consider that fine-grained instruction scheduling can be affected by the presence of profiling code in the inner loop.
For instance, if we wanted to accumulate the cycle counts of the `j` iterations
in the above examples, we would write the following code:

```C
1: void foo(float * in, float * out, size_t n) {
2:   ripple_block_t B = ripple_set_block_shape(HVX_LANES, 32);
3:   unsigned long long start, end, accumulated;
4:   for (size_t i = 0; i < n; ++i) {
5:     #pragma clang loop unroll(2)
6:     ripple_parallel(B, 0);
7:     for (size_t j = 0; j < n; ++j) {
8:       start = __builtin_readcyclecounter();
9:       a[n*i + j] = f(b[n*i + j]);
10:      end = __builtin_readcyclecounter();
11:      accumulated += end - start;
12:    }
13:  }
14:  printf("2-d loop too %llu cycles.\n", accumulated);
15:}
```

We note that, as far as Ripple is concerned, this code is correct.
Since `accumulated` doesn't depend upon `j`,
it is interpreted as a scalar value,
and the statement line 11 is also a scalar increment.
The accumulation will happen only once per vector computation
(n/32 times total), which is the intended behavior.

However, the profiling code can reduce the code performance,
compared to the unprofiled code,
by constraining the compiler's freedom to schedule instructions efficiently.
Hexagon being a VLIW machine, the Hexagon compiler needs to schedule instructions as packets of parallel instructions.
The more instructions can be reordered,
the better the chances for the compiler's VLIW scheduler
to create well-utilized packets.

In the example above, calls to `__builtin_readcyclecounter()` cannot be rescheduled w.r.t. other instructions, to let developers measure exactly what they want.
This also means that other instructions cannot go past a call to `__builtin_readcyclecounter()`. As a result, instructions from line 9 cannot be combined with instructions from other lines.
Instructions from line 9 at a certain `j` iteration also cannot be combined with instructions from the next `j` iteration
(such cross-iteration combination is normally enabled by the `unroll` loop pragma line 5).
Hence, the compiler has much less freedom to pack instructions into VLIW packets as efficiently as with the unprofiled code.

## Application profiling with `itrace`
itrace is a low-level profiling tool in the Hexagon SDK that provides:

- PMU (Performance Monitoring Unit) event capture for DSP and optionally CPU.
- Markers and sections for custom instrumentation.
- Periodic sampling of counters.
- Ability to log traces in Perfetto-compatible formats for visualization.
It’s more flexible than sysmon but requires code integration and configuration.

itrace is located in the Hexagon SDK under
```
libs/itrace/
```

To use itrace, lease consult the HTML documentation in the SDK:
`/docs/doxygen/itrace/index.html`

Event IDs are defined in the following files:
- DSP PMU: itrace_dsp_events_pmu.h
- CPU events: itrace_cpu_events_la.h

### Basic workflow
#### Initialize profiler
```C
itrace_profiler_handle_t dsp_profiler_handle;
itrace_profiler_create(&dsp_profiler_handle, ITRACE_DOMAIN_DSP);

```
#### Register the PMU events you want to see
```C
itrace_add_event(dsp_profiler_handle, ITRACE_DSP_EVENT_L1D_CACHE_MISS);
itrace_add_event(dsp_profiler_handle, ITRACE_DSP_EVENT_INSTR_COUNT);
```

#### Start/stop profiling
```C
itrace_start(dsp_profiler_handle);
// Run your workload
itrace_stop(dsp_profiler_handle);
itrace_dump(dsp_profiler_handle, "trace_output.json");
```
Output can be JSON or protobuf for Perfetto visualization.

### Instrumentation
Add markers for timeline correlation:
```C

itrace_add_marker(dsp_profiler_handle, "Begin DSP section");
itrace_add_section(dsp_profiler_handle, "FFT compute");
// ... code ...
itrace_end_section(dsp_profiler_handle);

```
Markers work on CPU and DSP; DSP-side markers give best granularity.

### Advanced features
___Periodic sampler:___ sample PMU counters at fixed intervals.

___Multi-pass mode:___ if you need >8 PMU events, itrace cycles through sets automatically.

___Perfetto integration:___ itrace can emit traces compatible with Chrome’s about://tracing or Perfetto UI.

### Constraints

- Only one PMU client at a time (itrace vs sysmon vs SDP).
If you see registration errors, stop other profilers or reboot.
- itrace is per-application; unlike sysmon, it doesn’t give system-wide DSP load.
- Some SDK versions (e.g., v69/v75) have known itrace bugs — please check release notes.

__Disclaimer__: `sysmon` is entirely independent from Ripple,
and Ripple support won't be able to assist you with `sysmon` issues.


## Profiling whole DSP execution with `sysmon`
Hexagon’s SysMon (“sysMonApp”) is the command‑line profiler bundled with the Hexagon SDK that lets you capture DSP workload/utilization and PMU (Performance Monitoring Unit) counters from different Q6 subsystems (ADSP/CDSP/SDSP/MDSP) across Android, Linux LE, QNX and virtualized (Gunyah) targets. Below is a concise, engineer‑friendly “how to” that you can use directly.

### Where to find sysmon in the SDK

SysMon binaries are shipped under the SDK install at:
```bash
<HEXAGON_SDK_ROOT>/tools/utils/sysmon/
```

Typical artifacts include:

sysMonApp (Android native / Linux LE)
sysMonAppQNX (QNX)
sysMon_DSP_Profiler_v2.apk (Android GUI app, Java)
These names can vary slightly by SDK version (6.x), but the folder location is consistent.

### Put the binary on your device/target
#### Android (ADB)

Ensure your device is accessible and /system is writable.
Push the binary and make it executable:

```bash
adb root
adb remount
adb push $HEXAGON_SDK_ROOT/tools/utils/sysmon/sysMonApp /data/local/tmp/
adb shell chmod 777 /data/local/tmp/sysMonApp
```

Heads‑up: To profile from Android (guest VM) into the DSP (PVM), FastRPC daemon must be running on the Android side (e.g., /system/bin/adsprpcd). If that daemon isn’t present, you can run SysMon directly on the PVM (e.g., QNX) instead.

#### QNX (or Linux‑LE)

Copy the matching sysMonAppQNX (or sysMonAppLE) onto the target filesystem and run it there.


### Quickstart: capture high‑level DSP utilization
Pick your DSP with --q6 (defaults to adsp). Options typically include adsp, cdsp, sdsp, mdsp.

#### QNX example
```bash
sysMonAppQNX profiler --q6 adsp --duration 60
# Outputs (by default): /data/sysmon.bin
```

`--duration <sec>` controls capture length (default ~10s).
For MDSP, the file name becomes `sysmon_mdsp.bin`.

#### Android native example
```bash
shell /data/local/tmp/sysMonApp profiler --q6 cdsp --duration 30
# Then pull the bin:
adb pull /data/sysmon.bin
```
If `--q6 cdsp` [fails on your device](https://mysupport.qualcomm.com/supportforums/s/question/0D5dK000004mN1kSAE/i-installed-hexagon-6002-to-monitor-npu-usage-on-a-mi-14-device-sysmon-works-with-the-adsp-flag-but-fails-with-cdsp-since-ai-app-monitoring-needs-the-cdsp-flag-how-can-i-fix-this), check chip support/firmware and FastRPC daemon availability; there are known device‑specific limitations and you may need OEM images or support to enable CDSP monitoring.

### Thread‑Level Performance (PMU) sampling
To sample PMU counters and per‑thread stats (TLP = thread‑level performance), use the `tlp` subcommand.

#### QNX example (TLP + overall profile)

```bash
sysMonAppQNX tlp \
  --profile 1 \
  --samplingPeriod 1 \
  --q6 adsp \
  --defaultSetEnable 3 \
  --duration 60

# Outputs: /data/sysmontlp_adsp.bin (or sysmontlp_mdsp.bin for MDSP)
```

Key flags:
- `--samplingPeriod` <ms> controls sample period (default ~50 ms).
- `--defaultSetEnable` selects logging preset (common values 0–3;
use 3 for rich PMU capture on newer tools).
The public docs describe 0/1/2 modes;
field practice often uses 3 with newer SysMon builds.


__Important constraint:__ only one PMU client can own the hardware at a time.
Don’t run SysMon concurrently with other PMU consumers (e.g., SDP, itrace).
If you see “register failed” errors, stop the other tool
or reboot the system to release PMU.


### Typical workflow (Android device → CDSP/ADSP)
#### Prep

Push sysMonApp to `/data/local/tmp`.
Confirm the right FastRPC daemon is present for your target DSP
(e.g., adsprpcd for ADSP; device‑specific setup for CDSP).

#### Record
```bash
adb shell /data/local/tmp/sysMonApp profiler --q6 adsp --duration 20
adb shell /data/local/tmp/sysMonApp tlp --profile 1 --samplingPeriod 2 --q6 adsp --defaultSetEnable 3 --duration 20
```

#### Collect
```bash
adb pull /data/sysmon.bin .
adb pull /data/sysmontlp_adsp.bin
```

#### Analyze

SysMon’s binary logs can be parsed using sysmon's parser `sysmon_parser`.

```bash
# Pull the binary log from the device
adb pull /data/sysmon.bin
# Create HTML pages --> sysmon_report_YYYYMMDD_HHMM/sysmon_report.html
sysmon_parser sysmon.bin
```
Note that the `.bin` name varies based on monitoring options.

#### Troubleshooting tips

Some common sysmon-related issues and their solutions are described in
mysupport.qualcomm.com and stack overflow.

__Disclaimer__: `sysmon` is entirely independent from Ripple,
and Ripple support won't be able to assist you with `sysmon` issues.

## Whole-system profiling with Qualcomm (R) Profiler
The Qualcomm Profiler is a higher-level tool to monitor the whole system.
Please consult the [product webpage](https://www.qualcomm.com/developer/software/qualcomm-profiler#profiler) for more information and support.

## Profiling in QNN-HTP
Profiling within QNN-HTP is detailed for Qualcomm internal developers
in the `docs/profiling.md` document in the HexNN repository.

---
Hexagon is a registered trademark of Qualcomm Incorporated.
---
*Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear*