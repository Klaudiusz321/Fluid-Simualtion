#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include "../math/Vec2.hpp"

/// Spatial hash grid for O(n) neighbor search.
/// Divides 2D space into cells of size `cellSize` and hashes particle indices.
class SpatialHash {
public:
    SpatialHash() : cellSize_(1.0f), invCellSize_(1.0f) {}

    explicit SpatialHash(float cellSize)
        : cellSize_(cellSize)
        , invCellSize_(1.0f / cellSize)
    {}

    /// Clear all cells (call once per frame before inserting)
    void clear() {
        grid_.clear();
    }

    /// Insert a particle index at a given position
    void insert(int particleIndex, const Vec2& pos) {
        uint64_t key = hashPos(pos);
        grid_[key].push_back(particleIndex);
    }

    /// Get all particle indices in a cell and its 8 neighbors
    /// Results are appended to `neighbors`
    void queryNeighbors(const Vec2& pos, std::vector<int>& neighbors) const {
        int cx = cellX(pos.x);
        int cy = cellY(pos.y);

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                uint64_t key = hashCell(cx + dx, cy + dy);
                auto it = grid_.find(key);
                if (it != grid_.end()) {
                    neighbors.insert(neighbors.end(), it->second.begin(), it->second.end());
                }
            }
        }
    }

    void setCellSize(float size) {
        cellSize_ = size;
        invCellSize_ = 1.0f / size;
    }

    float getCellSize() const { return cellSize_; }

private:
    float cellSize_;
    float invCellSize_;
    std::unordered_map<uint64_t, std::vector<int>> grid_;

    inline int cellX(float x) const { return static_cast<int>(std::floor(x * invCellSize_)); }
    inline int cellY(float y) const { return static_cast<int>(std::floor(y * invCellSize_)); }

    inline uint64_t hashCell(int cx, int cy) const {
        // Large primes for spatial hashing (Teschner et al. 2003)
        return (static_cast<uint64_t>(cx) * 73856093ULL) ^
               (static_cast<uint64_t>(cy) * 19349663ULL);
    }

    inline uint64_t hashPos(const Vec2& pos) const {
        return hashCell(cellX(pos.x), cellY(pos.y));
    }
};
