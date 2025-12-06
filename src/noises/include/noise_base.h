// src/noises/noise_base.h

#pragma once

using real_t = double;

/**
 * @brief Abstract base class for noise generation algorithms.
 * * All specific implementations of noise (Perlin, Simplex, Worley, etc.)
 * must inherit this class and implement the GetNoise method.
 */
class NoiseBase {
public:
    virtual ~NoiseBase() = default;

    /**
     * @brief Generates a noise value at a 2D point.
     * @param x X-axis coordinate.
     * @param y Y-axis coordinate.
     * @return The noise value, typically in the range [-1.0, 1.0] or [0.0, 1.0].
     */
    virtual real_t GetNoise(real_t x, real_t y) const = 0;
};
