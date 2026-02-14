#version 430 core

// Metaball fragment shader
// Renders each particle as a Gaussian splat for screen-space fluid rendering
// Used in the first pass: accumulate density into an FBO

out vec4 FragColor;

void main() {
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(coord, coord);
    
    if (r2 > 1.0) discard;
    
    // Gaussian falloff: e^(-k * r²)
    // k = 3.0 gives a nice smooth falloff
    float strength = exp(-r2 * 3.0);
    
    // Output accumulated "density" for compositing pass
    FragColor = vec4(strength, strength * 0.8, strength * 0.5, 1.0);
}
