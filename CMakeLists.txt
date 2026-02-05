cmake_minimum_required(VERSION 3.20)
project(finegui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =============================================================================
# Options
# =============================================================================
option(FINEGUI_BUILD_TESTS "Build tests" ON)
option(FINEGUI_BUILD_EXAMPLES "Build examples" ON)

# =============================================================================
# Find finevk (sibling project with pre-built libraries)
# =============================================================================
set(FINEVK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../FineStructureVK")
set(FINEVK_BUILD_DIR "${FINEVK_ROOT}/build")

if(NOT EXISTS "${FINEVK_ROOT}/include/finevk/finevk.hpp")
    message(FATAL_ERROR "finevk not found at ${FINEVK_ROOT}. "
            "Please ensure FineStructureVK is a sibling directory.")
endif()

if(NOT EXISTS "${FINEVK_BUILD_DIR}/libfinevk-core.dylib")
    message(FATAL_ERROR "finevk libraries not found at ${FINEVK_BUILD_DIR}. "
            "Please build FineStructureVK first.")
endif()

# Create imported targets for finevk libraries
if(NOT TARGET finevk-core)
    add_library(finevk-core SHARED IMPORTED)
    set_target_properties(finevk-core PROPERTIES
        IMPORTED_LOCATION "${FINEVK_BUILD_DIR}/libfinevk-core.dylib"
        INTERFACE_INCLUDE_DIRECTORIES "${FINEVK_ROOT}/include"
    )
endif()

if(NOT TARGET finevk-engine)
    add_library(finevk-engine SHARED IMPORTED)
    set_target_properties(finevk-engine PROPERTIES
        IMPORTED_LOCATION "${FINEVK_BUILD_DIR}/libfinevk-engine.dylib"
        INTERFACE_INCLUDE_DIRECTORIES "${FINEVK_ROOT}/include"
    )
endif()

# =============================================================================
# Find Vulkan (for shader compilation)
# =============================================================================
find_package(Vulkan REQUIRED)

# =============================================================================
# Find GLFW and GLM (required by finevk headers)
# =============================================================================
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# =============================================================================
# ImGui
# =============================================================================
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)

if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
    message(FATAL_ERROR "ImGui not found at ${IMGUI_DIR}. "
            "Please run: git submodule add https://github.com/ocornut/imgui.git third_party/imgui")
endif()

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
)

# =============================================================================
# Shader compilation
# =============================================================================
if(Vulkan_glslc_FOUND OR EXISTS "${Vulkan_GLSLC_EXECUTABLE}")
    set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders)
    set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

    file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

    # Compile vertex shader
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_DIR}/gui.vert.spv
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${SHADER_DIR}/gui.vert -o ${SHADER_OUTPUT_DIR}/gui.vert.spv
        DEPENDS ${SHADER_DIR}/gui.vert
        COMMENT "Compiling gui.vert"
    )

    # Compile fragment shader
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_DIR}/gui.frag.spv
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${SHADER_DIR}/gui.frag -o ${SHADER_OUTPUT_DIR}/gui.frag.spv
        DEPENDS ${SHADER_DIR}/gui.frag
        COMMENT "Compiling gui.frag"
    )

    add_custom_target(finegui_shaders ALL
        DEPENDS
            ${SHADER_OUTPUT_DIR}/gui.vert.spv
            ${SHADER_OUTPUT_DIR}/gui.frag.spv
    )
else()
    message(WARNING "glslc not found. Shaders will not be compiled automatically.")
endif()

# =============================================================================
# finegui library
# =============================================================================
set(FINEGUI_SOURCES
    ${IMGUI_SOURCES}
    src/input_adapter.cpp
    src/gui_system.cpp
    src/state_dispatcher.cpp
    src/texture_registry.cpp
    src/backend/imgui_impl_finevk.cpp
)

set(FINEGUI_HEADERS
    include/finegui/finegui.hpp
    include/finegui/gui_system.hpp
    include/finegui/gui_config.hpp
    include/finegui/gui_state.hpp
    include/finegui/gui_draw_data.hpp
    include/finegui/input_adapter.hpp
    include/finegui/texture_handle.hpp
)

add_library(finegui STATIC ${FINEGUI_SOURCES} ${FINEGUI_HEADERS})

target_include_directories(finegui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${IMGUI_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Make ImGui headers available to consumers
target_include_directories(finegui
    PUBLIC
        $<BUILD_INTERFACE:${IMGUI_DIR}>
)

target_link_libraries(finegui
    PUBLIC
        finevk-core
        finevk-engine
        Vulkan::Vulkan
        glfw
        glm::glm
)

# Add shader output directory for runtime loading
target_compile_definitions(finegui
    PRIVATE
        FINEGUI_SHADER_DIR="${SHADER_OUTPUT_DIR}"
)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(finegui PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(finegui PRIVATE /W4)
endif()

# Add shader dependency
if(TARGET finegui_shaders)
    add_dependencies(finegui finegui_shaders)
endif()

# =============================================================================
# Tests
# =============================================================================
if(FINEGUI_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# =============================================================================
# Examples
# =============================================================================
if(FINEGUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
