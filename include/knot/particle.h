// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include <glm/glm.hpp>

namespace knot {

/**
 * @brief Tuning for a ParticleSystem.
 *
 * All values are in world units and seconds. The defaults are deliberately
 * neutral: gravity and wind are zero, drag is zero and the default direction
 * points straight down. A demo or game is expected to override them with a
 * preset (rain, snow, smoke, sparks, ...).
 */
struct ParticleSystemSettings {
    /** @brief Hard upper bound on live particles. The pool is allocated once at this size. */
    unsigned int maxParticles = 4096;

    /** @brief Number of particles spawned per second while the pool has room. */
    float emissionRate = 128.0f;

    /** @brief Half extents of the spawn box centered on the system origin. */
    glm::vec3 spawnHalfExtents{1.0f};

    /** @brief Minimum and maximum lifetime of a spawned particle, in seconds. */
    glm::vec2 lifetimeRange{1.0f, 2.0f};

    /** @brief Minimum and maximum initial speed of a spawned particle. */
    glm::vec2 speedRange{1.0f, 2.0f};

    /** @brief Base emission direction. It is normalized before use. */
    glm::vec3 direction{0.0f, -1.0f, 0.0f};

    /** @brief Maximum angular deviation from direction, as a cone half-angle in radians. */
    float directionJitter = 0.0f;

    /** @brief Constant acceleration, typically (0, -9.81, 0). */
    glm::vec3 gravity{0.0f};

    /** @brief Constant acceleration added on top of gravity, used for a prevailing wind. */
    glm::vec3 wind{0.0f};

    /** @brief Velocity damping coefficient. Velocity is scaled by 1 / (1 + drag * dt) per substep. */
    float drag = 0.0f;

    /** @brief Minimum and maximum billboard size of a spawned particle, in world units. */
    glm::vec2 sizeRange{0.1f, 0.1f};

    /** @brief Base color of a spawned particle. The alpha channel is used as an opacity scale. */
    glm::vec4 color{1.0f};
};

/**
 * @brief One live particle produced by a ParticleSystem.
 *
 * The struct is plain data so it can be uploaded to a GPU buffer verbatim; the
 * renderer reads position, size, color and the normalized age for the billboard
 * pass. Nothing here touches OpenGL.
 */
struct Particle {
    /** @brief World-space center of the particle. */
    glm::vec3 position{0.0f};

    /** @brief World-space velocity in units per second. */
    glm::vec3 velocity{0.0f};

    /** @brief Seconds elapsed since the particle was spawned. */
    float age = 0.0f;

    /** @brief Total lifetime of the particle in seconds. */
    float lifetime = 1.0f;

    /** @brief Billboard size in world units. */
    float size = 0.1f;

    /** @brief Base color, copied from the settings at spawn time. */
    glm::vec4 color{1.0f};

    /**
     * @brief Returns the fraction of the lifetime that has elapsed.
     *
     * @return 0 at spawn, 1 at expiry, clamped to that range.
     */
    float normalizedAge() const {
        if (lifetime <= 0.0f)
            return 1.0f;

        const float t = age / lifetime;

        if (t <= 0.0f)
            return 0.0f;

        return t >= 1.0f ? 1.0f : t;
    }
};

/**
 * @brief Deterministic, GPU-agnostic CPU particle simulation.
 *
 * The system keeps a fixed-capacity pool of particles and advances it in
 * update(). Emission, motion integration and lifetime recycling all happen on
 * the CPU, so the same system can feed any renderer (instanced billboards,
 * sprites, debug points) and can be unit tested without a graphics context.
 *
 * Simulation rules, applied per substep:
 *  - emission: @c emissionAccumulator grows by emissionRate * dt; every whole
 *    particle it holds is spawned at the origin plus a random point inside
 *    spawnHalfExtents, then the whole part is subtracted again.
 *  - integration: velocity gains (gravity + wind) * dt, is damped by
 *    drag, and position advances by velocity * dt. Age grows by dt.
 *  - recycling: a particle whose age reaches its lifetime is removed by moving
 *    the last live particle into its slot, so the live range stays contiguous
 *    and never exceeds maxParticles.
 *
 * Determinism: the system owns a std::mt19937 seeded through seed(). Random
 * values are derived with a fixed mapping from the generator output, so the
 * same seed and the same sequence of update() calls produce bit-identical
 * particle state. Large deltas are split into substeps by a rule that depends
 * only on the delta, so even a stalled frame replays identically.
 */
class ParticleSystem {
public:
    /** @brief Largest delta accepted by update(); anything longer is clamped to it. */
    static constexpr float maxDeltaTime = 0.25f;

