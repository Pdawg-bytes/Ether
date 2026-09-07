# Ether
A real-time, portable, and deterministic software ray-tracer and ray-marcher written in C++.

## Features
### Lighting
- **Analytical Direct Lighting:** Deterministic evaluation for point and directional light sources.
- **Physically Based Shading:** Full Cook-Torrance BRDF evaluation via GGX.
- **SDF Ambient Occlusion:** Fast proximity occlusion calculated via ray-marched distance fields across all primitives.

### Materials
- **Physical Properties:** Support for roughness, metalness, and emission properties.
- **Texture Mapping:** Optional mapping for albedo, roughness, emission, and bump maps (supports explicit UVs on polygons and planar projection on primitives).
- **Glass Refraction:** `IOR` and `Transmission` properties for emulating transparent objects.

### Geometry
- **Hybrid Representation:** Scene-wide Surface Area Heuristic BVH that captures individual primitives alongside an SDF evaluator for CSG trees.
- **Mesh Loading:** `.obj` + `.mtl` file parser for meshes.
- **Analytic Primitives:** Sphere, Box, Plane, Torus, and Cylinder primitives.
- **Constructive Solid Geometry:** Full boolean operations supported in CSG trees for all primitives.

## Platforms
- Windows (With multithreading support `#define MULTI_THREADED_RENDERING` in `Raytracer.h`)
- Nintendo 3DS (Runs poorly, more of a proof-of-concept)

## Compilation
### Windows
1. Download and install Visual Studio 2026 with the C++ development workload **along with Clang-CL support**.
2. Open the Ether solution.
3. Compile.
### 3DS
1. Download and install the devkitPro toolchain and include 3DS support.
2. Enter the Ether directory and run `make`.

## Usage
In order to use `.obj` files, you must place them inside of a folder named `Data` in the root of this repository. All paths in Ether are relative to this folder. Loading an object from `Teapot/teapot.obj` points to `</path/to/repository>/Data/Teapot/teapot.obj`. The 3DS build automatically includes this entire folder in the RomFS of the 3dsx.

## Examples
### OBJ Files
<img width="1593" height="984" alt="image" src="https://github.com/user-attachments/assets/ac4a9495-3d6f-4113-9dcb-84348f169124" />

### Cornell Box
<img width="1597" height="984" alt="image" src="https://github.com/user-attachments/assets/d4a91045-f746-4af3-bdae-015b034bb773" />

### CSG Scene
<img width="1595" height="987" alt="image" src="https://github.com/user-attachments/assets/c7a8dc8b-1681-4d98-a036-a1526012670f" />

## CSG and primitives mixed
<img width="1591" height="982" alt="image" src="https://github.com/user-attachments/assets/f2a686b3-45e5-4600-a785-17457b9c1ccb" />

### OBJ Scene on Nintendo 3DS
<img width="2880" height="2160" alt="image" src="https://github.com/user-attachments/assets/45103125-674a-48ca-be16-655ce858f13e" />

### BVH Visualization
<img width="1591" height="986" alt="image" src="https://github.com/user-attachments/assets/10900da7-0747-4919-aa88-039d8dbd8bbc" />

