#pragma once

#include "../core/Particle.hpp"
#include "Renderer.hpp"
#include <vector>
#include <cstdint>

/// Renders SPH fluid particles using OpenGL.
/// Supports:
///   1. Point sprite rendering (fast, simple)
///   2. Screen-space fluid rendering (metaball-like, smooth surface)
class FluidRenderer {
public:
    FluidRenderer();
    ~FluidRenderer();

    /// Initialize shaders and buffers
    bool init(int screenWidth, int screenHeight);

    /// Upload particle positions to GPU
    void updateParticles(const std::vector<Particle>& particles);

    /// Render particles as colored point sprites
    void renderPoints(float pointSize = 8.0f);

    /// Render fluid using screen-space metaball technique:
    ///   Pass 1: render depth/thickness to FBO
    ///   Pass 2: blur
    ///   Pass 3: composite fluid surface
    void renderFluidSurface();

    /// Cleanup GPU resources
    void cleanup();

    /// Set color for the fluid
    void setFluidColor(float r, float g, float b, float a = 1.0f);

private:
    // Shader programs
    uint32_t pointShader_ = 0;
    uint32_t metaballShader_ = 0;
    uint32_t blurShader_ = 0;
    uint32_t compositeShader_ = 0;

    // Vertex buffer for particle positions
    uint32_t vao_ = 0;
    uint32_t vbo_ = 0;
    size_t particleCount_ = 0;

    // Framebuffer objects for screen-space rendering
    uint32_t fbo_ = 0;
    uint32_t depthTexture_ = 0;
    uint32_t thicknessTexture_ = 0;
    uint32_t blurTexture_ = 0;

    // Screen quad for fullscreen passes
    uint32_t quadVAO_ = 0;
    uint32_t quadVBO_ = 0;

    int screenWidth_ = 0;
    int screenHeight_ = 0;

    float fluidColor_[4] = { 0.2f, 0.5f, 0.9f, 0.8f };

    // Helpers
    uint32_t compileShader(const char* vertSrc, const char* fragSrc);
    void initScreenQuad();
    void initFramebuffers(int w, int h);
};
