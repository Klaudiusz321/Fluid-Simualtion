#include "../../include/core/SPHSolver.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

SPHSolver::SPHSolver()
    : spatialHash_(SimConfig::H, 8192)
    , nextId_(0)
{
    // Precompute kernel coefficients (done once, not per-frame)
    poly6Coeff_     = SPHKernels::poly6Coeff(SimConfig::H);
    spikyGradCoeff_ = SPHKernels::spikyGradCoeff(SimConfig::H);
    viscLapCoeff_   = SPHKernels::viscLapCoeff(SimConfig::H);
    wendlandCoeff_  = SPHKernels::wendlandC2Coeff(SimConfig::H);
}

// ============================================================
// Initialization
// ============================================================

void SPHSolver::initDamBreak(int rows, int cols) {
    particles_.clear();
    particles_.reserve(rows * cols);

    float spacing = SimConfig::PARTICLE_SPACING;
    float startX = SimConfig::WINDOW_WIDTH * 0.1f;
    float startY = SimConfig::WINDOW_HEIGHT * 0.3f;

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            // Small jitter to break symmetry
            float jitterX = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * spacing * 0.1f;
            float jitterY = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * spacing * 0.1f;

            Particle p(startX + j * spacing + jitterX,
                       startY + i * spacing + jitterY,
                       nextId_++);
            p.mass = SimConfig::PARTICLE_MASS;
            particles_.push_back(p);
        }
    }

    std::cout << "[SPH] Dam break initialized: " << particles_.size() << " particles\n";
}

void SPHSolver::initDroplet(float cx, float cy, float radius) {
    particles_.clear();
    float spacing = SimConfig::PARTICLE_SPACING;

    for (float y = cy - radius; y <= cy + radius; y += spacing) {
        for (float x = cx - radius; x <= cx + radius; x += spacing) {
            float dx = x - cx;
            float dy = y - cy;
            if (dx * dx + dy * dy <= radius * radius) {
                Particle p(x, y, nextId_++);
                p.mass = SimConfig::PARTICLE_MASS;
                particles_.push_back(p);
            }
        }
    }

    std::cout << "[SPH] Droplet initialized: " << particles_.size() << " particles\n";
}

void SPHSolver::initPool(float fillFraction) {
    particles_.clear();
    float spacing = SimConfig::PARTICLE_SPACING;

    float poolTop = SimConfig::WINDOW_HEIGHT * (1.0f - fillFraction);
    float margin = SimConfig::H;

    for (float y = poolTop; y < SimConfig::WINDOW_HEIGHT - margin; y += spacing) {
        for (float x = margin; x < SimConfig::WINDOW_WIDTH - margin; x += spacing) {
            float jx = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * spacing * 0.05f;
            float jy = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * spacing * 0.05f;

            Particle p(x + jx, y + jy, nextId_++);
            p.mass = SimConfig::PARTICLE_MASS;
            particles_.push_back(p);
        }
    }

    std::cout << "[SPH] Pool initialized: " << particles_.size() << " particles\n";
}

void SPHSolver::initTallDam() {
    particles_.clear();
    float spacing = SimConfig::PARTICLE_SPACING;
    float margin = SimConfig::H;

    // Tall column on the left (40% width, 85% height)
    float colWidth = SimConfig::WINDOW_WIDTH * 0.35f;
    float colTop = SimConfig::WINDOW_HEIGHT * 0.1f;

    for (float y = colTop; y < SimConfig::WINDOW_HEIGHT - margin; y += spacing) {
        for (float x = margin; x < colWidth; x += spacing) {
            float jx = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * spacing * 0.05f;
            float jy = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * spacing * 0.05f;

            Particle p(x + jx, y + jy, nextId_++);
            p.mass = SimConfig::PARTICLE_MASS;
            particles_.push_back(p);
        }
    }

    std::cout << "[SPH] Tall dam initialized: " << particles_.size() << " particles\n";
}

void SPHSolver::addParticle(float x, float y) {
    Particle p(x, y, nextId_++);
    p.mass = SimConfig::PARTICLE_MASS;
    particles_.push_back(p);
}

// ============================================================
// Spatial Hash
// ============================================================

void SPHSolver::buildSpatialHash() {
    // Extract positions into contiguous array for compact grid build
    positions_.resize(particles_.size());
    for (size_t i = 0; i < particles_.size(); ++i) {
        positions_[i] = particles_[i].position;
    }
    spatialHash_.build(positions_.data(), static_cast<int>(particles_.size()));
}

// ============================================================
// Density & Pressure (Tait Equation of State)
// ============================================================

