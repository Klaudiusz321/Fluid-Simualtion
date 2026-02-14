# Theoretical Foundations of the SPH Fluid Simulator

## 1. Navier-Stokes Equations

The motion of an incompressible Newtonian fluid is governed by:

### Conservation of Momentum

$$\rho \frac{D\mathbf{v}}{Dt} = -\nabla p + \mu \nabla^2 \mathbf{v} + \rho \mathbf{g}$$

where:
- $\rho$ — fluid density
- $\mathbf{v}$ — velocity field
- $p$ — pressure
- $\mu$ — dynamic viscosity
- $\mathbf{g}$ — gravitational acceleration
- $\frac{D}{Dt} = \frac{\partial}{\partial t} + \mathbf{v} \cdot \nabla$ — material derivative

### Conservation of Mass (Continuity)

$$\frac{D\rho}{Dt} + \rho \nabla \cdot \mathbf{v} = 0$$

For incompressible flow: $\nabla \cdot \mathbf{v} = 0$.

---

## 2. SPH Discretization

### 2.1 Fundamental SPH Interpolation

Any field quantity $A$ at position $\mathbf{r}$ is approximated as:

$$A(\mathbf{r}) = \sum_j m_j \frac{A_j}{\rho_j} W(\mathbf{r} - \mathbf{r}_j, h)$$

where:
- $j$ — index over neighboring particles
- $m_j$ — mass of particle $j$
- $\rho_j$ — density of particle $j$
- $W$ — smoothing kernel with support radius $h$

### 2.2 Key property: gradients transfer to the kernel

$$\nabla A(\mathbf{r}) = \sum_j m_j \frac{A_j}{\rho_j} \nabla W(\mathbf{r} - \mathbf{r}_j, h)$$

This is the core insight of SPH — derivatives of field quantities become derivatives of the known kernel function.

---

## 3. Smoothing Kernels

### 3.1 Poly6 Kernel (Müller et al., 2003)

Used for density estimation:

$$W_{poly6}(r, h) = \frac{315}{64\pi h^9} \begin{cases} (h^2 - r^2)^3 & 0 \leq r \leq h \\ 0 & r > h \end{cases}$$

**Gradient:**
$$\nabla W_{poly6} = -\frac{945}{32\pi h^9} (h^2 - r^2)^2 \, \mathbf{r}$$

**Laplacian:**
$$\nabla^2 W_{poly6} = -\frac{945}{32\pi h^9} (h^2 - r^2)(3h^2 - 7r^2)$$

### 3.2 Spiky Kernel (for pressure)

The Poly6 gradient vanishes at $r = 0$, causing clustering. The Spiky kernel fixes this:

$$\nabla W_{spiky}(r, h) = -\frac{45}{\pi h^6} (h - r)^2 \hat{\mathbf{r}}$$

### 3.3 Viscosity Kernel

$$\nabla^2 W_{visc}(r, h) = \frac{45}{\pi h^6} (h - r)$$

### 3.4 Wendland C2 Kernel

A more modern, numerically stable kernel:

$$W_{C2}(q) = \frac{7}{4\pi h^2} \left(1 - \frac{q}{2}\right)^4 (1 + 2q), \quad q = \frac{r}{h}, \quad q \leq 2$$

**Advantages over Poly6:**
- No tensile instability
- No particle clumping
- Positive definite
- Smooth with compact support

---

## 4. Density Estimation

$$\rho_i = \sum_j m_j \, W(\mathbf{r}_i - \mathbf{r}_j, h)$$

Note: particle $i$ contributes to its own density (self-contribution).

---

## 5. Equation of State (Pressure)

### 5.1 Ideal Gas (simple)

$$p = k(\rho - \rho_0)$$

where $k$ is the stiffness constant and $\rho_0$ is the rest density.

### 5.2 Tait Equation (more physical)

$$p = B \left[\left(\frac{\rho}{\rho_0}\right)^\gamma - 1\right]$$

where $B = \frac{\rho_0 c_s^2}{\gamma}$, $c_s$ is the speed of sound, and $\gamma = 7$ for water.

### 5.3 PCISPH / DFSPH (advanced)

We will eventually implement prediction-correction schemes that enforce $\nabla \cdot \mathbf{v} = 0$ iteratively, achieving near-perfect incompressibility.

---

## 6. Forces

### 6.1 Pressure Force

$$\mathbf{F}_i^{pressure} = -\sum_j m_j \frac{p_i + p_j}{2\rho_j} \nabla W_{spiky}(\mathbf{r}_i - \mathbf{r}_j, h)$$

The symmetric pressure $(p_i + p_j)/2$ ensures Newton's third law (action = reaction).

### 6.2 Viscosity Force

$$\mathbf{F}_i^{viscosity} = \mu \sum_j m_j \frac{\mathbf{v}_j - \mathbf{v}_i}{\rho_j} \nabla^2 W_{visc}(\mathbf{r}_i - \mathbf{r}_j, h)$$

