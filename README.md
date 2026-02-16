# SPH Fluid Simulation

Real-time **2D fluid simulation** using **Smoothed Particle Hydrodynamics** (SPH), rendered with OpenGL. Top-down view with interactive mouse control — drag to push the fluid around and watch density-based coloring change in real time.

![Status](https://img.shields.io/badge/status-in%20development-yellow)
![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![OpenGL](https://img.shields.io/badge/OpenGL-4.3-green)

---

## Demo

<!-- Replace the placeholder below with your actual screenshot/GIF.
     Take a screenshot of the running simulation and save it as:
       screenshots/demo.png   (or demo.gif for an animation)
     Then uncomment the correct line: -->

![Simulation Demo](screenshots/Recording%202026-02-16%20at%2008.22.05.gif)

<!-- For a GIF recording, use tools like:
     - ScreenToGif (Windows): https://www.screentogif.com/
     - gifcap (browser): https://gifcap.dev/
     Save as screenshots/demo.gif and change the path above. -->

> **Top-down fluid view** — color indicates density:
> 🔵 low → 🟢 medium → 🟡 high → 🔴 very high.
> Drag with the mouse (LMB) to push the fluid.

---

## Features

### Physics
- **Weakly Compressible SPH** (WCSPH) with Tait equation of state
- **Multiple smoothing kernels**: Poly6, Spiky, Viscosity, Wendland C2
- **Surface tension** via Continuum Surface Force (CSF) model
- **XSPH velocity correction** for coherent fluid motion
- **Vorticity confinement** to compensate numerical dissipation
- **O(n) spatial hashing** using counting sort (GPU-style, cache-friendly)
- **Symplectic Euler** integration (energy-preserving)
- **Top-down 2D view** — zero gravity, flat plane

### Rendering
- **Screen-space fluid rendering** pipeline:
  1. Gaussian splat particles → density FBO
  2. Multi-pass separable Gaussian blur
  3. Composite pass with density-to-RGB color mapping
- **Density-based color ramp**: blue → cyan → green → yellow → red
- Smooth edge detection for fluid boundaries

### Interaction
- **Mouse drag** (LMB) — push fluid in the direction of cursor movement
- Smooth quadratic force falloff from cursor center
- Configurable force radius and strength

---

## Controls

| Input | Action |
|-------|--------|
| **LMB + drag** | Push fluid in drag direction |
| **1** | Dam-break scene |
| **2** | Pool scene (default, fills screen) |
| **3** | Tall dam scene |
| **R** | Reset current scene |
| **Space** | Pause / Resume |
| **ESC** | Quit |

---

## Build

### Requirements
- CMake 3.20+
- C++17 compiler (MSVC, GCC, or Clang)
- OpenGL 4.3+ compatible GPU
- Git (for dependency fetching)

Dependencies (GLFW + GLAD) are fetched automatically via CMake FetchContent.

### Windows (MSVC)
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
.\SPHFluid.exe
```

### Linux
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./SPHFluid
```

---

## How It Works

The simulation uses **Smoothed Particle Hydrodynamics** — a Lagrangian, mesh-free method where the fluid is represented by thousands of particles. Each frame:

```
buildSpatialHash()        ← O(n) counting-sort grid
    ↓
computeDensityPressure()  ← Poly6 kernel + Tait EOS
    ↓
computeForces()           ← Pressure (Spiky) + Viscosity
    ↓
computeSurfaceTension()   ← CSF model (color field gradient)
    ↓
integrate()               ← Symplectic Euler
    ↓
applyXSPH()              ← Velocity smoothing
    ↓
enforceBoundaries()       ← Wall collision + damping
```

The renderer converts particle positions into a continuous density field (Gaussian splat → blur → composite), then maps density to an RGB color ramp.

### Key Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| Smoothing radius (H) | 12 | Kernel support radius in pixels |
| Rest density (ρ₀) | 1000 | Target density for incompressibility |
| Gas constant (k) | 2000 | Pressure stiffness (Tait EOS) |
| Viscosity (μ) | 60 | Low = water-like, high = honey |
| Surface tension (σ) | 0.3 | Cohesion force at fluid surface |
| Mouse force | 150k | Strength of mouse interaction |

All parameters are in [include/core/SimConfig.hpp](include/core/SimConfig.hpp).

---

## Project Structure

```
fluids/
├── CMakeLists.txt
├── README.md
├── screenshots/          # Screenshots & GIFs for README
├── include/
│   ├── math/             # Vec2, Vec3, Mat3
│   ├── core/             # Particle, SPHSolver, SpatialHash, Kernels, SimConfig
│   ├── physics/          # SurfaceTension, Viscosity, VorticityConfinement
│   └── render/           # Renderer, FluidRenderer
├── src/
│   ├── main.cpp          # Entry point, main loop, mouse input
│   ├── core/             # Solver & boundary implementations
│   ├── physics/          # Physics module implementations
│   └── render/           # OpenGL rendering (3-pass pipeline)
├── shaders/
│   ├── particle.vert/frag
│   ├── metaball.frag
│   └── compute_*.glsl
└── docs/
    └── theory.md         # Mathematical derivations
```

---

## Theory

See [docs/theory.md](docs/theory.md) for complete mathematical derivations including:
- Navier-Stokes → SPH discretization
- Kernel functions and their properties
- Tait equation of state
- CFL stability condition
- Surface tension (CSF model)

---

## Roadmap

- [x] Basic SPH solver with Tait EOS
- [x] Spatial hashing (O(n) neighbor search)
- [x] Surface tension (CSF model)
- [x] XSPH velocity correction
- [x] Vorticity confinement
- [x] Screen-space fluid rendering
- [x] Top-down view with density-based coloring
- [x] Mouse interaction (drag to push fluid)
- [ ] GPU compute shader acceleration
- [ ] PCISPH (predictive-corrective incompressible SPH)
- [ ] DFSPH (divergence-free SPH)
- [ ] 3D simulation + Marching Cubes
- [ ] Multiphase fluids (water + oil)

---

## License

MIT

## References

1. Müller, M., Charypar, D., & Gross, M. (2003). *Particle-based fluid simulation for interactive applications.*
2. Monaghan, J. J. (1992). *Smoothed Particle Hydrodynamics.*
3. Solenthaler, B., & Pajarola, R. (2009). *Predictive-Corrective Incompressible SPH.*
4. Bender, J., & Koschier, D. (2015). *Divergence-free Smoothed Particle Hydrodynamics.*