    /** @brief Longest single integration substep, in seconds. */
    static constexpr float maxSubstep = 1.0f / 60.0f;

    /**
     * @brief Builds an empty system and allocates its fixed-capacity pool.
     *
     * @param settings Initial tuning; out-of-range values are sanitized.
     */
    explicit ParticleSystem(const ParticleSystemSettings& settings = ParticleSystemSettings{});

    /**
     * @brief Advances the simulation.
     *
     * A delta longer than maxDeltaTime is clamped, then split into equally
     * sized substeps of at most maxSubstep seconds. Non-positive and non-finite
     * deltas are ignored.
     *
     * @param dt Seconds of simulated time.
     */
    void update(float dt);

    /**
     * @brief Resets the random generator.
     *
     * Particles already alive keep their state; a fresh system plus a fixed
     * seed is what makes a whole run reproducible.
     *
     * @param value Seed for std::mt19937.
     */
    void seed(std::uint32_t value);

    /**
     * @brief Moves the spawn box and therefore future emission.
     *
     * Live particles keep their world positions; only newly spawned ones use
     * the new origin. This is how a rain or smoke system follows a camera.
     *
     * @param origin Center of the spawn box in world space.
     */
    void setOrigin(const glm::vec3& origin);

    /** @brief Returns the current spawn box center. */
    const glm::vec3& origin() const {
        return origin_;
    }

    /**
     * @brief Replaces the tuning and reallocates the pool if maxParticles changed.
     *
     * If the new capacity is smaller than the live count, the excess particles
     * are dropped.
     *
     * @param settings New tuning; out-of-range values are sanitized.
     */
    void setSettings(const ParticleSystemSettings& settings);

    /** @brief Returns the sanitized settings currently in use. */
    const ParticleSystemSettings& settings() const {
        return settings_;
    }

    /**
     * @brief Read-only view of the pool.
     *
     * Only the first aliveCount() entries are live; the rest is recycled
     * storage and must not be read. The live entries are contiguous, which lets
     * a renderer upload them in one call.
     */
    const std::vector<Particle>& particles() const {
        return pool_;
    }

    /** @brief Number of live particles. */
    std::size_t aliveCount() const {
        return aliveCount_;
    }

    /** @brief Fixed capacity of the pool, equal to maxParticles. */
    std::size_t capacity() const {
        return pool_.size();
    }

    /** @brief Removes every live particle and drops the pending emission budget. */
    void clear();

private:
    /** @brief Applies one substep of emission, integration and recycling. */
    void step(float dt);

    /** @brief Spawns one particle into the next free slot. */
    void spawnParticle();

    /** @brief Clamps the settings into a physically sane range. */
    static ParticleSystemSettings sanitize(ParticleSystemSettings settings);

    /** @brief Returns a random float in [min, max) using a fixed generator mapping. */
    float randomFloat(float min, float max);

    /** @brief Returns a random point uniformly distributed inside the spawn box. */
    glm::vec3 randomInSpawnBox();

    /** @brief Returns a unit emission direction inside the jitter cone. */
    glm::vec3 sampleDirection();

    /** @brief Sanitized tuning. */
    ParticleSystemSettings settings_{};

    /** @brief Fixed-capacity particle storage; the first aliveCount_ entries are live. */
    std::vector<Particle> pool_{};

    /** @brief Number of live particles. */
    std::size_t aliveCount_ = 0;

    /** @brief Fractional particles owed to the emission rate. */
    float emissionAccumulator_ = 0.0f;

    /** @brief Center of the spawn box. */
    glm::vec3 origin_{0.0f};

    /** @brief Deterministic random source. */
    std::mt19937 rng_{0u};
};

} // namespace knot
