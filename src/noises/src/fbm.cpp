// src/noises/src/fbm.cpp

#include "../include/fbm.h"
#include <cmath>

FBMNoise::FBMNoise(std::unique_ptr<NoiseBase> base_noise,
                   int octaves,
                   real_t lacunarity,
                   real_t persistence)
    : m_base_noise(std::move(base_noise)),
      m_octaves(octaves),
      m_lacunarity(lacunarity),
      m_persistence(persistence)
{
    // A quick check to ensure the base noise was correctly passed
    if (!m_base_noise) {
        // Handle error: FBM requires a valid base noise generator.
    }
}

real_t FBMNoise::GetNoise(real_t x, real_t y) const {
    real_t total = 0.0;
    real_t amplitude = 1.0;
    real_t frequency = 1.0;
    real_t max_amplitude = 0.0; // Tracks the sum of amplitudes for final normalization

    for (int i = 0; i < m_octaves; ++i) {
        // 1. Generate the noise for the current octave, scaled by frequency.
        real_t noise_val = m_base_noise->GetNoise(x * frequency, y * frequency);

        // 2. Add the octave's contribution to the total.
        // The noise value is scaled by the current amplitude (persistence).
        total += noise_val * amplitude;

        // 3. Keep a running sum of the total maximum possible amplitude (for normalization)
        max_amplitude += amplitude;

        // 4. Update parameters for the next, finer octave.
        frequency *= m_lacunarity; // Frequency increases (finer detail)
        amplitude *= m_persistence; // Amplitude decreases (less influence)
    }

    // 5. Final Normalization.
    // Dividing by the sum of amplitudes guarantees the result is in the range [-1.0, 1.0].
    if (max_amplitude > 0.0) {
        return total / max_amplitude;
    }

    return 0.0;
}
