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

    /** @brief Ambient light strength. */
    float ambient = 1.0f;
    /** @brief Resolution of the directional shadow map in pixels. */
    unsigned int dir_shadow_res = 2048;
    /** @brief Resolution of each point-light shadow face in pixels. */
    unsigned int point_shadow_res = 512;
};

/** @brief Global renderer settings used by the engine. */
inline EngineSettings engine_settings;
/** @brief Short alias for engine_settings. */
inline auto& s = engine_settings;

} // namespace knot
