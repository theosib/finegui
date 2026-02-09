# Examples CMakeLists.txt

# Simple demo - basic GUI rendering
add_executable(simple_demo
    simple_demo.cpp
)

target_link_libraries(simple_demo PRIVATE finegui)

# Copy shader files to example build directory
if(TARGET finegui_shaders)
    add_dependencies(simple_demo finegui_shaders)
endif()

# Retained-mode demo
if(FINEGUI_BUILD_RETAINED)
    add_executable(retained_demo
        retained_demo.cpp
    )
    target_link_libraries(retained_demo PRIVATE finegui-retained)
    if(TARGET finegui_shaders)
        add_dependencies(retained_demo finegui_shaders)
    endif()
endif()

# Script demo - script + retained-mode combined
if(FINEGUI_BUILD_SCRIPT)
    add_executable(script_demo
        script_demo.cpp
    )
    target_link_libraries(script_demo PRIVATE finegui-script)
    if(TARGET finegui_shaders)
        add_dependencies(script_demo finegui_shaders)
    endif()
endif()
