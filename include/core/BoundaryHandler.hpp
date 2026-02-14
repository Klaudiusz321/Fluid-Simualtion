#pragma once

#include "../math/Vec2.hpp"
#include <vector>

/// Handles boundary conditions for the fluid simulation.
/// Supports simple box boundaries and signed-distance-field (SDF) boundaries.
class BoundaryHandler {
public:
    BoundaryHandler() = default;

    /// Set box boundaries
    void setBox(float minX, float minY, float maxX, float maxY);

    /// Enforce boundaries on a particle — clamp and reflect velocity
    void enforce(Vec2& position, Vec2& velocity, float damping = -0.5f) const;

    /// Check if a point is inside the boundary
    bool isInside(const Vec2& pos) const;

    /// Get the signed distance to the nearest wall (negative = inside)
    float signedDistance(const Vec2& pos) const;

    /// Normal pointing inward at position
    Vec2 boundaryNormal(const Vec2& pos) const;

private:
    float minX_ = 0.0f, minY_ = 0.0f;
    float maxX_ = 1280.0f, maxY_ = 720.0f;
};
