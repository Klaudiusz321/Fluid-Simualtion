# SPH Fluid Simulation

Real-time 2D fluid simulation using **Smoothed Particle Hydrodynamics** (SPH) with OpenGL rendering.

![Status](https://img.shields.io/badge/status-in%20development-yellow)
![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![OpenGL](https://img.shields.io/badge/OpenGL-4.3-green)

## Features

### Physics
- **Weakly Compressible SPH** (WCSPH) with Tait equation of state
- **Multiple smoothing kernels**: Poly6, Spiky, Viscosity, Wendland C2, Cubic Spline
- **Surface tension** via Continuum Surface Force (CSF) model
- **XSPH velocity correction** for coherent fluid motion
- **Vorticity confinement** to compensate numerical dissipation
- **Spatial hashing** for O(n) neighbor search
- **Adaptive timestep** based on CFL condition
- **Symplectic Euler** integration (energy-preserving)

### Rendering
- **Point sprite** rendering with velocity-dependent coloring
- **Screen-space fluid rendering** (metaball accumulation + compositing)
- **GPU compute shaders** (prepared for 100k+ particle simulations)

### Controls
| Key | Action |
|-----|--------|
| Left Click | Emit particles |
| R | Reset simulation |
| Space | Pause / Resume |
| 1 | Point rendering mode |
| 2 | Fluid surface rendering mode |
| ESC | Quit |

## Build

### Requirements
- CMake 3.20+
- C++17 compiler (MSVC, GCC, or Clang)
- OpenGL 4.3+ compatible GPU
- Git (for dependency fetching)

### Windows (MSVC)
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Linux
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

Dependencies (GLFW + GLAD) are fetched automatically via CMake FetchContent.

## Project Structure

```
fluids/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── math/          # Vec2, Vec3, Mat3
│   ├── core/          # Particle, SPHSolver, SpatialHash, Kernels
│   ├── physics/       # SurfaceTension, Viscosity, VorticityConfinement
│   └── render/        # Renderer, FluidRenderer
├── src/
│   ├── main.cpp       # Entry point + main loop
│   ├── core/          # Solver & boundary implementations
│   ├── physics/       # Physics module implementations
│   └── render/        # OpenGL rendering
├── shaders/
│   ├── particle.vert/frag     # Point sprite shaders
│   ├── metaball.frag          # Screen-space fluid
│   └── compute_*.glsl         # GPU compute shaders
└── docs/
    └── theory.md      # Mathematical derivations
```

## Theory

See [docs/theory.md](docs/theory.md) for complete mathematical derivations including:
- Navier-Stokes → SPH discretization
- Kernel functions and their properties
- Equation of state analysis
- CFL stability condition
- Surface tension (CSF model)

## Roadmap

- [x] Basic SPH solver with Tait EOS
- [x] Spatial hashing (O(n) neighbor search)
- [x] Surface tension (CSF model)
- [x] XSPH velocity correction
- [x] Vorticity confinement
- [x] Screen-space fluid rendering
- [ ] GPU compute shader acceleration
- [ ] PCISPH (predictive-corrective incompressible SPH)
- [ ] DFSPH (divergence-free SPH)
- [ ] 3D simulation + Marching Cubes
- [ ] Multiphase fluids (water + oil)
- [ ] Rigid body coupling
- [ ] Non-Newtonian fluids

## License

MIT

## References

1. Müller, M., Charypar, D., & Gross, M. (2003). *Particle-based fluid simulation for interactive applications.*
2. Monaghan, J. J. (1992). *Smoothed Particle Hydrodynamics.*
3. Solenthaler, B., & Pajarola, R. (2009). *Predictive-Corrective Incompressible SPH.*
4. Bender, J., & Koschier, D. (2015). *Divergence-free Smoothed Particle Hydrodynamics.*
