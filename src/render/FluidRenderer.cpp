#include "../../include/render/FluidRenderer.hpp"

#include <glad/gl.h>
#include <iostream>
#include <vector>

// ============================================================
// PASS 1: Gaussian splat — each particle writes a smooth blob
// ============================================================

static const char* splatVertSrc = R"(
#version 430 core
layout(location = 0) in vec2 aPos;

uniform mat4 uProjection;
uniform float uPointSize;

void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    gl_PointSize = uPointSize;
}
)";

static const char* splatFragSrc = R"(
#version 430 core
out vec4 FragColor;

void main() {
    vec2 c = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(c, c);
    if (r2 > 1.0) discard;

    // Smooth Gaussian energy distribution
    float energy = exp(-r2 * 4.0) * 0.8;
    FragColor = vec4(energy, energy, energy, 1.0);
}
)";

// ============================================================
// PASS 2: Separable Gaussian blur (two passes: H then V)
// ============================================================

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

static const char* blurFragSrc = R"(
#version 430 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTex;
uniform vec2 uDirection;  // (1/w, 0) for horizontal, (0, 1/h) for vertical

void main() {
    // 9-tap Gaussian blur with sigma ~ 4.0
    float weights[5] = float[](0.227027, 0.194596, 0.121621, 0.054054, 0.016216);
    
    vec3 result = texture(uTex, vUV).rgb * weights[0];
    
    for (int i = 1; i < 5; ++i) {
        vec2 offset = uDirection * float(i) * 1.5;
        result += texture(uTex, vUV + offset).rgb * weights[i];
        result += texture(uTex, vUV - offset).rgb * weights[i];
    }
    
    FragColor = vec4(result, 1.0);
}
)";

// ============================================================
// PASS 3: Composite — ShaderToy-quality fluid surface
// ============================================================

static const char* compositeFragSrc = R"(
#version 430 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uDensityTex;
uniform vec4 uFluidColor;
uniform float uTime;
uniform vec2 uResolution;

// --- Noise for caustics ---
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);  // smoothstep
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; ++i) {
        v += a * noise(p);
        p *= 2.0;
        a *= 0.5;
    }
    return v;
}

void main() {
    float density = texture(uDensityTex, vUV).r;
    
    // Surface extraction with smooth threshold
    float surface = smoothstep(0.08, 0.25, density);
    
    if (surface < 0.001) {
        // Background — subtle gradient
        float bgGrad = 1.0 - vUV.y * 0.3;
        FragColor = vec4(vec3(0.02, 0.02, 0.04) * bgGrad, 1.0);
        return;
    }
    
    // --- Compute normals from density gradient (Sobel) ---
    vec2 texel = 1.0 / uResolution;
    float dL = texture(uDensityTex, vUV - vec2(texel.x, 0.0)).r;
    float dR = texture(uDensityTex, vUV + vec2(texel.x, 0.0)).r;
    float dU = texture(uDensityTex, vUV - vec2(0.0, texel.y)).r;
    float dD = texture(uDensityTex, vUV + vec2(0.0, texel.y)).r;
    
    vec2 gradient = vec2(dR - dL, dD - dU);
    float gradLen = length(gradient);
    vec3 normal = normalize(vec3(-gradient * 3.0, 1.0));
    
    // --- Depth (thicker fluid = darker) ---
    float depth = clamp(density * 1.5, 0.0, 1.0);
    
    // --- Base water color with depth variation ---
    vec3 shallowColor = vec3(0.15, 0.55, 0.95);
    vec3 deepColor = vec3(0.02, 0.12, 0.35);
    vec3 waterColor = mix(shallowColor, deepColor, depth * 0.8);
    
    // Override with user color, blended
    waterColor = mix(waterColor, uFluidColor.rgb, 0.3);
    
    // --- Lighting ---
    vec3 lightDir = normalize(vec3(0.4, -0.6, 0.7));
    vec3 viewDir = vec3(0.0, 0.0, 1.0);
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 64.0);
    
    // Fresnel (Schlick approximation)
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 3.0);
    
    // --- Caustics ---
    vec2 causticUV = vUV * 8.0 + vec2(uTime * 0.3, uTime * 0.2);
    float caustic1 = fbm(causticUV);
    float caustic2 = fbm(causticUV * 1.3 + vec2(1.7, 3.2));
    float caustics = pow(abs(caustic1 - caustic2), 1.5) * surface * depth * 0.6;
    
    // --- Refraction offset (distort background) ---
    vec2 refractOffset = gradient * texel * 15.0;
    
    // --- Edge highlighting (foam-like) ---
    float edge = smoothstep(0.08, 0.15, density) - smoothstep(0.15, 0.35, density);
    vec3 edgeColor = vec3(0.7, 0.85, 1.0);
    
    // --- Compose final color ---
    vec3 color = waterColor;
    
    // Apply lighting
    color *= 0.25 + 0.5 * diff;                  // Ambient + diffuse
    color += vec3(1.0, 0.95, 0.9) * spec * 0.8;  // Specular highlights
    color += vec3(0.3, 0.5, 0.7) * fresnel * 0.3; // Fresnel rim
    color += vec3(0.4, 0.7, 1.0) * caustics;      // Caustics
    color += edgeColor * edge * 0.5;               // Foam edges
    
    // Subtle internal glow in deep areas
    color += deepColor * depth * 0.15;
    
    // Tone mapping (prevent blow-out)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    float alpha = surface * uFluidColor.a;
    FragColor = vec4(color, alpha);
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
    splatShader_ = compileShader(splatVertSrc, splatFragSrc);
    blurShaderH_ = compileShader(quadVertSrc, blurFragSrc);
    blurShaderV_ = compileShader(quadVertSrc, blurFragSrc);
    compositeShader_ = compileShader(quadVertSrc, compositeFragSrc);

    if (!splatShader_ || !blurShaderH_ || !compositeShader_) {
        std::cerr << "[FluidRenderer] Shader compilation failed\n";
        return false;
    }

    // Particle VAO/VBO
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Screen quad
    initScreenQuad();

    // FBOs: splat + 2 ping-pong for blur
    createFBO(splatFBO_, splatTex_, screenWidth, screenHeight);
    createFBO(blurFBO_[0], blurTex_[0], screenWidth, screenHeight);
    createFBO(blurFBO_[1], blurTex_[1], screenWidth, screenHeight);

    std::cout << "[FluidRenderer] Initialized (3-pass pipeline)\n";
    return true;
}

