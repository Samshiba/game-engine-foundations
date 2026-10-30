## GEF rendering benchmark

- **GPU**: NVIDIA GeForce RTX 3060 Laptop GPU/PCIe/SSE2 (NVIDIA Corporation)
- **Driver**: OpenGL 4.5.0 NVIDIA 610.62
- **Build**: Release, 1280x720, VSync off
- **Method**: 2.0 s warm-up, then 10.0 s measured per scenario (2026-10-30)

| Mesh | Triangles/mesh | Entities | Instancing | Draw calls | Binds (pipeline/VAO/UBO) | FPS | Mean (ms) | p50 (ms) | p99 (ms) | Worst (ms) |
|---|---:|---:|:---:|---:|:---:|---:|---:|---:|---:|---:|
| cube | 12 | 1000 | off | 1000 | 1/1/2 | 732.7 | 1.36 | 1.31 | 2.21 | 4.39 |
| cube | 12 | 1000 | on | 1 | 1/1/2 | 861.2 | 1.16 | 1.14 | 1.54 | 4.72 |
| cube | 12 | 10000 | off | 10000 | 1/1/2 | 320.1 | 3.12 | 3.07 | 3.77 | 4.89 |
| cube | 12 | 10000 | on | 1 | 1/1/2 | 553.3 | 1.81 | 1.93 | 2.51 | 3.04 |
| cube | 12 | 50000 | off | 50000 | 1/1/2 | 68.4 | 14.63 | 14.59 | 16.04 | 17.59 |
| cube | 12 | 50000 | on | 1 | 1/1/2 | 189.5 | 5.28 | 5.20 | 6.31 | 9.14 |
| stanford-bunny | 69451 | 1000 | off | 1000 | 1/1/2 | 60.7 | 16.48 | 16.49 | 17.05 | 17.53 |
| stanford-bunny | 69451 | 1000 | on | 1 | 1/1/2 | 60.3 | 16.58 | 16.58 | 17.23 | 17.93 |
| stanford-bunny | 69451 | 10000 | off | 10000 | 1/1/2 | 6.1 | 164.21 | 164.09 | 166.87 | 166.87 |
| stanford-bunny | 69451 | 10000 | on | 1 | 1/1/2 | 6.0 | 166.49 | 166.40 | 169.25 | 169.25 |
