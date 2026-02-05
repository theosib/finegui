#pragma once

/**
 * @file imgui_impl_finevk.hpp
 * @brief ImGui backend using finevk
 *
 * Internal implementation header for the finevk rendering backend.
 */

#include <finevk/finevk.hpp>

#include <imgui.h>

#include <memory>
#include <vector>
#include <string>

namespace finegui {
namespace backend {

/**
 * @brief Push constant data for GUI shader
 */
struct PushConstantBlock {
    float scale[2];      // 2.0 / displaySize
    float translate[2];  // -1.0
};

/**
 * @brief Per-frame rendering data
 *
 * Maintains vertex/index buffers per frame-in-flight to avoid GPU/CPU sync.
 */
struct FrameRenderData {
    finevk::BufferPtr vertexBuffer;
    finevk::BufferPtr indexBuffer;
    size_t vertexCapacity = 0;
    size_t indexCapacity = 0;
};

/**
 * @brief Registered texture entry
 */
struct TextureEntry {
    finevk::Texture* texture = nullptr;
    finevk::Sampler* sampler = nullptr;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

/**
 * @brief ImGui finevk backend implementation
 */
class ImGuiBackend {
public:
    ImGuiBackend(finevk::LogicalDevice* device, uint32_t framesInFlight);
    ~ImGuiBackend();

    // Non-copyable, non-movable (for simplicity)
    ImGuiBackend(const ImGuiBackend&) = delete;
    ImGuiBackend& operator=(const ImGuiBackend&) = delete;

    /**
     * @brief Initialize rendering resources
     * @param renderPass The render pass to render into
     * @param commandPool Command pool for resource creation
     * @param subpass Subpass index
     * @param msaaSamples MSAA sample count
     */
    void initialize(finevk::RenderPass* renderPass,
                    finevk::CommandPool* commandPool,
                    uint32_t subpass,
                    VkSampleCountFlagBits msaaSamples);

    /**
     * @brief Create/upload font texture
     */
    void createFontTexture(finevk::CommandPool* commandPool);

    /**
     * @brief Register a texture for use in GUI
     * @return Unique texture ID
     */
    uint64_t registerTexture(finevk::Texture* texture, finevk::Sampler* sampler);

    /**
     * @brief Unregister a texture
     */
    void unregisterTexture(uint64_t textureId);

    /**
     * @brief Get descriptor set for a texture ID
     */
    VkDescriptorSet getTextureDescriptor(uint64_t textureId);

    /**
     * @brief Render ImGui draw data
     * @param cmd Command buffer to record into
     * @param frameIndex Current frame-in-flight index
     */
    void render(finevk::CommandBuffer& cmd, uint32_t frameIndex);

    /**
     * @brief Render from captured draw data (for threaded mode)
     * @param cmd Command buffer to record into
     * @param frameIndex Current frame-in-flight index
     * @param data Captured draw data
     */
    void renderDrawData(finevk::CommandBuffer& cmd, uint32_t frameIndex,
                        const struct GuiDrawData& data);

    /**
     * @brief Rebuild font texture from ImGui font atlas
     *
     * Call this after modifying fonts via ImGui's font API.
     */
    void rebuildFontTexture();

    /**
     * @brief Get font texture descriptor (used as default)
     */
    VkDescriptorSet getFontDescriptor() const { return fontDescriptorSet_; }

    /**
     * @brief Get the pipeline layout
     */
    finevk::PipelineLayout* pipelineLayout() { return pipelineLayout_.get(); }

    /**
     * @brief Check if backend is initialized
     */
    bool isInitialized() const { return initialized_; }

private:
    void createPipeline(finevk::RenderPass* renderPass, uint32_t subpass,
                        VkSampleCountFlagBits msaaSamples);
    void createDescriptorResources();
    void ensureBufferCapacity(uint32_t frameIndex, size_t vertexCount, size_t indexCount);
    VkDescriptorSet allocateTextureDescriptor(finevk::Texture* texture, finevk::Sampler* sampler);

    finevk::LogicalDevice* device_ = nullptr;
    finevk::CommandPool* commandPool_ = nullptr;  // Stored for font rebuild
    uint32_t framesInFlight_ = 2;
    bool initialized_ = false;

    // Pipeline resources
    finevk::DescriptorSetLayoutPtr descriptorSetLayout_;
    finevk::PipelineLayoutPtr pipelineLayout_;
    finevk::GraphicsPipelinePtr pipeline_;

    // Descriptor resources
    finevk::DescriptorPoolPtr descriptorPool_;
    VkDescriptorSet fontDescriptorSet_ = VK_NULL_HANDLE;

    // Font texture
    finevk::TextureRef fontTexture_;
    finevk::SamplerPtr fontSampler_;

    // Per-frame data
    std::vector<FrameRenderData> frameData_;

    // Registered textures
    std::unordered_map<uint64_t, TextureEntry> textures_;
    uint64_t nextTextureId_ = 1;

    // Shader paths
    std::string shaderDir_;
};

} // namespace backend
} // namespace finegui
