// src/noises/src/worley.cpp

#include "../include/worley.h"
#include <cmath>
#include <algorithm>
#include <limits>

WorleyNoise::WorleyNoise(unsigned int seed, DistanceMetric metric, WorleyFunction func, int points_per_cell)
    : m_seed(seed), m_metric(metric), m_func(func), m_points_per_cell(points_per_cell) {}

// A basic spatial hashing function for a 2D integer cell (i, j)
unsigned int WorleyNoise::hash_cell(int i, int j) const {
    unsigned int h = m_seed;
    // Use large prime numbers to mix coordinates and prevent grid artifacts
    h ^= (unsigned int)i * 0x85ebca6b;
    h ^= (unsigned int)j * 0xc2b2ae3d;
    h *= 0x9e3779b9; // Final multiplication (similar to FNV or MurmurHash mixing)

    // Final bitwise mixing steps
    h ^= (h >> 16);
    h *= 0x85ebca6b;
    h ^= (h >> 13);
    h *= 0xc2b2ae3d;
    h ^= (h >> 16);
    return h;
}

// Generates a reproducible random point within the [0, 1] cell range
FeaturePoint WorleyNoise::get_point(unsigned int cell_hash, int point_index) const {
    // Mix the cell hash with the point index (k) for unique positions within the cell
    unsigned int sub_hash = cell_hash ^ (unsigned int)point_index;

    // Convert the 32-bit hash value into a real_t value between [0, 1]
    // Note: 0x1.0p-32 is a floating-point literal equal to 1 / 2^32
    real_t rand_x = (real_t)((sub_hash * 0x85ebca6b) >> 1) * 0x1.0p-32;
    real_t rand_y = (real_t)((sub_hash * 0xc2b2ae3d) >> 1) * 0x1.0p-32;

    return {rand_x, rand_y};
}

real_t WorleyNoise::calculate_distance(real_t dx, real_t dy) const {
    switch (m_metric) {
        case DistanceMetric::Euclidean:
            // Standard smooth distance (results in round cells)
            return std::sqrt(dx * dx + dy * dy);
        case DistanceMetric::Manhattan:
            // Diamond-shaped cells
            return std::abs(dx) + std::abs(dy);
        case DistanceMetric::Chebyshev:
            // Square/Box-shaped cells
            return std::max(std::abs(dx), std::abs(dy));
        default:
            return std::sqrt(dx * dx + dy * dy);
    }
}

real_t WorleyNoise::GetNoise(real_t x, real_t y) const {
    // 1. Determine the integer cell coordinates for the sample point (x, y)
    int i = (int)std::floor(x);
    int j = (int)std::floor(y);

    // 2. Initialize F1 (closest distance) and F2 (second closest distance)
    real_t F1 = std::numeric_limits<real_t>::max();
    real_t F2 = std::numeric_limits<real_t>::max();

    // 3. Search the 9 surrounding cells (including the current cell)
    // This ensures we find the true closest points.
    for (int cell_i = i - 1; cell_i <= i + 1; ++cell_i) {
        for (int cell_j = j - 1; cell_j <= j + 1; ++cell_j) {

            unsigned int cell_hash = hash_cell(cell_i, cell_j);

            // Generate all feature points within this cell
            for (int k = 0; k < m_points_per_cell; ++k) {
                FeaturePoint p = get_point(cell_hash, k);

                // Calculate the displacement vector from the feature point to (x, y)
                real_t dx = x - (cell_i + p.x);
                real_t dy = y - (cell_j + p.y);

                real_t dist = calculate_distance(dx, dy);

                // Simple two-element sorting to keep track of F1 and F2
                if (dist < F1) {
                    F2 = F1; // The old F1 becomes the new F2
                    F1 = dist; // The new distance is the new F1
                } else if (dist < F2) {
                    F2 = dist; // The new distance is between F1 and F2
                }
            }
        }
    }

    // 4. Return the requested Worley function output
    switch (m_func) {
        case WorleyFunction::F1:
            // Standard Voronoi pattern (closest distance)
            return F1;
        case WorleyFunction::F2:
            // Distance to the second closest point
            return F2;
        case WorleyFunction::F2_F1:
            // Creates the sharp boundary/ridge effect
            return F2 - F1;
        default:
            return F1;
    }
}