void SPHSolver::computeDensityPressure() {
    #pragma omp parallel for
    for (int i = 0; i < static_cast<int>(particles_.size()); ++i) {
        auto& pi = particles_[i];
        pi.density = 0.0f;

        const auto& neighbors = neighbourList_[i];
        for (int j : neighbors) {
            // NOTE: do NOT skip j==i — self-contribution is essential in SPH!
            const auto& pj = particles_[j];
            Vec2 rij = pj.position - pi.position;
            float r2 = rij.lengthSq();

            if (r2 < SimConfig::HSQ) {
                pi.density += pj.mass * SPHKernels::poly6(r2, SimConfig::H, poly6Coeff_);
            }
        }

        // Clamp minimum density to avoid division by zero
        pi.density = std::max(pi.density, SimConfig::REST_DENSITY * 0.01f);

        // Tait equation of state: p = k * (ρ - ρ₀)
        // Clamp to non-negative to prevent tensile instability
        // (negative pressure creates artificial attraction → particle clumping)
        pi.pressure = std::max(0.0f, SimConfig::GAS_CONSTANT * (pi.density - SimConfig::REST_DENSITY));
    }
}

// ============================================================
// Forces: Pressure + Viscosity
// ============================================================

void SPHSolver::computeForces() {
    #pragma omp parallel for
    for (int i = 0; i < static_cast<int>(particles_.size()); ++i) {
        auto& pi = particles_[i];
        Vec2 fPressure(0.0f, 0.0f);
        Vec2 fViscosity(0.0f, 0.0f);

        const auto& neighbors = neighbourList_[i];

        for (int jIdx : neighbors) {
            if (jIdx == i) continue;  // Skip self

            const auto& pj = particles_[jIdx];
            Vec2 rij = pj.position - pi.position;
            float r = rij.length();

            if (r < SimConfig::H && r > 1e-6f) {
                Vec2 rNorm = rij * (1.0f / r);

                // Pressure force (Spiky kernel gradient)
                // rij = r_j - r_i, so rNorm points from i toward j.
                // ∇_i W = W'(r) * (r_i - r_j)/r = W'(r) * (-rNorm)
                // F_p = -m_j (p_i+p_j)/(2ρ_j) * ∇_i W
                //      = -m_j (p_i+p_j)/(2ρ_j) * W'(r) * (-rNorm)
                //      = rNorm * [m_j (p_i+p_j)/(2ρ_j) * W'(r)]
                float pressureMag = pj.mass *
                    (pi.pressure + pj.pressure) / (2.0f * pj.density) *
                    SPHKernels::spikyGrad(r, SimConfig::H, spikyGradCoeff_);
                fPressure += rNorm * pressureMag;

                // Viscosity force (Viscosity kernel Laplacian)
                // F_v = μ * m_j * (v_j - v_i) / ρ_j * ∇²W_visc
                Vec2 velDiff = pj.velocity - pi.velocity;
                float viscMag = SimConfig::VISCOSITY * pj.mass / pj.density *
                    SPHKernels::viscLaplacian(r, SimConfig::H, viscLapCoeff_);
                fViscosity += velDiff * viscMag;
            }
        }

        // Gravity (downward in screen coordinates: +y is down)
        Vec2 fGravity(0.0f, SimConfig::GRAVITY * pi.density);

        // Sum all forces
        pi.force = fPressure + fViscosity + fGravity;
    }
}

// ============================================================
// Surface Tension (CSF model)
// ============================================================

void SPHSolver::computeSurfaceTension() {
    // Step 1: Compute color field, gradient, and laplacian
    #pragma omp parallel for
    for (int i = 0; i < static_cast<int>(particles_.size()); ++i) {
        auto& pi = particles_[i];
        pi.colorField = 0.0f;
        pi.colorGradient = Vec2(0.0f, 0.0f);
        pi.colorLaplacian = 0.0f;

        const auto& neighbors = neighbourList_[i];

        for (int jIdx : neighbors) {
            const auto& pj = particles_[jIdx];
            Vec2 rij = pj.position - pi.position;
            float r2 = rij.lengthSq();

            if (r2 < SimConfig::HSQ) {
                float massOverDensity = pj.mass / pj.density;

                // Color field
                pi.colorField += massOverDensity * SPHKernels::poly6(r2, SimConfig::H, poly6Coeff_);

                // Gradient of color field (using poly6 gradient)
                float gradMag = SPHKernels::poly6Grad(r2, SimConfig::H, poly6Coeff_);
                pi.colorGradient += rij * (massOverDensity * gradMag);

                // Laplacian of color field
                pi.colorLaplacian += massOverDensity * SPHKernels::poly6Laplacian(r2, SimConfig::H, poly6Coeff_);
            }
        }
    }

    // Step 2: Apply surface tension force where |∇c| is large enough
    float threshold = 6.0f;  // Only apply near the surface
    for (auto& pi : particles_) {
        float gradLen = pi.colorGradient.length();
        if (gradLen > threshold) {
            // κ = -∇²c / |∇c|
            float curvature = -pi.colorLaplacian / gradLen;

            // F_st = σ * κ * n̂
            Vec2 normal = pi.colorGradient.normalized();
            pi.force += normal * (SimConfig::SURFACE_TENSION * curvature * pi.density);
        }
    }
}

