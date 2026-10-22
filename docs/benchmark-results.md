## GEF rendering benchmark

- **GPU**: NVIDIA GeForce RTX 3060 Laptop GPU/PCIe/SSE2 (NVIDIA Corporation)
- **Driver**: OpenGL 4.5.0 NVIDIA 610.62
- **Build**: Release, 1280x720, VSync off
- **Method**: 2.0 s warm-up, then 10.0 s measured per scenario (2026-10-22)

| Mesh | Triangles/mesh | Entities | Instancing | Draw calls | FPS | Mean (ms) | p50 (ms) | p99 (ms) | Worst (ms) |
|---|---:|---:|:---:|---:|---:|---:|---:|---:|---:|
| cube | 12 | 1000 | off | 1000 | 705.5 | 1.42 | 1.35 | 2.46 | 8.49 |
| cube | 12 | 1000 | on | 1 | 785.6 | 1.27 | 1.25 | 1.83 | 2.65 |
| cube | 12 | 10000 | off | 10000 | 287.7 | 3.48 | 3.42 | 4.22 | 5.27 |
| cube | 12 | 10000 | on | 1 | 465.1 | 2.15 | 2.20 | 2.86 | 3.51 |
| cube | 12 | 50000 | off | 50000 | 64.6 | 15.48 | 15.43 | 16.43 | 16.95 |
| cube | 12 | 50000 | on | 1 | 143.9 | 6.95 | 6.89 | 7.66 | 10.13 |
| stanford-bunny | 69451 | 1000 | off | 1000 | 60.4 | 16.54 | 16.54 | 17.50 | 18.37 |
| stanford-bunny | 69451 | 1000 | on | 1 | 59.6 | 16.78 | 16.80 | 17.79 | 18.13 |
| stanford-bunny | 69451 | 10000 | off | 10000 | 5.9 | 169.32 | 169.20 | 171.51 | 171.51 |
| stanford-bunny | 69451 | 10000 | on | 1 | 5.8 | 172.61 | 173.09 | 174.90 | 174.90 |
