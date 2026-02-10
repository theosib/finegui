#pragma once

/**
 * @file imgui_impl_finevk.hpp
 * @brief ImGui backend using finevk
 *
 * Internal implementation header for the finevk rendering backend.
 * Supports ImGui 1.92+ with ImGuiBackendFlags_RendererHasTextures.
 */

#include <finegui/gui_draw_data.hpp>

#include <finevk/finevk.hpp>

#include <imgui.h>

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

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
 * @brief Backend texture data stored in ImTextureData::BackendUserData
 */
struct BackendTextureData {
    finevk::TextureRef texture;
    finevk::DescriptorSetPtr descriptorSet;
};

/**
 * @brief Registered texture entry (for user-registered textures)
 */
struct TextureEntry {
    finevk::Texture* texture = nullptr;
    finevk::Sampler* sampler = nullptr;
    finevk::DescriptorSetPtr descriptorSet;
};

/**
 * @brief ImGui finevk backend implementation
 */
class ImGuiBackend {
public:
    ImGuiBackend(finevk::RenderSurface* surface);
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
     * @brief Register a texture for use in GUI
     * @return Unique texture ID
     */
    uint64_t registerTexture(finevk::Texture* texture, finevk::Sampler* sampler);

    /**
     * @brief Register an image view for use in GUI (e.g. offscreen render result)
     * @param imageView The image view to sample from
     * @param sampler The sampler to use (uses default if null)
     * @return Unique texture ID
     */
    uint64_t registerTexture(finevk::ImageView* imageView, finevk::Sampler* sampler);

    /**
     * @brief Unregister a texture
     */
    void unregisterTexture(uint64_t textureId);

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
                        const GuiDrawData& data);

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
    finevk::DescriptorSetPtr allocateTextureDescriptor(finevk::Texture* texture, finevk::Sampler* sampler);
    finevk::DescriptorSetPtr allocateTextureDescriptor(VkImageView view, VkSampler sampler);

    // ImGui 1.92+ texture lifecycle
    void updateTexture(ImTextureData* tex);
    void destroyTexture(ImTextureData* tex);

    finevk::RenderSurface* surface_ = nullptr;
    finevk::LogicalDevice* device_ = nullptr;
    finevk::CommandPool* commandPool_ = nullptr;
    uint32_t framesInFlight_ = 2;
    bool initialized_ = false;

    // Pipeline resources
    finevk::DescriptorSetLayoutPtr descriptorSetLayout_;
    finevk::PipelineLayoutPtr pipelineLayout_;
    finevk::GraphicsPipelinePtr pipeline_;

    // Descriptor resources
    finevk::DescriptorPoolPtr descriptorPool_;

    // Default sampler for textures
    finevk::SamplerPtr defaultSampler_;

    // Per-frame data
    std::vector<FrameRenderData> frameData_;

    // User-registered textures, keyed by VkDescriptorSet handle
    std::unordered_map<uint64_t, TextureEntry> textures_;

    // Shader paths
    std::string shaderDir_;
};

} // namespace backend
} // namespace finegui
