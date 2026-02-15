#pragma once

#include "../core/Particle.hpp"
#include "Renderer.hpp"
#include <vector>
#include <cstdint>

/// Renders SPH fluid using a 3-pass screen-space technique:
///
///   Pass 1: Render particles as Gaussian splats → accumulate density to FBO
///   Pass 2: Bilateral/Gaussian blur to smooth the density field
///   Pass 3: Composite with lighting, refraction, caustics, depth coloring
///
/// This is the same approach used in AAA games and ShaderToy fluid demos.
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
    // Shader programs
    uint32_t splatShader_ = 0;       // Pass 1: Gaussian splat
    uint32_t blurShaderH_ = 0;       // Pass 2a: Horizontal blur
    uint32_t blurShaderV_ = 0;       // Pass 2b: Vertical blur
    uint32_t compositeShader_ = 0;   // Pass 3: Final composite

    // Particle VBO
    uint32_t vao_ = 0;
    uint32_t vbo_ = 0;
    size_t particleCount_ = 0;

    // FBOs and textures
    uint32_t splatFBO_ = 0;
    uint32_t splatTex_ = 0;

    uint32_t blurFBO_[2] = {};       // Ping-pong blur
    uint32_t blurTex_[2] = {};

    // Screen quad
    uint32_t quadVAO_ = 0;
    uint32_t quadVBO_ = 0;

    int screenWidth_ = 0;
    int screenHeight_ = 0;
    float fluidColor_[4] = { 0.1f, 0.4f, 0.9f, 0.9f };

    uint32_t compileShader(const char* vertSrc, const char* fragSrc);
    void initScreenQuad();
    void createFBO(uint32_t& fbo, uint32_t& tex, int w, int h);
    void setOrthoProjection(uint32_t shader, float w, float h);
};
