# <img width="32" height="32" alt="Cull Logo" src="https://github.com/user-attachments/assets/6f2a77f9-1903-42d4-a531-e729fad6e302" /> Cull Engine

![Language](https://img.shields.io/badge/C%2B%2B-20-00599C)
![License](https://img.shields.io/badge/license-Apache--2.0-purple)
![State](https://img.shields.io/badge/State-Development-CC5500)

A CPU rendering engine pairing a real-time scene editor with a multi-threaded offline ray tracer.

| Result |
| --- |
 |  <img width="4000" height="2829" alt="bothCars3" src="https://github.com/user-attachments/assets/b97ede6a-e4c6-4a18-a647-41c15b9169b2" /> |

| Editor |
| --- |
| <img width="1995" height="979" alt="image" src="https://github.com/user-attachments/assets/2e9d100f-cb2e-4ea7-83e5-b84a9bd9266a" /> |

⚠️ **Warning:** Cull Engine is in active development and it's not ready for use. If you want to experiment with it anyway, refer to [run.ps1](run.ps1) and [example.cpp](example/example.cpp)

## Two pipelines
1. **Editor:** live window, rasterized
2. **Ray-Tracer:** produces result in `.ppm` format

## Controls

| Action | Input |
| --- | --- |
| Move camera | `W` `A` `S` `D` |
| Rotate camera | Arrow keys |
| Render to *.ppm* | `R` |
| View solid triangles | Hold `E` |
| Select model | Left mouse button |
| Use gizmo | Select model, drag arrows |
| Change gizmo mode | `G` |
| Delete model | Select model, then `Delete` |
