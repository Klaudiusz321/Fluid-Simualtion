#include "../../include/core/BoundaryHandler.hpp"
#include <cmath>
#include <algorithm>

void BoundaryHandler::setBox(float minX, float minY, float maxX, float maxY) {
    minX_ = minX;
    minY_ = minY;
    maxX_ = maxX;
    maxY_ = maxY;
}

void BoundaryHandler::enforce(Vec2& position, Vec2& velocity, float damping) const {
    if (position.x < minX_) {
        position.x = minX_;
        velocity.x *= damping;
    }
    if (position.x > maxX_) {
        position.x = maxX_;
        velocity.x *= damping;
    }
    if (position.y < minY_) {
        position.y = minY_;
        velocity.y *= damping;
    }
    if (position.y > maxY_) {
        position.y = maxY_;
        velocity.y *= damping;
    }
}

bool BoundaryHandler::isInside(const Vec2& pos) const {
    return pos.x >= minX_ && pos.x <= maxX_ &&
           pos.y >= minY_ && pos.y <= maxY_;
}

float BoundaryHandler::signedDistance(const Vec2& pos) const {
    // Distance to the nearest wall (negative = inside)
    float dx = std::max(minX_ - pos.x, pos.x - maxX_);
    float dy = std::max(minY_ - pos.y, pos.y - maxY_);
    return std::max(dx, dy);
}

Vec2 BoundaryHandler::boundaryNormal(const Vec2& pos) const {
    // Return the inward-pointing normal of the closest wall
    float distLeft   = pos.x - minX_;
    float distRight  = maxX_ - pos.x;
    float distTop    = pos.y - minY_;
    float distBottom = maxY_ - pos.y;

    float minDist = std::min({distLeft, distRight, distTop, distBottom});

    if (minDist == distLeft)   return Vec2( 1.0f,  0.0f);
    if (minDist == distRight)  return Vec2(-1.0f,  0.0f);
    if (minDist == distTop)    return Vec2( 0.0f,  1.0f);
    return Vec2(0.0f, -1.0f);
}