void FluidRenderer::updateParticles(const std::vector<Particle>& particles) {
    particleCount_ = particles.size();
    if (particleCount_ == 0) return;

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

void FluidRenderer::render(float time) {
    if (particleCount_ == 0) return;

    float w = static_cast<float>(screenWidth_);
    float h = static_cast<float>(screenHeight_);

    // ==== PASS 1: Splat particles to density FBO ====
    glBindFramebuffer(GL_FRAMEBUFFER, splatFBO_);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);  // Additive blending

    glUseProgram(splatShader_);
    setOrthoProjection(splatShader_, w, h);
    glUniform1f(glGetUniformLocation(splatShader_, "uPointSize"), 24.0f);

    glBindVertexArray(vao_);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(particleCount_));
    glBindVertexArray(0);

    // ==== PASS 2: Gaussian blur (3 iterations of H+V for very smooth result) ====
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);

    // Use the same blur shader, just change direction uniform
    uint32_t currentTex = splatTex_;

    int blurPasses = 3;
    for (int pass = 0; pass < blurPasses; ++pass) {
        // Horizontal blur: currentTex → blurTex_[0]
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO_[0]);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(blurShaderH_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTex);
        glUniform1i(glGetUniformLocation(blurShaderH_, "uTex"), 0);
        glUniform2f(glGetUniformLocation(blurShaderH_, "uDirection"), 1.0f / w, 0.0f);
        glBindVertexArray(quadVAO_);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // Vertical blur: blurTex_[0] → blurTex_[1]
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO_[1]);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, blurTex_[0]);
        glUniform2f(glGetUniformLocation(blurShaderH_, "uDirection"), 0.0f, 1.0f / h);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        currentTex = blurTex_[1];  // Next iteration reads from this
    }

    // ==== PASS 3: Composite to screen ====
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(compositeShader_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blurTex_[1]);
    glUniform1i(glGetUniformLocation(compositeShader_, "uDensityTex"), 0);
    glUniform4fv(glGetUniformLocation(compositeShader_, "uFluidColor"), 1, fluidColor_);
    glUniform1f(glGetUniformLocation(compositeShader_, "uTime"), time);
    glUniform2f(glGetUniformLocation(compositeShader_, "uResolution"), w, h);

    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);
}

void FluidRenderer::cleanup() {
    auto del = [](uint32_t& id, auto fn) { if (id) { fn(1, &id); id = 0; } };
    auto delProg = [](uint32_t& id) { if (id) { glDeleteProgram(id); id = 0; } };

    del(vao_, glDeleteVertexArrays);
    del(vbo_, glDeleteBuffers);
    del(quadVAO_, glDeleteVertexArrays);
    del(quadVBO_, glDeleteBuffers);
    del(splatFBO_, glDeleteFramebuffers);
    del(splatTex_, glDeleteTextures);
    for (int i = 0; i < 2; ++i) {
        del(blurFBO_[i], glDeleteFramebuffers);
        del(blurTex_[i], glDeleteTextures);
    }
    delProg(splatShader_);
    delProg(blurShaderH_);
    delProg(blurShaderV_);
    delProg(compositeShader_);
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

void FluidRenderer::initScreenQuad() {
    float quadVerts[] = {
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

void FluidRenderer::createFBO(uint32_t& fbo, uint32_t& tex, int w, int h) {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[FluidRenderer] FBO incomplete!\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
