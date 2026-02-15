#pragma once

#include "../core/Particle.hpp"
#include "Renderer.hpp"
#include <vector>
#include <cstdint>

/// Renders SPH particles as 3D-shaded spheres (Sebastian Lague style).
///
/// Each particle is a GL_POINT with a fragment shader that:
///   - Computes sphere normals from gl_PointCoord
///   - Applies Phong lighting (ambient + diffuse + specular)
///   - Colors based on velocity magnitude (slow=blue, fast=white)
///   - Discards fragments outside the sphere radius
///
/// Simple, clean, and shows the physics clearly.
class FluidRenderer {
public:
    FluidRenderer();
    ~FluidRenderer();

    bool init(int screenWidth, int screenHeight);
    void updateParticles(const std::vector<Particle>& particles);
    void render(float time);
    void cleanup();
    void setFluidColor(float r, float g, float b, float a = 1.0f);

private:
    uint32_t shader_ = 0;

    uint32_t vao_ = 0;
    uint32_t vbo_ = 0;         // interleaved: [pos.x, pos.y, speed]
    size_t particleCount_ = 0;

    int screenWidth_ = 0;
    int screenHeight_ = 0;
    float fluidColor_[4] = { 0.15f, 0.45f, 0.95f, 1.0f };

    uint32_t compileShader(const char* vertSrc, const char* fragSrc);
    void setOrthoProjection(uint32_t shader, float w, float h);
};
