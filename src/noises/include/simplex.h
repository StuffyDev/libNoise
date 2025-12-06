// src/noises/include/simplex.h
#pragma once

#include "noise_base.h"
#include <vector>

class SimplexNoise : public NoiseBase {
public:
    SimplexNoise(unsigned int seed = 42);
    real_t GetNoise(real_t x, real_t y) const override;

private:
    unsigned int m_seed;
    std::vector<int> m_p; // Permutation table

    // Simplex 2D constants for coordinate skewing/unskewing
    // F2: Factor for squishing the square grid into a triangular lattice
    static constexpr real_t F2 = 0.3660254037844386; // (sqrt(3) - 1) / 2
    // G2: Factor for unsquishing the coordinates back
    static constexpr real_t G2 = 0.21132486540518713; // (3 - sqrt(3)) / 6

    void initialize_permutation_table();

    // Attenuation function: (0.5 - r^2)^4. Used to smoothly taper off the noise contribution.
    real_t attn_func(real_t t) const { return t > 0 ? t * t * t * t : 0; }

    /**
     * @brief Computes the dot product between the distance vector (dx, dy)
     * and one of the 12 predefined Simplex gradient vectors.
     */
    real_t grad(int hash, real_t dx, real_t dy) const;
};
