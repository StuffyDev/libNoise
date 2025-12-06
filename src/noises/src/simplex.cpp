// src/noises/src/simplex.cpp

#include "../include/simplex.h"
#include <cmath>
#include <random>
#include <algorithm>
#include <numeric>

constexpr int SIMPLEX_TABLE_SIZE = 256;

// Predefined 2D Simplex Gradient vectors (12 unique vectors)
// The vectors are scaled to have length approximately 1.
static const real_t GRADIENTS_2D[16][2] = {
    {1, 1}, {-1, 1}, {1, -1}, {-1, -1},
    {1, 0}, {-1, 0}, {0, 1}, {0, -1},
    {1, 1}, {-1, 1}, {1, -1}, {-1, -1},
    {1, 0}, {-1, 0}, {0, 1}, {0, -1} // Repeat to ensure index % 12 works smoothly
};

SimplexNoise::SimplexNoise(unsigned int seed) : m_seed(seed) {
    initialize_permutation_table();
}

void SimplexNoise::initialize_permutation_table() {
    m_p.resize(SIMPLEX_TABLE_SIZE);
    std::iota(m_p.begin(), m_p.end(), 0);

    std::default_random_engine engine(m_seed);
    std::shuffle(m_p.begin(), m_p.end(), engine);

    // Double the table length to avoid explicit modulo 256 later
    m_p.insert(m_p.end(), m_p.begin(), m_p.end());
}

real_t SimplexNoise::grad(int hash, real_t dx, real_t dy) const {
    // Use modulo 12 to select one of the 12 unique gradient vectors
    int index = hash % 12;
    return GRADIENTS_2D[index][0] * dx + GRADIENTS_2D[index][1] * dy;
}

real_t SimplexNoise::GetNoise(real_t x, real_t y) const {
    // 1. Skew the input space to map from a square grid to a triangular lattice
    real_t s = (x + y) * F2;
    real_t xs = x + s;
    real_t ys = y + s;
    int i = (int)std::floor(xs);
    int j = (int)std::floor(ys);

    // 2. Unskew the integer cell coordinates back to get the origin (x0, y0)
    real_t t = (i + j) * G2;
    real_t x0 = x - (i - t);
    real_t y0 = y - (j - t);

    // 3. Determine the order of the remaining two vertices (i1, j1) and (i2, j2)
    // The second vertex is defined by which quadrant (x0 > y0 or x0 < y0)
    int i1, j1;
    if (x0 > y0) {
        i1 = 1; j1 = 0; // The displacement (1, 0)
    } else {
        i1 = 0; j1 = 1; // The displacement (0, 1)
    }
    // The third vertex is always (1, 1) displacement

    // 4. Get the coordinates of the other two vertices relative to the sample point (x, y)
    real_t x1 = x0 - i1 + G2;
    real_t y1 = y0 - j1 + G2;
    real_t x2 = x0 - 1.0 + 2.0 * G2;
    real_t y2 = y0 - 1.0 + 2.0 * G2;

    // 5. Calculate the noise contribution for each of the three vertices
    real_t noise = 0.0;

    // Contribution from the first vertex (i, j)
    real_t t0 = 0.5 - x0 * x0 - y0 * y0;
    if (t0 > 0) {
        // Hash the vertex coordinates
        int gi0 = m_p[i & 255] + (j & 255);
        // Attenuation function ensures noise contribution smoothly falls off
        noise += attn_func(t0) * grad(m_p[gi0], x0, y0);
    }

    // Contribution from the second vertex (i + i1, j + j1)
    real_t t1 = 0.5 - x1 * x1 - y1 * y1;
    if (t1 > 0) {
        int gi1 = m_p[(i + i1) & 255] + ((j + j1) & 255);
        noise += attn_func(t1) * grad(m_p[gi1], x1, y1);
    }

    // Contribution from the third vertex (i + 1, j + 1)
    real_t t2 = 0.5 - x2 * x2 - y2 * y2;
    if (t2 > 0) {
        int gi2 = m_p[(i + 1) & 255] + ((j + 1) & 255);
        noise += attn_func(t2) * grad(m_p[gi2], x2, y2);
    }

    // Final normalization to the range [-1, 1]. The standard factor for 2D is 32 or 40.
    return noise * 40.0;
}
