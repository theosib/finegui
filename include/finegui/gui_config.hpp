#pragma once

/**
 * @file gui_config.hpp
 * @brief Configuration types for GuiSystem
 */

#include <vulkan/vulkan.h>
#include <string>
#include <cstdint>

namespace finegui {

/**
 * @brief Configuration for GuiSystem initialization
 */
struct GuiConfig {
    // ========================================================================
    // Display settings
    // ========================================================================

    /// DPI scale factor (0 = auto-detect from framebuffer scale)
    float dpiScale = 0.0f;

    /// Additional font scaling multiplier
    float fontScale = 1.0f;

    // ========================================================================
    // Font configuration (use one approach)
    // ========================================================================

    /// Path to TTF font file (empty = ImGui default font)
    std::string fontPath;

    /// Font size in pixels
    float fontSize = 16.0f;

    /// Font data pointer (alternative to fontPath)
    const void* fontData = nullptr;

    /// Font data size in bytes
    size_t fontDataSize = 0;

    // ========================================================================
    // Behavior settings
    // ========================================================================

    /// Enable keyboard navigation
    bool enableKeyboard = true;

    /// Enable gamepad navigation
    bool enableGamepad = false;

    // ========================================================================
    // Performance settings
    // ========================================================================

    /// Number of frames in flight (0 = auto from device->framesInFlight())
    uint32_t framesInFlight = 0;

    /// Enable draw data capture for threaded mode
    bool enableDrawDataCapture = false;

    // ========================================================================
    // Rendering settings
    // ========================================================================

    /// MSAA sample count (must match render pass)
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
};

} // namespace finegui
