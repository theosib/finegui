cmake_minimum_required(VERSION 3.20)
project(finegui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =============================================================================
# Options
# =============================================================================
option(FINEGUI_BUILD_TESTS "Build tests" ON)
option(FINEGUI_BUILD_EXAMPLES "Build examples" ON)
option(FINEGUI_BUILD_RETAINED "Build retained-mode widget system" ON)
option(FINEGUI_BUILD_SCRIPT "Build script engine integration (requires finescript)" ON)

# =============================================================================
# Find finevk (sibling project with pre-built libraries)
# =============================================================================
set(FINEVK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../FineStructureVK" CACHE PATH "Path to FineStructureVK root directory")
set(FINEVK_BUILD_DIR "${FINEVK_ROOT}/build")

if(NOT EXISTS "${FINEVK_ROOT}/include/finevk/finevk.hpp")
    message(FATAL_ERROR "finevk not found at ${FINEVK_ROOT}. "
            "Please ensure FineStructureVK is a sibling directory or set FINEVK_ROOT.")
endif()

set(FINEVK_CORE_SHARED "${FINEVK_BUILD_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}finevk-core${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(FINEVK_CORE_STATIC "${FINEVK_BUILD_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}finevk-core${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(FINEVK_ENGINE_SHARED "${FINEVK_BUILD_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}finevk-engine${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(FINEVK_ENGINE_STATIC "${FINEVK_BUILD_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}finevk-engine${CMAKE_STATIC_LIBRARY_SUFFIX}")

if(EXISTS "${FINEVK_CORE_SHARED}")
    set(FINEVK_CORE_LIB "${FINEVK_CORE_SHARED}")
    set(FINEVK_CORE_TYPE SHARED)
elseif(EXISTS "${FINEVK_CORE_STATIC}")
    set(FINEVK_CORE_LIB "${FINEVK_CORE_STATIC}")
    set(FINEVK_CORE_TYPE STATIC)
else()
    message(FATAL_ERROR "finevk libraries not found at ${FINEVK_BUILD_DIR}. "
            "Please build FineStructureVK first.")
endif()

if(EXISTS "${FINEVK_ENGINE_SHARED}")
    set(FINEVK_ENGINE_LIB "${FINEVK_ENGINE_SHARED}")
    set(FINEVK_ENGINE_TYPE SHARED)
elseif(EXISTS "${FINEVK_ENGINE_STATIC}")
    set(FINEVK_ENGINE_LIB "${FINEVK_ENGINE_STATIC}")
    set(FINEVK_ENGINE_TYPE STATIC)
else()
    message(FATAL_ERROR "finevk libraries not found at ${FINEVK_BUILD_DIR}. "
            "Please build FineStructureVK first.")
endif()

# Create imported targets for finevk libraries
if(NOT TARGET finevk-core)
    add_library(finevk-core ${FINEVK_CORE_TYPE} IMPORTED)
    set_target_properties(finevk-core PROPERTIES
        IMPORTED_LOCATION "${FINEVK_CORE_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${FINEVK_ROOT}/include"
    )
endif()

if(NOT TARGET finevk-engine)
    add_library(finevk-engine ${FINEVK_ENGINE_TYPE} IMPORTED)
    set_target_properties(finevk-engine PROPERTIES
        IMPORTED_LOCATION "${FINEVK_ENGINE_LIB}"
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

# ImGui with skin hooks. The submodule stays exactly as it comes from upstream, so taking an
# update is a checkout. The patch goes onto copies of the two files it touches, and a hunk
# that no longer fits stops here: see docs/SKIN_SYSTEM.md for what to do then.
set(IMGUI_PATCH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui-skin-hooks.patch)
set(IMGUI_PATCHED_DIR ${CMAKE_CURRENT_BINARY_DIR}/imgui-patched)
set(IMGUI_PATCH_WORK ${CMAKE_CURRENT_BINARY_DIR}/imgui-patched-work)
file(REMOVE_RECURSE ${IMGUI_PATCH_WORK})
# As LF copies: git on Windows may check them out CRLF, and the patch wouldn't fit
foreach(_imgui_file imgui.cpp imgui_widgets.cpp)
    file(READ ${IMGUI_DIR}/${_imgui_file} _imgui_text)
    string(REPLACE "\r\n" "\n" _imgui_text "${_imgui_text}")
    file(WRITE ${IMGUI_PATCH_WORK}/${_imgui_file} "${_imgui_text}")
endforeach()
# Windows has no patch of its own, but Git for Windows brings one
set(_patch_hints)
if(WIN32)
    find_package(Git QUIET)
    if(GIT_EXECUTABLE)
        get_filename_component(_git_dir "${GIT_EXECUTABLE}" DIRECTORY)
        list(APPEND _patch_hints "${_git_dir}/../usr/bin" "${_git_dir}/../../usr/bin")
    endif()
    list(APPEND _patch_hints "C:/Program Files/Git/usr/bin")
endif()
find_program(PATCH_PROGRAM patch HINTS ${_patch_hints} REQUIRED)
execute_process(
    COMMAND ${PATCH_PROGRAM} -p1 --batch --forward -i ${IMGUI_PATCH}
    WORKING_DIRECTORY ${IMGUI_PATCH_WORK}
    RESULT_VARIABLE IMGUI_PATCH_RESULT
    OUTPUT_VARIABLE IMGUI_PATCH_OUTPUT
    ERROR_VARIABLE IMGUI_PATCH_OUTPUT)
if(NOT IMGUI_PATCH_RESULT EQUAL 0)
    message(FATAL_ERROR "The skin hooks no longer fit this ImGui:\n${IMGUI_PATCH_OUTPUT}")
endif()
# Through configure_file so that an unchanged result doesn't recompile ImGui
configure_file(${IMGUI_PATCH_WORK}/imgui.cpp ${IMGUI_PATCHED_DIR}/imgui.cpp COPYONLY)
configure_file(${IMGUI_PATCH_WORK}/imgui_widgets.cpp ${IMGUI_PATCHED_DIR}/imgui_widgets.cpp COPYONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${IMGUI_PATCH} ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_widgets.cpp)

set(IMGUI_SOURCES
    ${IMGUI_PATCHED_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_PATCHED_DIR}/imgui_widgets.cpp
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
    src/scene_texture.cpp
    src/skin.cpp
    src/backend/imgui_impl_finevk.cpp
)

set(FINEGUI_HEADERS
    include/finegui/finegui.hpp
    include/finegui/gui_system.hpp
    include/finegui/imgui_skin_hooks.h
    include/finegui/skin.hpp
    include/finegui/gui_config.hpp
    include/finegui/gui_state.hpp
    include/finegui/gui_draw_data.hpp
    include/finegui/input_adapter.hpp
    include/finegui/texture_handle.hpp
    include/finegui/texture_registry.hpp
    include/finegui/scene_texture.hpp
    include/finegui/widget_state.hpp
)

# Helper function to configure a finegui library target (static or shared)
function(finegui_configure_target target_name)
    target_include_directories(${target_name}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${IMGUI_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    # Make ImGui headers available to consumers
    target_include_directories(${target_name}
        PUBLIC
            $<BUILD_INTERFACE:${IMGUI_DIR}>
    )

    target_link_libraries(${target_name}
        PUBLIC
            finevk-core
            finevk-engine
            Vulkan::Vulkan
            glfw
            glm::glm
    )

    # Add shader output directory for runtime loading
    target_compile_definitions(${target_name}
        PRIVATE
            FINEGUI_SHADER_DIR="${SHADER_OUTPUT_DIR}"
    )

    # Compiler warnings
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    elseif(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
    endif()

    # Add shader dependency
    if(TARGET finegui_shaders)
        add_dependencies(${target_name} finegui_shaders)
    endif()
endfunction()

# --- Static library ---
add_library(finegui STATIC ${FINEGUI_SOURCES} ${FINEGUI_HEADERS})
finegui_configure_target(finegui)

# --- Shared library ---
# Not on Windows: nothing is marked dllexport, and the DLL's import library would be finegui.lib,
# the static one's name. The -shared names stand for the static libraries there.
if(WIN32)
    add_library(finegui-shared ALIAS finegui)
else()
    add_library(finegui-shared SHARED ${FINEGUI_SOURCES} ${FINEGUI_HEADERS})
    finegui_configure_target(finegui-shared)
    set_target_properties(finegui-shared PROPERTIES
        OUTPUT_NAME finegui
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endif()

# =============================================================================
# finegui-retained library (optional retained-mode widget system)
# =============================================================================
if(FINEGUI_BUILD_RETAINED)
    set(FINEGUI_RETAINED_SOURCES
        src/retained/widget_node.cpp
        src/retained/gui_renderer.cpp
        src/retained/drag_drop_manager.cpp
        src/retained/tween_manager.cpp
        src/retained/hotkey_manager.cpp
    )

    set(FINEGUI_RETAINED_HEADERS
        include/finegui/widget_node.hpp
        include/finegui/gui_renderer.hpp
        include/finegui/drag_drop_manager.hpp
        include/finegui/tween_manager.hpp
        include/finegui/hotkey_manager.hpp
    )

    # Helper function to configure a finegui-retained library target
    function(finegui_retained_configure_target target_name link_finegui_target)
        target_include_directories(${target_name}
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src
        )

        # ImGui headers needed for the renderer
        target_include_directories(${target_name}
            PUBLIC
                $<BUILD_INTERFACE:${IMGUI_DIR}>
        )

        target_link_libraries(${target_name}
            PUBLIC
                ${link_finegui_target}
        )

        # Compiler warnings
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
        elseif(MSVC)
            target_compile_options(${target_name} PRIVATE /W4)
        endif()
    endfunction()

    # --- Static library ---
    add_library(finegui-retained STATIC ${FINEGUI_RETAINED_SOURCES} ${FINEGUI_RETAINED_HEADERS})
    finegui_retained_configure_target(finegui-retained finegui)

    # --- Shared library ---
    if(WIN32)
        add_library(finegui-retained-shared ALIAS finegui-retained)
    else()
        add_library(finegui-retained-shared SHARED ${FINEGUI_RETAINED_SOURCES} ${FINEGUI_RETAINED_HEADERS})
        finegui_retained_configure_target(finegui-retained-shared finegui-shared)
        set_target_properties(finegui-retained-shared PROPERTIES
            OUTPUT_NAME finegui-retained
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}
        )
    endif()
endif()

# =============================================================================
# Find finescript (optional, sibling project)
# =============================================================================
if(FINEGUI_BUILD_SCRIPT)
    if(NOT FINEGUI_BUILD_RETAINED)
        message(FATAL_ERROR "FINEGUI_BUILD_SCRIPT requires FINEGUI_BUILD_RETAINED=ON")
    endif()

    set(FINESCRIPT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../game-language" CACHE PATH "Path to game-language (finescript) root directory")
    set(FINESCRIPT_BUILD_DIR "${FINESCRIPT_ROOT}/build")

    if(NOT EXISTS "${FINESCRIPT_ROOT}/include/finescript/script_engine.h")
        message(FATAL_ERROR "finescript not found at ${FINESCRIPT_ROOT}. "
                "Please ensure game-language is a sibling directory or set FINESCRIPT_ROOT.")
    endif()

    set(FINESCRIPT_SHARED "${FINESCRIPT_BUILD_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}finescript${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(FINESCRIPT_STATIC "${FINESCRIPT_BUILD_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}finescript${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(EXISTS "${FINESCRIPT_SHARED}")
        set(FINESCRIPT_LIB "${FINESCRIPT_SHARED}")
        set(FINESCRIPT_TYPE SHARED)
    elseif(EXISTS "${FINESCRIPT_STATIC}")
        set(FINESCRIPT_LIB "${FINESCRIPT_STATIC}")
        set(FINESCRIPT_TYPE STATIC)
    else()
        message(FATAL_ERROR "finescript library not found at ${FINESCRIPT_BUILD_DIR}. "
                "Please build game-language first.")
    endif()

    if(NOT TARGET finescript)
        add_library(finescript ${FINESCRIPT_TYPE} IMPORTED)
        set_target_properties(finescript PROPERTIES
            IMPORTED_LOCATION "${FINESCRIPT_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${FINESCRIPT_ROOT}/include"
        )
    endif()
endif()

# =============================================================================
# finegui-script library (optional script engine integration)
# =============================================================================
if(FINEGUI_BUILD_SCRIPT)
    set(FINEGUI_SCRIPT_SOURCES
        src/script/widget_converter.cpp
        src/script/map_renderer.cpp
        src/script/script_bindings.cpp
        src/script/script_gui.cpp
        src/script/script_gui_manager.cpp
        src/script/skin_loader.cpp
    )

    set(FINEGUI_SCRIPT_HEADERS
        include/finegui/widget_converter.hpp
        include/finegui/map_renderer.hpp
        include/finegui/script_bindings.hpp
        include/finegui/script_gui.hpp
        include/finegui/script_gui_manager.hpp
    )

    function(finegui_script_configure_target target_name link_retained_target)
        target_include_directories(${target_name}
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src
        )

        target_include_directories(${target_name}
            PUBLIC
                $<BUILD_INTERFACE:${IMGUI_DIR}>
        )

        target_link_libraries(${target_name}
            PUBLIC
                ${link_retained_target}
                finescript
        )

        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
        elseif(MSVC)
            target_compile_options(${target_name} PRIVATE /W4)
        endif()
    endfunction()

    # --- Static library ---
    add_library(finegui-script STATIC ${FINEGUI_SCRIPT_SOURCES} ${FINEGUI_SCRIPT_HEADERS})
    finegui_script_configure_target(finegui-script finegui-retained)

    # --- Shared library ---
    if(WIN32)
        add_library(finegui-script-shared ALIAS finegui-script)
    else()
        add_library(finegui-script-shared SHARED ${FINEGUI_SCRIPT_SOURCES} ${FINEGUI_SCRIPT_HEADERS})
        finegui_script_configure_target(finegui-script-shared finegui-retained-shared)
        set_target_properties(finegui-script-shared PROPERTIES
            OUTPUT_NAME finegui-script
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}
        )
    endif()
endif()

# =============================================================================
# Tests
# =============================================================================
if(FINEGUI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# =============================================================================
# Examples
# =============================================================================
if(FINEGUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
