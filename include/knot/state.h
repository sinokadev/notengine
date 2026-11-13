#pragma once

namespace knot {

/** @brief Runtime feature switches and shadow quality settings for the renderer. */
struct EngineSettings {
    /** @brief Enables the environment sky map. */
    bool skymap = true;
    /** @brief Enables shadow rendering. */
    bool shadow = true;
    /** @brief Enables screen-space ambient occlusion. */
    bool ssao = true;
    /** @brief Enables local reflection-probe contributions to image-based lighting. (Experimental features) */
    bool reflection_probe = false;

    /** @brief Ambient light strength. */
    float ambient = 1.0f;
    /** @brief Resolution of the directional shadow map in pixels. */
    unsigned int dir_shadow_res = 2048;
    /** @brief Resolution of each point-light shadow face in pixels. */
    unsigned int point_shadow_res = 512;

    /** @brief Minimum directional-shadow depth bias in normalized depth units.
     *  @see https://docs.unity.com/en-us/engine/6000.6/manual/lighting-overview/shadows/shadow-performance */
    float dir_shadow_bias = 0.0005f;
    /** @brief Additional directional-shadow bias at grazing angles. */
    float dir_shadow_slope_bias = 0.0025f;
    /** @brief Minimum point-shadow bias in world units. */
    float point_shadow_bias = 0.008f;
    /** @brief Additional point-shadow bias at grazing angles in world units. */
    float point_shadow_slope_bias = 0.03f;
    /** @brief PCF kernel radius: 0 = 1x1, 1 = 3x3; values are clamped to [0, 4]. */
    int shadow_pcf_steps = 1;
};

/** @brief Global renderer settings used by the engine. */
inline EngineSettings engine_settings;
/** @brief Short alias for engine_settings. */
inline auto& s = engine_settings;

} // namespace knot
