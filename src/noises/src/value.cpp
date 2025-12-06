// src/noises/src/value.cpp

#include "../include/value.h"
#include <cmath>
#include <numeric>
#include <limits> // For numeric_limits

ValueNoise::ValueNoise(unsigned int seed) : m_seed(seed) {}

// Spatial hashing function (similar to Worley's)
unsigned int ValueNoise::hash(int x, int y) const {
    unsigned int h = m_seed;
    // Mix coordinates using large primes
    h ^= (unsigned int)x * 0x85ebca6b;
    h ^= (unsigned int)y * 0xc2b2ae3d;
    h *= 0x9e3779b9;

    // Final mixing steps
    h ^= (h >> 16);
    h *= 0x85ebca6b;
    h ^= (h >> 13);
    h *= 0xc2b2ae3d;
    h ^= (h >> 16);

    return h;
}

// Converts the hash value to a floating-point value in the range [0, 1]
real_t ValueNoise::value_at(int x, int y) const {
    return (real_t)hash(x, y) / (real_t)std::numeric_limits<unsigned int>::max();
}

// Quintic interpolation (ensures C2 continuity across cell boundaries)
real_t ValueNoise::fade(real_t t) const {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

// Linear interpolation
real_t ValueNoise::lerp(real_t t, real_t a, real_t b) const {
    return a + t * (b - a);
}

// Main noise generation method for 2D
real_t ValueNoise::GetNoise(real_t x, real_t y) const {
    // 1. Find the integer grid coordinates of the bottom-left node
    int X = (int)std::floor(x);
    int Y = (int)std::floor(y);

    // 2. Find the fractional components (offset inside the cell [0, 1])
    real_t xf = x - X;
    real_t yf = y - Y;

    // 3. Compute interpolation weights using the fade function
    real_t u = fade(xf);
    real_t v = fade(yf);

    // 4. Get the 4 random values at the grid corners
    real_t v00 = value_at(X, Y);
    real_t v10 = value_at(X + 1, Y);
    real_t v01 = value_at(X, Y + 1);
    real_t v11 = value_at(X + 1, Y + 1);

    // 5. Interpolate across the X-axis
    real_t x1 = lerp(u, v00, v10); // Bottom row
    real_t x2 = lerp(u, v01, v11); // Top row

    // 6. Interpolate across the Y-axis
    // The output is naturally [0, 1], so we map it to [-1.0, 1.0]
    return lerp(v, x1, x2) * 2.0 - 1.0;
}
