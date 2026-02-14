#version 430 core

// Particle vertex shader
// Transforms 2D particle positions into screen space with point size

layout(location = 0) in vec2 aPos;
layout(location = 1) in float aDensity;  // Optional: for color mapping
layout(location = 2) in float aVelocity; // Optional: velocity magnitude

uniform mat4 uProjection;
uniform float uPointSize;

out float vDensity;
out float vVelocity;

void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    gl_PointSize = uPointSize;
    vDensity = aDensity;
    vVelocity = aVelocity;
}
