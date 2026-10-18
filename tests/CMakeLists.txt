# Tests CMakeLists.txt
# Tests link against the shared library (finegui-shared)

# Phase 1 test - Input adapter and basic GuiSystem
add_executable(test_phase1
    test_phase1.cpp
)

target_link_libraries(test_phase1 PRIVATE finegui-shared)
add_test(NAME test_phase1 COMMAND test_phase1)

# Phase 2 test - Rendering (requires window)
add_executable(test_phase2
    test_phase2.cpp
)

target_link_libraries(test_phase2 PRIVATE finegui-shared)
add_test(NAME test_phase2 COMMAND test_phase2)

# Retained-mode tests
if(FINEGUI_BUILD_RETAINED)
    # Unit tests - widget tree construction (no Vulkan required)
    add_executable(test_retained_unit
        test_retained_unit.cpp
    )
    target_link_libraries(test_retained_unit PRIVATE finegui-retained-shared)
    add_test(NAME test_retained_unit COMMAND test_retained_unit)

    # Integration tests - rendering with Vulkan
    add_executable(test_retained_render
        test_retained_render.cpp
    )
    target_link_libraries(test_retained_render PRIVATE finegui-retained-shared)
    add_test(NAME test_retained_render COMMAND test_retained_render)
endif()

# Script integration tests
if(FINEGUI_BUILD_SCRIPT)
    # Unit tests - conversion, bindings, ScriptGui logic (no Vulkan required)
    add_executable(test_script_unit
        test_script_unit.cpp
    )
    target_link_libraries(test_script_unit PRIVATE finegui-script-shared)
    add_test(NAME test_script_unit COMMAND test_script_unit)

    # Integration tests - rendering with Vulkan
    add_executable(test_script_render
        test_script_render.cpp
    )
    target_link_libraries(test_script_render PRIVATE finegui-script-shared)
    add_test(NAME test_script_render COMMAND test_script_render)
endif()
