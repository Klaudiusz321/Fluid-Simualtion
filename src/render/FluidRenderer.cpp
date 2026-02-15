#include "../../include/render/FluidRenderer.hpp"

#include <glad/gl.h>
#include <iostream>
#include <vector>

// ============================================================
// Inline shader sources
// ============================================================

static const char* pointVertSrc = R"(
#version 430 core
layout(location = 0) in vec2 aPos;

uniform mat4 uProjection;
uniform float uPointSize;

out float vDepth;

void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    gl_PointSize = uPointSize;
    vDepth = gl_Position.z;
}
)";

static const char* pointFragSrc = R"(
#version 430 core
in float vDepth;
out vec4 FragColor;

uniform vec4 uColor;

void main() {
    // Make points circular
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(coord, coord);
    if (r2 > 1.0) discard;

    // Smooth edge
    float alpha = 1.0 - smoothstep(0.6, 1.0, r2);
    FragColor = vec4(uColor.rgb, uColor.a * alpha);
}
)";

static const char* metaballVertSrc = R"(
#version 430 core
layout(location = 0) in vec2 aPos;

uniform mat4 uProjection;
uniform float uPointSize;

void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    gl_PointSize = uPointSize;
}
)";

static const char* metaballFragSrc = R"(
#version 430 core
out vec4 FragColor;

void main() {
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(coord, coord);
    if (r2 > 1.0) discard;

    // Gaussian-like falloff for metaball blending
    float strength = exp(-r2 * 3.0);
    FragColor = vec4(strength, strength, strength, 1.0);
}
)";

// Fullscreen quad for post-processing
static const char* quadVertSrc = R"(
#version 430 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

out vec2 vUV;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vUV = aUV;
}
)";

static const char* compositeFragSrc = R"(
#version 430 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uMetaballTex;
uniform vec4 uFluidColor;

void main() {
    float density = texture(uMetaballTex, vUV).r;
    
    // Threshold — create a sharp surface
    float surface = smoothstep(0.3, 0.6, density);
    
    if (surface < 0.01) discard;
    
    // Simple lighting based on density gradient
    float highlight = smoothstep(0.5, 0.9, density) * 0.3;
    vec3 color = uFluidColor.rgb + vec3(highlight);
    
    // Fresnel-like edge darkening
    float edge = smoothstep(0.3, 0.5, density);
    color *= edge;
    
    FragColor = vec4(color, surface * uFluidColor.a);
}
)";

// ============================================================
// Implementation
// ============================================================

FluidRenderer::FluidRenderer() {}

FluidRenderer::~FluidRenderer() {
    cleanup();
}

bool FluidRenderer::init(int screenWidth, int screenHeight) {
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;

    // Compile shaders
    pointShader_ = compileShader(pointVertSrc, pointFragSrc);
    metaballShader_ = compileShader(metaballVertSrc, metaballFragSrc);
    compositeShader_ = compileShader(quadVertSrc, compositeFragSrc);

    if (!pointShader_ || !metaballShader_ || !compositeShader_) {
        std::cerr << "[FluidRenderer] Shader compilation failed\n";
        return false;
    }

    // Create VAO/VBO for particle positions
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Initialize screen quad and framebuffers
    initScreenQuad();
    initFramebuffers(screenWidth, screenHeight);

    std::cout << "[FluidRenderer] Initialized successfully\n";
    return true;
}

void FluidRenderer::updateParticles(const std::vector<Particle>& particles) {
    particleCount_ = particles.size();
    if (particleCount_ == 0) return;

    // Extract positions into a flat array
    std::vector<float> positions(particleCount_ * 2);
    for (size_t i = 0; i < particleCount_; ++i) {
        positions[i * 2 + 0] = particles[i].position.x;
        positions[i * 2 + 1] = particles[i].position.y;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, 
                 static_cast<GLsizeiptr>(positions.size() * sizeof(float)),
                 positions.data(), GL_DYNAMIC_DRAW);
}

