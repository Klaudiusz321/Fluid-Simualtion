#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include "../math/Vec2.hpp"

/// Compact spatial hash grid for TRUE O(n) neighbor search.
///
/// Instead of std::unordered_map (slow, cache-unfriendly, O(n) amortized but
/// with big constant), this uses counting sort into a flat array:
///
/// 1. Hash each particle into a cell → O(n)
/// 2. Count particles per cell (histogram) → O(n)
/// 3. Prefix sum → offsets into sorted array → O(tableSize)
/// 4. Scatter particle indices into sorted array → O(n)
///
/// Query: look up 9 cells (3x3), iterate their contiguous ranges → O(k)
/// Total: O(n + tableSize) per frame, where tableSize is a fixed constant.
///
/// This is the same approach used in GPU SPH (Green 2008, NVIDIA particles demo).
class SpatialHash {
public:
    SpatialHash() : cellSize_(1.0f), invCellSize_(1.0f), tableSize_(0) {}

    explicit SpatialHash(float cellSize, int tableSize = 4096)
        : cellSize_(cellSize)
        , invCellSize_(1.0f / cellSize)
        , tableSize_(tableSize)
    {
        cellStart_.resize(tableSize_ + 1, 0);
        cellEnd_.resize(tableSize_ + 1, 0);
    }

    /// Build the entire grid from scratch — call once per frame.
    /// This replaces the old clear() + insert() + query() pattern.
    void build(const Vec2* positions, int count) {
        particleCount_ = count;
        if (count == 0) return;

        // Resize work arrays if needed
        if (static_cast<int>(cellHash_.size()) < count) {
            cellHash_.resize(count);
            sortedIndices_.resize(count);
        }

        // Step 1: Compute cell hash for each particle — O(n)
        for (int i = 0; i < count; ++i) {
            cellHash_[i] = hashPos(positions[i]) % tableSize_;
        }

        // Step 2: Count particles per cell (histogram) — O(n)
        std::fill(cellStart_.begin(), cellStart_.end(), 0);
        for (int i = 0; i < count; ++i) {
            cellStart_[cellHash_[i] + 1]++;
        }

        // Step 3: Prefix sum → cell start offsets — O(tableSize)
        for (int i = 1; i <= tableSize_; ++i) {
            cellStart_[i] += cellStart_[i - 1];
        }

        // Copy starts for scatter step
        std::copy(cellStart_.begin(), cellStart_.end(), cellEnd_.begin());

        // Step 4: Scatter into sorted array — O(n)
        for (int i = 0; i < count; ++i) {
            int cell = cellHash_[i];
            sortedIndices_[cellEnd_[cell]] = i;
            cellEnd_[cell]++;
        }
    }

    /// Query all particle indices in a cell and its 8 neighbors.
    /// Iterates contiguous memory ranges — extremely cache-friendly.
    inline void queryNeighbors(const Vec2& pos, std::vector<int>& neighbors) const {
        int cx = cellX(pos.x);
        int cy = cellY(pos.y);

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                uint32_t cell = hashCell(cx + dx, cy + dy) % tableSize_;
                int start = cellStart_[cell];
                int end   = cellEnd_[cell];
                for (int k = start; k < end; ++k) {
                    neighbors.push_back(sortedIndices_[k]);
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
    int tableSize_;
    int particleCount_ = 0;

    // Work arrays (reused every frame — no allocations after first build)
    std::vector<uint32_t> cellHash_;      // hash per particle
    std::vector<int> sortedIndices_;      // particle indices sorted by cell
    std::vector<int> cellStart_;          // where each cell starts in sorted array
    std::vector<int> cellEnd_;            // where each cell ends

    inline int cellX(float x) const { return static_cast<int>(std::floor(x * invCellSize_)); }
    inline int cellY(float y) const { return static_cast<int>(std::floor(y * invCellSize_)); }

    inline uint32_t hashCell(int cx, int cy) const {
        // Large primes for spatial hashing (Teschner et al. 2003)
        return static_cast<uint32_t>(
            (static_cast<uint64_t>(static_cast<uint32_t>(cx)) * 73856093ULL) ^
            (static_cast<uint64_t>(static_cast<uint32_t>(cy)) * 19349663ULL)
        );
    }

    inline uint32_t hashPos(const Vec2& pos) const {
        return hashCell(cellX(pos.x), cellY(pos.y));
    }
};
