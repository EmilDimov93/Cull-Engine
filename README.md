# <img width="32" height="32" alt="Cull Logo" src="https://github.com/user-attachments/assets/6f2a77f9-1903-42d4-a531-e729fad6e302" /> Cull Engine

![Language](https://img.shields.io/badge/C%2B%2B-20-00599C)
![License](https://img.shields.io/badge/license-Apache--2.0-purple)
![State](https://img.shields.io/badge/State-Development-CC5500)

A CPU rendering engine pairing a real-time scene editor with a multi-threaded offline ray tracer.

| Editor | Result |
| --- | --- |
| <img width="994" height="991" alt="beetleCowEditor" src="https://github.com/user-attachments/assets/d04780f2-61da-4cf0-b6d3-43cdf62b6a3c" /> | <img width="1000" height="1000" alt="beetleCowRayTracer" src="https://github.com/user-attachments/assets/0f818a01-01b5-46df-a384-5107157c3524" /> | 

⚠️ **Warning:** Cull Engine is in active development and it's not ready for use. If you want to experiment with it anyway, refer to [run.ps1](run.ps1) and [example.cpp](src/example.cpp)

## Two pipelines
1. **Editor:** live window, rasterized
2. **Ray-Tracer:** produces result in `.ppm` format

## Controls

| Action | Input |
| --- | --- |
| Move camera | `W` `A` `S` `D` |
| Rotate camera | Arrow keys |
| Render to `.ppm` | `R` |
| Select model | Left mouse button |
| Use gizmo | Select model, use arrows |
| Change gizmo mode | `G` |
| Delete model | Select model, then `Delete` |