void FluidRenderer::renderPoints(float pointSize) {
    if (particleCount_ == 0) return;

    glUseProgram(pointShader_);

    // Orthographic projection (2D)
    // Maps [0, width] x [0, height] to [-1, 1] x [-1, 1] (y-flipped for screen coords)
    float proj[16] = {0};
    float w = static_cast<float>(screenWidth_);
    float h = static_cast<float>(screenHeight_);
    proj[0]  =  2.0f / w;
    proj[5]  = -2.0f / h;   // Flip Y
    proj[10] = -1.0f;
    proj[12] = -1.0f;
    proj[13] =  1.0f;
    proj[15] =  1.0f;

    glUniformMatrix4fv(glGetUniformLocation(pointShader_, "uProjection"), 1, GL_FALSE, proj);
    glUniform1f(glGetUniformLocation(pointShader_, "uPointSize"), pointSize);
    glUniform4fv(glGetUniformLocation(pointShader_, "uColor"), 1, fluidColor_);

    glBindVertexArray(vao_);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(particleCount_));
    glBindVertexArray(0);
    glUseProgram(0);
}

void FluidRenderer::renderFluidSurface() {
    if (particleCount_ == 0) return;

    float proj[16] = {0};
    float w = static_cast<float>(screenWidth_);
    float h = static_cast<float>(screenHeight_);
    proj[0]  =  2.0f / w;
    proj[5]  = -2.0f / h;
    proj[10] = -1.0f;
    proj[12] = -1.0f;
    proj[13] =  1.0f;
    proj[15] =  1.0f;

    // Pass 1: Render metaballs to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);  // Additive blending for metaballs

    glUseProgram(metaballShader_);
    glUniformMatrix4fv(glGetUniformLocation(metaballShader_, "uProjection"), 1, GL_FALSE, proj);
    glUniform1f(glGetUniformLocation(metaballShader_, "uPointSize"), 32.0f);

    glBindVertexArray(vao_);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(particleCount_));
    glBindVertexArray(0);

    // Pass 2: Composite to screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(compositeShader_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexture_);
    glUniform1i(glGetUniformLocation(compositeShader_, "uMetaballTex"), 0);
    glUniform4fv(glGetUniformLocation(compositeShader_, "uFluidColor"), 1, fluidColor_);

    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glUseProgram(0);
}

void FluidRenderer::cleanup() {
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (quadVAO_) glDeleteVertexArrays(1, &quadVAO_);
    if (quadVBO_) glDeleteBuffers(1, &quadVBO_);
    if (fbo_) glDeleteFramebuffers(1, &fbo_);
    if (depthTexture_) glDeleteTextures(1, &depthTexture_);
    if (pointShader_) glDeleteProgram(pointShader_);
    if (metaballShader_) glDeleteProgram(metaballShader_);
    if (compositeShader_) glDeleteProgram(compositeShader_);
}

void FluidRenderer::setFluidColor(float r, float g, float b, float a) {
    fluidColor_[0] = r;
    fluidColor_[1] = g;
    fluidColor_[2] = b;
    fluidColor_[3] = a;
}

uint32_t FluidRenderer::compileShader(const char* vertSrc, const char* fragSrc) {
    // Vertex shader
    uint32_t vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertSrc, nullptr);
    glCompileShader(vert);

    int success;
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(vert, 512, nullptr, log);
        std::cerr << "[Shader] Vertex compile error:\n" << log << "\n";
        return 0;
    }

    // Fragment shader
    uint32_t frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragSrc, nullptr);
    glCompileShader(frag);

    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(frag, 512, nullptr, log);
        std::cerr << "[Shader] Fragment compile error:\n" << log << "\n";
        return 0;
    }

    // Link program
    uint32_t program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "[Shader] Link error:\n" << log << "\n";
        return 0;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

void FluidRenderer::initScreenQuad() {
    // Fullscreen quad (position + UV)
    float quadVerts[] = {
        // pos      // uv
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
    };

    glGenVertexArrays(1, &quadVAO_);
    glGenBuffers(1, &quadVBO_);

    glBindVertexArray(quadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void FluidRenderer::initFramebuffers(int w, int h) {
    // Create FBO with a color texture for metaball accumulation
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    glGenTextures(1, &depthTexture_);
    glBindTexture(GL_TEXTURE_2D, depthTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, depthTexture_, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[FluidRenderer] Framebuffer incomplete!\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
