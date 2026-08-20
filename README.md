# <img width="32" height="32" alt="Cull Logo" src="https://github.com/user-attachments/assets/6f2a77f9-1903-42d4-a531-e729fad6e302" /> Cull Engine

![Language](https://img.shields.io/badge/C%2B%2B-20-00599C)
![License](https://img.shields.io/badge/license-Apache--2.0-purple)

A CPU rendering engine pairing a real-time scene editor with a multi-threaded offline ray-tracer.

| Result |
| --- |
| <img alt="ray-traced" src="https://github.com/user-attachments/assets/b97ede6a-e4c6-4a18-a647-41c15b9169b2" /> |

| Editor |
| --- |
| <img alt="editor" src="https://github.com/user-attachments/assets/2e9d100f-cb2e-4ea7-83e5-b84a9bd9266a" /> |

## Two pipelines

1. **Ray-Tracer:**
    - Sun and multiple point-lights
    - Smooth shading
    - Shadows
    - Reflections
    - Refractions
    - Alpha blending
    - BVH
    - Multi-threading
    - Metallic-roughness materials
    - Textures
    - Vignette (post-processing)
    - Produces result in `.ppm` format

2. **Editor:**
    - Rasterized (solid or wireframe)
    - Runs in live window
    - Move and rotate camera
    - Move, rotate or scale models with gizmo

## CLM (Cull Math)

A custom math library that includes:
- `vec2`, `vec3`, `vec4`, `ivec2`, `uvec2`
- `mat4`
- `quaternion`
- `clamp`, `lerp`

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

## Building & Running

**Requirements:** Windows, MSVC with C++20 support

**Refer to** [run.ps1](run.ps1) and the [example](example) folder
