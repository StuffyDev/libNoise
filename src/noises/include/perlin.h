// src/noises/perlin.h
#pragma once

#include "noise_base.h"
#include <vector>

class PerlinNoise : public NoiseBase {
public:
    PerlinNoise(unsigned int seed = 42);

    real_t GetNoise(real_t x, real_t y) const override;

private:
    unsigned int m_seed;
    // Permutation table (Perlin's original P array, doubled for seamless access)
    // Used for pseudo-random gradient selection
    std::vector<int> m_p;

    // Support functions
    void initialize_permutation_table();

    // 5th order interpolating function
    real_t fade(real_t t) const;

    // Linear interpolation
    real_t lerp(real_t t, real_t a, real_t b) const;

    /**
     * @brief Calculates the scalar product of the displacement vector (dx, dy)
     * and the gradient vector selected by hash
     */
    real_t grad(int hash, real_t dx, real_t dy) const;
};