// ============================================================
// XSPH Velocity Correction
// ============================================================

void SPHSolver::applyXSPH() {
    // Compute corrections first, apply after (so we don't modify while iterating)
    std::vector<Vec2> corrections(particles_.size(), Vec2(0.0f, 0.0f));

    #pragma omp parallel for
    for (int i = 0; i < static_cast<int>(particles_.size()); ++i) {
        const auto& pi = particles_[i];
        const auto& neighbors = neighbourList_[i];

        for (int jIdx : neighbors) {
            if (jIdx == i) continue;

            const auto& pj = particles_[jIdx];
            Vec2 rij = pj.position - pi.position;
            float r2 = rij.lengthSq();

            if (r2 < SimConfig::HSQ) {
                float avgDensity = (pi.density + pj.density) * 0.5f;
                float w = SPHKernels::poly6(r2, SimConfig::H, poly6Coeff_);
                corrections[i] += (pj.velocity - pi.velocity) * (pj.mass / avgDensity * w);
            }
        }
    }

    // Apply XSPH correction
    for (size_t i = 0; i < particles_.size(); ++i) {
        particles_[i].velocity += corrections[i] * SimConfig::XSPH_EPSILON;
    }
}

// ============================================================
// Integration (Symplectic Euler)
// ============================================================

void SPHSolver::integrate() {
    float dt = SimConfig::DT;

    for (auto& p : particles_) {
        // a = F / ρ
        Vec2 acceleration = p.force / p.density;

        // Symplectic Euler (updates velocity first, then position)
        // This is energy-conserving unlike standard Euler
        p.velocity += acceleration * dt;
        p.position += p.velocity * dt;
    }
}

// ============================================================
// Boundary Enforcement
// ============================================================

void SPHSolver::enforceBoundaries() {
    float damping = SimConfig::BOUNDARY_DAMPING;
    float margin = SimConfig::H * 0.5f;

    for (auto& p : particles_) {
        // Left wall
        if (p.position.x < margin) {
            p.position.x = margin;
            p.velocity.x *= damping;
        }
        // Right wall
        if (p.position.x > SimConfig::WINDOW_WIDTH - margin) {
            p.position.x = SimConfig::WINDOW_WIDTH - margin;
            p.velocity.x *= damping;
        }
        // Bottom wall
        if (p.position.y > SimConfig::WINDOW_HEIGHT - margin) {
            p.position.y = SimConfig::WINDOW_HEIGHT - margin;
            p.velocity.y *= damping;
        }
        // Top wall
        if (p.position.y < margin) {
            p.position.y = margin;
            p.velocity.y *= damping;
        }
    }
}

// ============================================================
// Adaptive Timestep (CFL condition)
// ============================================================

float SPHSolver::computeAdaptiveDT() const {
    float maxVel = 0.0f;
    float maxAcc = 0.0f;

    for (const auto& p : particles_) {
        float v = p.velocity.length();
        float a = (p.density > 0.0f) ? (p.force / p.density).length() : 0.0f;
        maxVel = std::max(maxVel, v);
        maxAcc = std::max(maxAcc, a);
    }

    float dtVel = (maxVel > 1e-6f) ? (SimConfig::H / maxVel) : SimConfig::DT * 10.0f;
    float dtAcc = (maxAcc > 1e-6f) ? std::sqrt(SimConfig::H / maxAcc) : SimConfig::DT * 10.0f;

    float dt = SimConfig::CFL_FACTOR * std::min(dtVel, dtAcc);

    // Clamp to reasonable range
    return std::clamp(dt, SimConfig::DT * 0.1f, SimConfig::DT * 5.0f);
}

// ============================================================
// Main Update
// ============================================================

void SPHSolver::update() {
    buildSpatialHash();

    if (neighbourList_.size() != particles_.size()){
        neighbourList_.resize(particles_.size());
    }
    #pragma omp parallel for
    for(int i = 0; i < static_cast<int>(particles_.size()); ++i){
        neighbourList_[i].clear();
        spatialHash_.queryNeighbors(particles_[i].position, neighbourList_[i]);
    }

    computeDensityPressure();
    computeForces();
    // Surface tension disabled (SimConfig::SURFACE_TENSION == 0)
    if (SimConfig::SURFACE_TENSION > 0.0f) {
        computeSurfaceTension();
    }
    integrate();
    applyXSPH();
    enforceBoundaries();
}
