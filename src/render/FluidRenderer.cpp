#include "../../include/render/FluidRenderer.hpp"

#include <glad/gl.h>
#include <iostream>
#include <vector>
#include <cmath>

// ============================================================
// Vertex shader — positions particles as GL_POINTS,
// passes speed to fragment shader for velocity coloring
// ============================================================

static const char* sphereVertSrc = R"(
#version 430 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in float aSpeed;

uniform mat4 uProjection;
uniform float uPointSize;

out float vSpeed;

void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    gl_PointSize = uPointSize;
    vSpeed = aSpeed;
}
)";

// ============================================================
// Fragment shader — 3D-shaded sphere per particle
//   - Computes sphere normal from gl_PointCoord
//   - Phong lighting (ambient + diffuse + specular)
//   - Velocity-based color gradient (slow=deep blue, fast=cyan/white)
//   - Soft shadow at bottom of sphere
//   - Fresnel rim highlight
// ============================================================

static const char* sphereFragSrc = R"(
#version 430 core
in float vSpeed;
out vec4 FragColor;

uniform vec4 uBaseColor;

void main() {
    // Map point coord to [-1, 1]
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(coord, coord);

    // Discard outside circle → makes it look like a sphere
    if (r2 > 1.0) discard;

    // Reconstruct sphere normal (z points toward camera)
    vec3 normal = vec3(coord, sqrt(1.0 - r2));

    // --- Lighting setup ---
    vec3 lightDir = normalize(vec3(0.5, -0.7, 0.8));
    vec3 viewDir  = vec3(0.0, 0.0, 1.0);

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);

    // Specular (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 80.0);

    // Fresnel rim (light edge)
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 3.0);

    // --- Velocity-based color ---
    // speed: 0 = still (deep blue), ~150+ = fast (cyan/white)
    // Typical sim velocities: 0-200 px/s, so divide by 150 for full range
    float t = clamp(vSpeed / 150.0, 0.0, 1.0);

    vec3 slowColor = uBaseColor.rgb;                      // Deep blue
    vec3 midColor  = vec3(0.1, 0.75, 0.95);              // Teal / cyan
    vec3 fastColor = vec3(0.85, 0.95, 1.0);              // Near-white

    vec3 baseColor;
    if (t < 0.5) {
        baseColor = mix(slowColor, midColor, t * 2.0);
    } else {
        baseColor = mix(midColor, fastColor, (t - 0.5) * 2.0);
    }

    // --- Compose ---
    float ambient = 0.15;
    vec3 color = baseColor * (ambient + diff * 0.7);
    color += vec3(1.0) * spec * 0.6;                      // White specular
    color += vec3(0.3, 0.5, 0.8) * fresnel * 0.25;       // Blue rim

    // Soft contact shadow at bottom of sphere
    float shadow = smoothstep(-1.0, -0.3, coord.y);
    color *= (0.7 + 0.3 * shadow);

    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
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

    shader_ = compileShader(sphereVertSrc, sphereFragSrc);
    if (!shader_) {
        std::cerr << "[FluidRenderer] Shader compilation failed\n";
        return false;
    }

    // VAO/VBO — interleaved: [x, y, speed] per particle
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // position (vec2)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // speed (float)
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    std::cout << "[FluidRenderer] Initialized (3D sphere style)\n";
    return true;
}

void FluidRenderer::updateParticles(const std::vector<Particle>& particles) {
    particleCount_ = particles.size();
    if (particleCount_ == 0) return;

    // Interleaved buffer: [x, y, speed, x, y, speed, ...]
    std::vector<float> data(particleCount_ * 3);
    for (size_t i = 0; i < particleCount_; ++i) {
        data[i * 3 + 0] = particles[i].position.x;
        data[i * 3 + 1] = particles[i].position.y;
        data[i * 3 + 2] = particles[i].velocity.length();
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(data.size() * sizeof(float)),
                 data.data(), GL_DYNAMIC_DRAW);
}

void FluidRenderer::render(float /*time*/) {
    if (particleCount_ == 0) return;

    float w = static_cast<float>(screenWidth_);
    float h = static_cast<float>(screenHeight_);

    // Dark background
    glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Enable point sprites and depth-like sorting
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(shader_);
    setOrthoProjection(shader_, w, h);
    glUniform1f(glGetUniformLocation(shader_, "uPointSize"), 28.0f);
    glUniform4fv(glGetUniformLocation(shader_, "uBaseColor"), 1, fluidColor_);

    glBindVertexArray(vao_);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(particleCount_));
    glBindVertexArray(0);
    glUseProgram(0);
}

void FluidRenderer::cleanup() {
    if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
    if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (shader_) { glDeleteProgram(shader_); shader_ = 0; }
}

void FluidRenderer::setFluidColor(float r, float g, float b, float a) {
    fluidColor_[0] = r;
    fluidColor_[1] = g;
    fluidColor_[2] = b;
    fluidColor_[3] = a;
}

uint32_t FluidRenderer::compileShader(const char* vertSrc, const char* fragSrc) {
    uint32_t vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertSrc, nullptr);
    glCompileShader(vert);

    int success;
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(vert, 1024, nullptr, log);
        std::cerr << "[Shader] Vertex error:\n" << log << "\n";
        return 0;
    }

    uint32_t frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragSrc, nullptr);
    glCompileShader(frag);

    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(frag, 1024, nullptr, log);
        std::cerr << "[Shader] Fragment error:\n" << log << "\n";
        return 0;
    }

    uint32_t program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "[Shader] Link error:\n" << log << "\n";
        return 0;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

void FluidRenderer::setOrthoProjection(uint32_t shader, float w, float h) {
    float proj[16] = {0};
    proj[0]  =  2.0f / w;
    proj[5]  = -2.0f / h;
    proj[10] = -1.0f;
    proj[12] = -1.0f;
    proj[13] =  1.0f;
    proj[15] =  1.0f;
    glUniformMatrix4fv(glGetUniformLocation(shader, "uProjection"), 1, GL_FALSE, proj);
}
