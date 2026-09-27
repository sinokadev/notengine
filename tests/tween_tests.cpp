#include "test_support.h"
#include <knot/utility/tween.h>

namespace {
knot::Tween activeTween(int duration = 1000) {
    knot::Tween tween;
    tween.duration = duration;
    tween.is_finished = false;
    tween.target.position = {10, 20, 30};
    tween.target.scale = {3, 5, 7};
    tween.target.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0));
    return tween;
}

void interpolation() {
    const auto tween = activeTween();
    auto start = knot::mixTransform(tween.start, tween.target, 0);
    auto end = knot::mixTransform(tween.start, tween.target, 1);
    auto middle = knot::mixTransform(tween.start, tween.target, 0.5f);
    CHECK(near(start.position, {0, 0, 0}));
    CHECK(near(end.position, {10, 20, 30}));
    CHECK(near(end.getFront(), {-1, 0, 0}));
    CHECK(near(middle.position, {5, 10, 15}));
    CHECK(near(middle.scale, {2, 3, 4}));
    const float diagonal = std::sqrt(0.5f);
    CHECK(near(middle.getFront(), {-diagonal, 0, -diagonal}));
}

void progressAndCompletion() {
    auto tween = activeTween();
    knot::Object object;
    tween.update(object, 0.25f);
    CHECK(tween.elapsed_time == 250);
    CHECK(!tween.is_finished);
    CHECK(near(object.position, {2.5f, 5, 7.5f}));
    tween.update(object, 2);
    CHECK(tween.is_finished);
    CHECK(near(object.position, tween.target.position));
    CHECK(near(object.scale, tween.target.scale));
    CHECK(near(object.getFront(), tween.target.getFront()));
    object.position = {-1, -2, -3};
    tween.update(object, 1);
    CHECK(near(object.position, {-1, -2, -3}));
}

void easingAndFallback() {
    auto tween = activeTween();
    tween.ease_func = [](float t) { return t * t; };
    knot::Object object;
    tween.update(object, 0.5f);
    CHECK(near(object.position, {2.5f, 5, 7.5f}));
    CHECK(!tween.is_finished);
    tween = activeTween();
    tween.ease_func = {};
    tween.update(object, 0.5f);
    CHECK(near(object.position, {5, 10, 15}));
}

void fractionalMilliseconds() {
    auto tween = activeTween(2);
    knot::Object object;
    tween.update(object, 0.00025f);
    CHECK(tween.elapsed_time == 0);
    CHECK(near(object.position, {0, 0, 0}));
    for (int i = 0; i < 3; ++i)
        tween.update(object, 0.00025f);
    CHECK(tween.elapsed_time == 1);
    CHECK(near(object.position, {5, 10, 15}));
    for (int i = 0; i < 4; ++i)
        tween.update(object, 0.00025f);
    CHECK(tween.is_finished);
    CHECK(near(object.position, tween.target.position));
}

void inactiveAndNonpositiveDuration() {
    knot::Object object;
    object.position = {1, 2, 3};
    knot::Tween inactive;
    inactive.update(object, 1);
    CHECK(near(object.position, {1, 2, 3}));
    for (int duration : {0, -10}) {
        auto tween = activeTween(duration);
        tween.update(object, 0);
        CHECK(tween.is_finished);
        CHECK(near(object.position, tween.target.position));
    }
    auto tween = activeTween();
    tween.update(object, 0);
    CHECK(!tween.is_finished);
    CHECK(tween.elapsed_time == 0);
    CHECK(near(object.position, tween.start.position));
}
} // namespace

int main() {
    return runTests({{"interpolation", interpolation}, {"progress and completion", progressAndCompletion},
                     {"easing and fallback", easingAndFallback}, {"fractional milliseconds", fractionalMilliseconds},
                     {"inactive and nonpositive duration", inactiveAndNonpositiveDuration}});
}
