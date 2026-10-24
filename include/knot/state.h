#pragma once

namespace knot {

struct EngineSettings {
    bool skymap = true;
    bool shadow = true;
    bool ssao = true;
    
    float ambient = 1.0f;
    unsigned int dir_shadow_res = 2048;
    unsigned int point_shadow_res = 512;
};

inline EngineSettings engine_settings;
inline auto& s = engine_settings;

} // namespace knot