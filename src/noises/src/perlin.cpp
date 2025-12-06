// src/noises/perlin.cpp

#include "../include/perlin.h"
#include <cmath>
#include <numeric>
#include <random>
#include <algorithm>

// Standard size for the permutation table in classic Perlin
constexpr int PERLIN_TABLE_SIZE = 256;

PerlinNoise::PerlinNoise(unsigned int seed) : m_seed(seed) {
    initialize_permutation_table();
}

/**
 * @brief Initializes the permutation table (P-array).
 * Generates a pseudo-random, shuffled array of numbers from 0 to 255,
 * and then doubles it to simplify indexing/wrapping (m_p[i] == m_p[i & 255]).
 */
void PerlinNoise::initialize_permutation_table() {
    m_p.resize(PERLIN_TABLE_SIZE);
    // Fill the array with 0, 1, 2, ..., 255
    std::iota(m_p.begin(), m_p.end(), 0);

    // Use the provided seed to shuffle the sequence
    std::default_random_engine engine(m_seed);
    std::shuffle(m_p.begin(), m_p.end(), engine);

    // Double the array for easy indexing without explicit modulo 256
    m_p.insert(m_p.end(), m_p.begin(), m_p.end());
}

// Quintic interpolation (6t^5 - 15t^4 + 10t^3)
// This function ensures the 1st and 2nd derivatives are zero at t=0 and t=1,
// which eliminates blocky artifacts (C2 continuity).
real_t PerlinNoise::fade(real_t t) const {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

// Standard linear interpolation
real_t PerlinNoise::lerp(real_t t, real_t a, real_t b) const {
    return a + t * (b - a);
}

/**
 * @brief Calculates the dot product between the gradient vector and the distance vector.
 * The gradient vector is selected based on the hash (m_p[hash]).
 * * @param hash Gradient index (0-255, obtained from the permutation table).
 * @param dx X distance vector component (from grid node to the sample point).
 * @param dy Y distance vector component (from grid node to the sample point).
 * @return Dot product result.
 */
real_t PerlinNoise::grad(int hash, real_t dx, real_t dy) const {
    // We use the simplified set of 4 axial gradients (faster but slightly less isotropic)
    hash &= 3; // Keep only 4 gradients: (1,0), (-1,0), (0,1), (0,-1)

    // Calculate the dot product (G * D)
    if (hash == 0) return dx;       // Gradient (1, 0)
    if (hash == 1) return -dx;      // Gradient (-1, 0)
    if (hash == 2) return dy;       // Gradient (0, 1)
    return -dy;                     // Gradient (0, -1)
}


// Main noise generation method for 2D
real_t PerlinNoise::GetNoise(real_t x, real_t y) const {
    // 1. Find the coordinates of the bottom-left grid cell node
    int X = (int)std::floor(x) & 255;
    int Y = (int)std::floor(y) & 255;

    // 2. Find the fractional components (offset inside the cell [0, 1])
    real_t xf = x - std::floor(x);
    real_t yf = y - std::floor(y);

    // Distance vectors from the four grid nodes to the sample point (xf, yf)
    real_t dx0 = xf;       // Distance from (0, 0) node
    real_t dy0 = yf;
    real_t dx1 = xf - 1.0; // Distance from (1, 1) node
    real_t dy1 = yf - 1.0;

    // 3. Compute interpolation weights using the fade function
    real_t u = fade(xf);
    real_t v = fade(yf);

    // 4. Hash the 4 grid corners to get their gradient indices.
    // The hash table is doubled, so modulo 256 is implicit.
    int A = m_p[X] + Y;       // Hashing for row 0 (Y)
    int B = m_p[X + 1] + Y;   // Hashing for row 1 (Y)

    // 5. Compute the 4 dot products (Gradient * Distance)
    // Node (0, 0): P[X+0] + Y+0
    real_t g00 = grad(m_p[A], dx0, dy0);
    // Node (1, 0): P[X+1] + Y+0
    real_t g10 = grad(m_p[B], dx1, dy0);
    // Node (0, 1): P[X+0] + Y+1
    real_t g01 = grad(m_p[A + 1], dx0, dy1);
    // Node (1, 1): P[X+1] + Y+1
    real_t g11 = grad(m_p[B + 1], dx1, dy1);

    // 6. Interpolate across the X-axis (between the two rows)
    real_t x1 = lerp(u, g00, g10); // Interpolate row 0 (y=0)
    real_t x2 = lerp(u, g01, g11); // Interpolate row 1 (y=1)

    // 7. Interpolate across the Y-axis
    real_t noise_val = lerp(v, x1, x2);

    // Perlin noise typically results in a range around [-0.7, 0.7].
    // We scale it slightly to better fit the expected [-1, 1] range.
    return noise_val * 2.1;
}
