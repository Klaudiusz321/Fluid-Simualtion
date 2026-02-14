#version 430 core

// Particle fragment shader
// Renders circular point sprites with velocity-dependent coloring

in float vDensity;
in float vVelocity;

out vec4 FragColor;

uniform vec4 uBaseColor;
uniform float uMaxVelocity;

// Smooth circular point sprite
void main() {
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(coord, coord);
    
    // Discard outside circle
    if (r2 > 1.0) discard;
    
    // Smooth edge antialiasing
    float alpha = 1.0 - smoothstep(0.7, 1.0, r2);
    
    // Velocity-based color (blue = slow, white = fast)
    float t = clamp(vVelocity / max(uMaxVelocity, 0.001), 0.0, 1.0);
    vec3 slowColor = uBaseColor.rgb;
    vec3 fastColor = vec3(0.8, 0.9, 1.0);
    vec3 color = mix(slowColor, fastColor, t);
    
    // Slight fresnel-like rim lighting
    float rim = 1.0 - sqrt(r2);
    color += vec3(0.1) * rim;
    
    FragColor = vec4(color, uBaseColor.a * alpha);
}
