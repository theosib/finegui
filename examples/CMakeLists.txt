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
