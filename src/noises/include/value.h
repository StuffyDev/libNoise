// src/noises/value.h
#pragma once

#include "noise_base.h"

class ValueNoise : public NoiseBase {
public:
    ValueNoise(unsigned int seed = 42);

    real_t GetNoise(real_t x, real_t y) const override;

private:
    unsigned int m_seed;

    // Helper functions
    // Spatially hashes integer coordinates (x, y) to get a unique random seed for each grid node
    unsigned int hash(int x, int y) const;
    // Quintic interpolation (6t^5 - 15t^4 + 10t^3)
    real_t fade(real_t t) const;
    // Standard linear interpolation
    real_t lerp(real_t t, real_t a, real_t b) const;
    // Gets a pseudorandom value [0, 1] at a specific integer grid coordinate
    real_t value_at(int x, int y) const;
};
