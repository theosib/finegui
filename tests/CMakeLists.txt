# Tests CMakeLists.txt
# Tests link against the shared library (finegui-shared)

# Phase 1 test - Input adapter and basic GuiSystem
add_executable(test_phase1
    test_phase1.cpp
)

target_link_libraries(test_phase1 PRIVATE finegui-shared)

# Phase 2 test - Rendering (requires window)
add_executable(test_phase2
    test_phase2.cpp
)

target_link_libraries(test_phase2 PRIVATE finegui-shared)
