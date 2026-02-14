#pragma once

#include "../math/Vec2.hpp"

/// Single SPH particle with all physical quantities
struct Particle {
    Vec2 position;
    Vec2 velocity;
    Vec2 force;
    Vec2 velocityHalf;  // For Velocity Verlet half-step

    float density;
    float pressure;
    float mass;

    // Color field (for surface detection / rendering)
    float colorField;
    Vec2  colorGradient;
    float colorLaplacian;

    // Unique id for debugging
    int id;

    Particle()
        : position(0.0f, 0.0f)
        , velocity(0.0f, 0.0f)
        , force(0.0f, 0.0f)
        , velocityHalf(0.0f, 0.0f)
        , density(0.0f)
        , pressure(0.0f)
        , mass(1.0f)
        , colorField(0.0f)
        , colorGradient(0.0f, 0.0f)
        , colorLaplacian(0.0f)
        , id(-1)
    {}

    Particle(float x, float y, int _id = -1)
        : position(x, y)
        , velocity(0.0f, 0.0f)
        , force(0.0f, 0.0f)
        , velocityHalf(0.0f, 0.0f)
        , density(0.0f)
        , pressure(0.0f)
        , mass(1.0f)
        , colorField(0.0f)
        , colorGradient(0.0f, 0.0f)
        , colorLaplacian(0.0f)
        , id(_id)
    {}
};
