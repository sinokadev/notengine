// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#include <knot/particle.h>

#include <algorithm>
#include <cmath>

#include <glm/gtc/constants.hpp>

namespace knot {
namespace {

/** @brief Smallest capacity a system may be configured with. */
constexpr unsigned int kMinParticles = 1;

/** @brief Upper bound on the pool size, so a bad setting cannot exhaust memory. */
constexpr unsigned int kMaxParticles = 1u << 20u;

/** @brief Unit vector used when the configured direction is degenerate. */
constexpr glm::vec3 kDefaultDirection{0.0f, -1.0f, 0.0f};

/** @brief Reciprocal of the mt19937 output range, mapping a draw to [0, 1). */
constexpr double kRandomScale = 1.0 / 4294967296.0;

/** @brief Orders a two-component range so min <= max. */
glm::vec2 orderedRange(const glm::vec2& range) {
    const float lo = std::min(range.x, range.y);
    const float hi = std::max(range.x, range.y);
    return glm::vec2(std::max(lo, 0.0f), std::max(hi, 0.0f));
}

} // namespace

ParticleSystemSettings ParticleSystem::sanitize(ParticleSystemSettings settings) {
    settings.maxParticles = std::clamp(settings.maxParticles, kMinParticles, kMaxParticles);
    settings.emissionRate = std::max(settings.emissionRate, 0.0f);
    settings.spawnHalfExtents = glm::max(settings.spawnHalfExtents, glm::vec3(0.0f));
    settings.lifetimeRange = glm::max(orderedRange(settings.lifetimeRange), glm::vec2(1.0e-4f));
    settings.speedRange = orderedRange(settings.speedRange);
    settings.directionJitter = std::clamp(settings.directionJitter, 0.0f, glm::pi<float>());
    settings.drag = std::max(settings.drag, 0.0f);
    settings.sizeRange = glm::max(orderedRange(settings.sizeRange), glm::vec2(0.0f));

    return settings;
}

ParticleSystem::ParticleSystem(const ParticleSystemSettings& settings) {
    setSettings(settings);
}

void ParticleSystem::setSettings(const ParticleSystemSettings& settings) {
    settings_ = sanitize(settings);

    const std::size_t capacity = static_cast<std::size_t>(settings_.maxParticles);

    if (pool_.size() != capacity) {
        pool_.resize(capacity);
        aliveCount_ = std::min(aliveCount_, capacity);
    }
}

void ParticleSystem::seed(std::uint32_t value) {
    rng_.seed(value);
}

void ParticleSystem::setOrigin(const glm::vec3& origin) {
    origin_ = origin;
}

void ParticleSystem::clear() {
    aliveCount_ = 0;
    emissionAccumulator_ = 0.0f;
}

void ParticleSystem::update(float dt) {
    if (!std::isfinite(dt) || dt <= 0.0f)
        return;

    const float clamped = std::min(dt, maxDeltaTime);
    const float substepCount = std::ceil(clamped / maxSubstep);
    const int steps = std::max(1, static_cast<int>(substepCount));
    const float stepDt = clamped / static_cast<float>(steps);

    for (int i = 0; i < steps; ++i)
        step(stepDt);
}

void ParticleSystem::step(float dt) {
    // Emission: accumulate the fractional budget and spend whole particles.
    emissionAccumulator_ += settings_.emissionRate * dt;

    while (emissionAccumulator_ >= 1.0f) {
        if (aliveCount_ >= pool_.size()) {
            // The pool is full; drop the budget instead of banking it forever.
            emissionAccumulator_ = 0.0f;
            break;
        }

        spawnParticle();
        emissionAccumulator_ -= 1.0f;
    }

    // Integration: constant acceleration, drag damping, then position.
    const glm::vec3 acceleration = settings_.gravity + settings_.wind;
    const float damping = settings_.drag > 0.0f ? 1.0f / (1.0f + settings_.drag * dt) : 1.0f;

    for (std::size_t i = 0; i < aliveCount_; ++i) {
        Particle& particle = pool_[i];
        particle.velocity = (particle.velocity + acceleration * dt) * damping;
        particle.position += particle.velocity * dt;
        particle.age += dt;
    }

    // Recycling: retire expired particles by pulling the last live one forward.
    std::size_t i = 0;

    while (i < aliveCount_) {
        if (pool_[i].age < pool_[i].lifetime) {
            ++i;
            continue;
        }

        --aliveCount_;

        if (i != aliveCount_)
            pool_[i] = pool_[aliveCount_];
    }
}

void ParticleSystem::spawnParticle() {
    Particle& particle = pool_[aliveCount_];

    const float lifetime = randomFloat(settings_.lifetimeRange.x, settings_.lifetimeRange.y);
    const float speed = randomFloat(settings_.speedRange.x, settings_.speedRange.y);

    particle.position = origin_ + randomInSpawnBox();
    particle.velocity = sampleDirection() * speed;
    particle.age = 0.0f;
    particle.lifetime = lifetime;
    particle.size = randomFloat(settings_.sizeRange.x, settings_.sizeRange.y);
    particle.color = settings_.color;

    ++aliveCount_;
}

float ParticleSystem::randomFloat(float min, float max) {
    // std::uniform_real_distribution maps the generator differently on each
    // standard library, so the mapping is spelled out here: it keeps a run
    // reproducible across platforms for a given std::mt19937 sequence.
    const double unit = static_cast<double>(rng_()) * kRandomScale;
    return min + static_cast<float>(unit) * (max - min);
}

glm::vec3 ParticleSystem::randomInSpawnBox() {
    return glm::vec3(randomFloat(-settings_.spawnHalfExtents.x, settings_.spawnHalfExtents.x),
                     randomFloat(-settings_.spawnHalfExtents.y, settings_.spawnHalfExtents.y),
                     randomFloat(-settings_.spawnHalfExtents.z, settings_.spawnHalfExtents.z));
}

glm::vec3 ParticleSystem::sampleDirection() {
    const float length = glm::length(settings_.direction);
    const glm::vec3 axis = length > 1.0e-6f ? settings_.direction / length : kDefaultDirection;

    if (settings_.directionJitter <= 0.0f)
        return axis;

    // Uniform sampling inside a cone: cos(theta) is drawn uniformly between the
    // cone's cosine bound and 1, then the sample is rotated into the axis frame.
    const float cosMax = std::cos(settings_.directionJitter);
    const float cosTheta = 1.0f - randomFloat(0.0f, 1.0f) * (1.0f - cosMax);
    const float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
    const float phi = randomFloat(0.0f, glm::two_pi<float>());

    const glm::vec3 helper = std::abs(axis.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 tangent = glm::normalize(glm::cross(helper, axis));
    const glm::vec3 bitangent = glm::cross(axis, tangent);

    const glm::vec3 direction = axis * cosTheta + tangent * (sinTheta * std::cos(phi)) + bitangent * (sinTheta * std::sin(phi));
    const float directionLength = glm::length(direction);

    return directionLength > 1.0e-6f ? direction / directionLength : axis;
}

} // namespace knot
