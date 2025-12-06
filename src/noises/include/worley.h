// src/noises/include/worley.h
#pragma once

#include "noise_base.h"
#include <vector>

// Simple struct to hold a feature point's coordinates within a cell [0, 1]
struct FeaturePoint {
    real_t x;
    real_t y;
};

// Available distance metrics (different "shapes" for the cells)
enum class DistanceMetric {
    Euclidean,      // Standard distance: sqrt(dx^2 + dy^2) - most common, smooth edges
    Manhattan,      // Taxicab distance: |dx| + |dy| - diamond-shaped cells
    Chebyshev       // Chessboard distance: max(|dx|, |dy|) - square cells
};

// Available Worley functions (determining the final output value)
enum class WorleyFunction {
    F1,     // Distance to the closest feature point (standard Voronoi)
    F2,     // Distance to the second closest point
    F2_F1   // Difference (F2 - F1) which creates sharp cell boundaries/ridges
};

class WorleyNoise : public NoiseBase {
public:
    WorleyNoise(unsigned int seed = 42,
                DistanceMetric metric = DistanceMetric::Euclidean,
                WorleyFunction func = WorleyFunction::F1,
                int points_per_cell = 1);

    real_t GetNoise(real_t x, real_t y) const override;

private:
    unsigned int m_seed;
    DistanceMetric m_metric;
    WorleyFunction m_func;
    int m_points_per_cell;

    // Helper methods
    real_t calculate_distance(real_t dx, real_t dy) const;
    // Hashes the integer cell coordinates to create a reproducible random seed for that cell
    unsigned int hash_cell(int i, int j) const;
    // Generates a feature point's coordinates [0, 1] based on the cell hash and point index
    FeaturePoint get_point(unsigned int cell_hash, int point_index) const;
};
