# These tests exercise CPU-only behavior; no window, graphics context, or audio device.
add_custom_target(unit_tests)
foreach(suite IN ITEMS transform camera manager tween scene_asset_path)
    add_executable(knot_${suite}_tests ${suite}_tests.cpp)
    target_link_libraries(knot_${suite}_tests PRIVATE notengine)
    add_dependencies(unit_tests knot_${suite}_tests)
    add_test(NAME knot.unit.${suite} COMMAND knot_${suite}_tests)
    set_tests_properties(knot.unit.${suite} PROPERTIES LABELS unit TIMEOUT 30)
endforeach()