### 6.3 Gravity

$$\mathbf{F}_i^{gravity} = \rho_i \, \mathbf{g}$$

### 6.4 Surface Tension (CSF Model)

Based on Brackbill, Kothe, Zemach (1992):

1. **Color field:** $c_i = \sum_j \frac{m_j}{\rho_j} W_{ij}$
2. **Surface normal:** $\mathbf{n}_i = \nabla c_i$
3. **Curvature:** $\kappa_i = -\frac{\nabla^2 c_i}{|\mathbf{n}_i|}$
4. **Force:** $\mathbf{F}_i^{surface} = \sigma \kappa_i \hat{\mathbf{n}}_i$

Applied only where $|\mathbf{n}_i|$ exceeds a threshold (near the free surface).

---

## 7. XSPH Velocity Correction

Monaghan (1989) proposed smoothing the velocity field:

$$\tilde{\mathbf{v}}_i = \mathbf{v}_i + \varepsilon \sum_j \frac{m_j}{\bar{\rho}_{ij}} (\mathbf{v}_j - \mathbf{v}_i) W_{ij}$$

where $\bar{\rho}_{ij} = (\rho_i + \rho_j) / 2$ and $\varepsilon \in [0, 1]$.

This reduces noise and makes the simulation look much more coherent without affecting the physical equations.

---

## 8. Vorticity Confinement

Numerical dissipation kills small vortices. We re-inject rotational energy:

In 2D, vorticity is a scalar:
$$\omega_i = \sum_j \frac{m_j}{\rho_j} (\mathbf{v}_j - \mathbf{v}_i) \times \nabla W_{ij}$$

The confinement force:
$$\mathbf{F}_i^{vort} = \varepsilon \left(\hat{\mathbf{N}}_i \times \omega_i\right)$$

where $\mathbf{N}_i = \nabla |\omega_i|$ and $\hat{\mathbf{N}}_i$ is its unit vector.

---

## 9. Time Integration

### 9.1 Symplectic Euler (used in this implementation)

$$\mathbf{v}^{n+1} = \mathbf{v}^n + \frac{\mathbf{F}}{\rho} \Delta t$$
$$\mathbf{x}^{n+1} = \mathbf{x}^n + \mathbf{v}^{n+1} \Delta t$$

Key property: **symplectic** — preserves the symplectic structure of Hamiltonian mechanics, meaning energy oscillates around the correct value rather than drifting.

### 9.2 Velocity Verlet (for future implementation)

$$\mathbf{x}^{n+1} = \mathbf{x}^n + \mathbf{v}^n \Delta t + \frac{1}{2}\mathbf{a}^n \Delta t^2$$
$$\mathbf{v}^{n+1} = \mathbf{v}^n + \frac{\mathbf{a}^n + \mathbf{a}^{n+1}}{2} \Delta t$$

Second-order accurate and time-reversible.

---

## 10. Stability Analysis — CFL Condition

The Courant-Friedrichs-Lewy condition ensures information doesn't propagate more than one cell per timestep:

$$\Delta t \leq \lambda \cdot \min\left(\frac{h}{v_{max}}, \sqrt{\frac{h}{a_{max}}}\right)$$

where $\lambda \in [0.1, 0.4]$ is a safety factor.

For WCSPH, the speed of sound imposes an additional constraint:
$$\Delta t \leq \frac{h}{c_s}$$

---

## 11. Spatial Hashing for Neighbor Search

The naive $O(n^2)$ neighbor search dominates computation. Using spatial hashing (Teschner et al., 2003):

1. Hash each particle's cell: $\text{hash}(x, y) = (x \cdot p_1) \oplus (y \cdot p_2)$ where $p_1 = 73856093$ and $p_2 = 19349663$.
2. For each particle, query only the 9 surrounding cells.
3. Complexity: $O(n \cdot k)$ where $k \approx 20-40$ neighbors.

---

## References

1. Müller, M., Charypar, D., & Gross, M. (2003). *Particle-based fluid simulation for interactive applications.* SCA.
2. Monaghan, J. J. (1992). *Smoothed Particle Hydrodynamics.* Annual Review of Astronomy and Astrophysics.
3. Brackbill, J. U., Kothe, D. B., & Zemach, C. (1992). *A continuum method for modeling surface tension.* JCP.
4. Teschner, M. et al. (2003). *Optimized spatial hashing for collision detection of deformable objects.* VMV.
5. Becker, M., & Teschner, M. (2007). *Weakly compressible SPH for free surface flows.* SCA.
6. Solenthaler, B., & Pajarola, R. (2009). *Predictive-Corrective Incompressible SPH.* ACM SIGGRAPH.
7. Bender, J., & Koschier, D. (2015). *Divergence-free Smoothed Particle Hydrodynamics.* SCA.
