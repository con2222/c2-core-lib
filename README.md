[Читать на русском](README.ru.md) | [Read in English](README.md)

Foundational library (Core) built for custom C++ game engines and real-time interactive applications.

**Status:** Under active development. More modules will be added over time.

### Current Features

1. **`C2Core::Time` (Time Management & Frame Pacing)**
* **High-Precision DeltaTime:** Accurate frame duration tracking via system clocks.
* **Fixed Timestep Accumulator:** Decouples physics and simulation updates from the rendering framerate for deterministic logic.
* **Hybrid FPS Limiter:** Smart frame-rate capping that combines CPU-friendly OS sleep (`sleep_for`) with ultra-precise spin-locking (`yield`) to prevent micro-stuttering.
* **Performance Analytics (1% Low):** Built-in ring buffer tracking the last 1000 frames to compute average/min/max frame times and real-time 1% Low FPS.

2. **`C2Core::Profiler` (Hardware-Level Profiling & Benchmarking)**
* **RDTSC Precision:** Utilizes `__rdtsc` / `__rdtscp` intrinsics with memory fences (`_mm_lfence`) for true CPU cycle-level measurement and core-migration detection.
* **RAII Scoped Profiler:** Block execution timing with automatic baseline overhead subtraction and dynamic CPU frequency calibration. Supports multiple units (Cycles, Nanoseconds, Milliseconds, etc.).
* **Benchmarking Suite:** Features `runBenchmark` with warmup iterations and a `StatsAccumulator` computing Min, Max, Mean, and Standard Deviation (StdDev) on the fly.
* **Thread Pinning:** Cross-platform `pinThreadToCore` (Windows/Linux) to lock execution to a specific CPU core, preventing OS scheduler inaccuracies during critical benchmarks.

### How to Integrate

The library is compiled as a **static library (.lib / .a)** using CMake.

1. Copy the library folder into your project's dependencies directory (e.g., `vendor/c2-core-lib`).
2. In your main `CMakeLists.txt`, register the subdirectory:
```cmake
add_subdirectory(vendor/c2-core-lib)
```

3. Link the library to your main executable target:
```cmake
target_link_libraries(${APP_NAME} PRIVATE C2Core)
```

*Include paths (`#include <C2Core/...>`) are propagated automatically via CMake's PUBLIC interface configuration.*

### Practical Example

A complete, real-world example demonstrating how to integrate the time context, manage the spin loop pacing, and render the resulting 1% Low statistics inside an ImGui interface can be found directly in the **[Dawn-Boids-3D](https://github.com/con2222/Dawn-Boids-3D)** repository (refer to `App.cpp` and `App.hpp`).