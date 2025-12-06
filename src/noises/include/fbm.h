// src/noises/include/fbm.h
#pragma once

#include "noise_base.h"
#include <memory> // For std::unique_ptr

class FBMNoise : public NoiseBase {
public:
    /**
     * @brief Constructor for Fractal Brownian Motion (FBM), a procedural texture.
     * FBM sums multiple octaves of a base noise algorithm (like Perlin or Simplex).
     * * @param base_noise The base noise generator object (e.g., a SimplexNoise instance).
     * @param octaves The number of noise layers to sum.
     * @param lacunarity The frequency multiplier (how much faster the next octave is).
     * @param persistence The amplitude multiplier (how much weaker the next octave is).
     */
    FBMNoise(std::unique_ptr<NoiseBase> base_noise,
             int octaves = 6,
             real_t lacunarity = 2.0,
             real_t persistence = 0.5);

    real_t GetNoise(real_t x, real_t y) const override;

private:
    // Holds ownership of the base noise algorithm
    std::unique_ptr<NoiseBase> m_base_noise;

    int m_octaves;
    real_t m_lacunarity;
    real_t m_persistence;
};
