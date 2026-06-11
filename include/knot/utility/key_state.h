#pragma once

namespace knot {

enum class KeyState : int {
    RELEASE = 0,
    PRESS   = 1,
    REPEAT  = 2
};

} // namespace knot