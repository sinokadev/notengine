#pragma once

#include <cmath>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <glm/glm.hpp>

// Unlike assert(), checks remain active in Release builds.
#define CHECK(expression)                                                                                      \
    do {                                                                                                       \
        if (!(expression))                                                                                     \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " #expression); \
    } while (false)

inline bool near(float actual, float expected, float tolerance = 1e-4f) {
    return std::isfinite(actual) && std::abs(actual - expected) <= tolerance;
}

inline bool near(const glm::vec3& actual, const glm::vec3& expected, float tolerance = 1e-4f) {
    return near(actual.x, expected.x, tolerance) && near(actual.y, expected.y, tolerance) && near(actual.z, expected.z, tolerance);
}

inline glm::vec3 project(const glm::mat4& matrix, const glm::vec3& point) {
    const auto clip = matrix * glm::vec4(point, 1.0f);
    return glm::vec3(clip) / clip.w;
}

inline int runTests(std::initializer_list<std::pair<const char*, void (*)()>> cases) {
    int failures = 0;
    for (const auto& test : cases) {
        try {
            test.second();
            std::cout << "PASS: " << test.first << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "FAIL: " << test.first << ": " << error.what() << '\n';
        } catch (...) {
            ++failures;
            std::cerr << "FAIL: " << test.first << ": unknown exception\n";
        }
    }
    std::cout << cases.size() - failures << '/' << cases.size() << " cases passed\n";
    return failures == 0 ? 0 : 1;
}
