## GEF rendering benchmark

- **GPU**: NVIDIA GeForce RTX 3060 Laptop GPU/PCIe/SSE2 (NVIDIA Corporation)
- **Driver**: OpenGL 4.5.0 NVIDIA 610.62
- **Build**: Release, 1280x720, VSync off
- **Method**: 2.0 s warm-up, then 10.0 s measured per scenario (2026-11-07)

| Mesh           | Triangles/mesh | Entities | Instancing | Draw calls | Binds (pipeline/VAO/UBO) |   FPS | Mean (ms) | p50 (ms) | p99 (ms) | Worst (ms) | Submit CPU (ms) | Submit GPU (ms) |
|----------------|---------------:|---------:|:----------:|-----------:|:------------------------:|------:|----------:|---------:|---------:|-----------:|----------------:|----------------:|
| cube           |             12 |     1000 |    off     |       1000 |          1/1/2           | 764.1 |      1.31 |     1.28 |     1.96 |       9.49 |            0.18 |            0.02 |
| cube           |             12 |     1000 |     on     |          1 |          1/1/2           | 831.1 |      1.20 |     1.18 |     1.59 |       2.24 |            0.02 |            0.01 |
| cube           |             12 |    10000 |    off     |      10000 |          1/1/2           | 314.6 |      3.18 |     3.11 |     3.99 |       5.90 |            1.65 |            0.29 |
| cube           |             12 |    10000 |     on     |          1 |          1/1/2           | 502.0 |      1.99 |     2.09 |     2.81 |       3.28 |            0.03 |            0.05 |
| cube           |             12 |    50000 |    off     |      50000 |          1/1/2           |  67.5 |     14.81 |    14.78 |    15.87 |      17.08 |            8.57 |            6.01 |
| cube           |             12 |    50000 |     on     |          1 |          1/1/2           | 186.8 |      5.35 |     5.29 |     6.13 |       8.45 |            0.04 |            0.24 |
| stanford-bunny |          69451 |     1000 |    off     |       1000 |          1/1/2           |  60.2 |     16.61 |    16.60 |    17.72 |      18.12 |            0.25 |           16.56 |
| stanford-bunny |          69451 |     1000 |     on     |          1 |          1/1/2           |  59.0 |     16.95 |    16.95 |    17.93 |      18.66 |            0.05 |           16.90 |
| stanford-bunny |          69451 |    10000 |    off     |      10000 |          1/1/2           |   5.8 |    170.95 |   170.87 |   175.09 |     175.09 |            1.81 |          170.44 |
| stanford-bunny |          69451 |    10000 |     on     |          1 |          1/1/2           |   5.7 |    174.52 |   174.08 |   177.25 |     177.25 |            0.06 |          174.05 |

**Submit CPU**: time spent issuing the frame's commands to the driver.
**Submit GPU**: time between two GPU timestamps around the same commands.
About equal: draw call bound. GPU much higher: GPU bound.
