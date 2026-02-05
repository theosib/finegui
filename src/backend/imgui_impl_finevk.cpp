/**
 * @file imgui_impl_finevk.cpp
 * @brief ImGui finevk backend implementation
 */

#include "imgui_impl_finevk.hpp"

#include <finegui/gui_draw_data.hpp>

#include <stdexcept>
#include <cstring>

namespace finegui {
namespace backend {

// ============================================================================
// Constructor/Destructor
// ============================================================================

ImGuiBackend::ImGuiBackend(finevk::LogicalDevice* device, uint32_t framesInFlight)
    : device_(device)
    , framesInFlight_(framesInFlight)
{
    if (!device_) {
        throw std::runtime_error("ImGuiBackend: device cannot be null");
    }

    // Get shader directory from compile definition or use relative path
#ifdef FINEGUI_SHADER_DIR
    shaderDir_ = FINEGUI_SHADER_DIR;
#else
    shaderDir_ = "shaders";
#endif

    // Initialize per-frame data
    frameData_.resize(framesInFlight_);
}

ImGuiBackend::~ImGuiBackend() {
    if (device_) {
        device_->waitIdle();
    }
}

// ============================================================================
// Initialization
// ============================================================================

void ImGuiBackend::initialize(finevk::RenderPass* renderPass,
                               finevk::CommandPool* commandPool,
                               uint32_t subpass,
                               VkSampleCountFlagBits msaaSamples)
{
    if (!renderPass || !commandPool) {
        throw std::runtime_error("ImGuiBackend::initialize: renderPass and commandPool required");
    }

    commandPool_ = commandPool;  // Store for font rebuild

    createDescriptorResources();
    createPipeline(renderPass, subpass, msaaSamples);
    createFontTexture(commandPool);

    initialized_ = true;
}

void ImGuiBackend::createDescriptorResources() {
    // Create descriptor set layout for combined image sampler
    descriptorSetLayout_ = finevk::DescriptorSetLayout::create(device_)
        .combinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    // Create descriptor pool with enough sets for font + registered textures
    // Start with 100, can be expanded if needed
    descriptorPool_ = finevk::DescriptorPool::create(device_)
        .maxSets(100)
        .poolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100)
        .allowFree(true)
        .build();
}

void ImGuiBackend::createPipeline(finevk::RenderPass* renderPass,
                                   uint32_t subpass,
                                   VkSampleCountFlagBits msaaSamples)
{
    // Create pipeline layout with push constants
    pipelineLayout_ = finevk::PipelineLayout::create(device_)
        .descriptorSetLayout(descriptorSetLayout_.get())
        .pushConstant<PushConstantBlock>(VK_SHADER_STAGE_VERTEX_BIT)
        .build();

    // Build shader paths
    std::string vertPath = shaderDir_ + "/gui.vert.spv";
    std::string fragPath = shaderDir_ + "/gui.frag.spv";

    // Create graphics pipeline
    pipeline_ = finevk::GraphicsPipeline::create(device_, renderPass, pipelineLayout_.get())
        .vertexShader(vertPath)
        .fragmentShader(fragPath)
        // Vertex input matching ImDrawVert
        .vertexBinding(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX)
        .vertexAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos))
        .vertexAttribute(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv))
        .vertexAttribute(2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col))
        // Rasterization
        .cullNone()
        .topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        // Blending (premultiplied alpha)
        .alphaBlending()
        // Depth
        .depthTest(false)
        .depthWrite(false)
        // Dynamic state
        .dynamicViewportAndScissor()
        // MSAA
        .samples(msaaSamples)
        // Subpass
        .subpass(subpass)
        .build();
}

void ImGuiBackend::createFontTexture(finevk::CommandPool* commandPool) {
    ImGuiIO& io = ImGui::GetIO();

    // Get font atlas data
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Create texture from memory
    fontTexture_ = finevk::Texture::fromMemory(
        device_,
        pixels,
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        commandPool,
        false,  // No mipmaps
        false   // Not sRGB
    );

    // Create sampler
    fontSampler_ = finevk::Sampler::create(device_)
        .filter(VK_FILTER_LINEAR)
        .addressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
        .build();

    // Allocate descriptor set for font
    fontDescriptorSet_ = allocateTextureDescriptor(fontTexture_.get(), fontSampler_.get());

    // Store font texture ID in ImGui
    io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(static_cast<uint64_t>(0)));
}

// ============================================================================
// Texture management
// ============================================================================

uint64_t ImGuiBackend::registerTexture(finevk::Texture* texture, finevk::Sampler* sampler) {
    if (!texture) {
        throw std::runtime_error("ImGuiBackend::registerTexture: texture cannot be null");
    }

    // Use default sampler if none provided
    finevk::Sampler* actualSampler = sampler ? sampler : fontSampler_.get();

    uint64_t id = nextTextureId_++;

    TextureEntry entry;
    entry.texture = texture;
    entry.sampler = actualSampler;
    entry.descriptorSet = allocateTextureDescriptor(texture, actualSampler);

    textures_[id] = entry;
    return id;
}

void ImGuiBackend::unregisterTexture(uint64_t textureId) {
    auto it = textures_.find(textureId);
    if (it != textures_.end()) {
        descriptorPool_->free(it->second.descriptorSet);
        textures_.erase(it);
    }
}

VkDescriptorSet ImGuiBackend::getTextureDescriptor(uint64_t textureId) {
    if (textureId == 0) {
        return fontDescriptorSet_;
    }

    auto it = textures_.find(textureId);
    if (it != textures_.end()) {
        return it->second.descriptorSet;
    }

    // Fall back to font texture if not found
    return fontDescriptorSet_;
}

VkDescriptorSet ImGuiBackend::allocateTextureDescriptor(finevk::Texture* texture,
                                                         finevk::Sampler* sampler)
{
    VkDescriptorSet set = descriptorPool_->allocate(descriptorSetLayout_.get());

    finevk::DescriptorWriter(device_)
        .writeImage(set, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    texture->view()->handle(), sampler->handle(),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        .update();

    return set;
}

// ============================================================================
// Buffer management
// ============================================================================

void ImGuiBackend::ensureBufferCapacity(uint32_t frameIndex,
                                         size_t vertexCount,
                                         size_t indexCount)
{
    auto& frame = frameData_[frameIndex];

    // Resize vertex buffer if needed
    if (vertexCount > frame.vertexCapacity) {
        frame.vertexCapacity = vertexCount + 5000;  // Growth buffer
        VkDeviceSize size = frame.vertexCapacity * sizeof(ImDrawVert);

        frame.vertexBuffer = finevk::Buffer::createVertexBuffer(
            device_, size,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    // Resize index buffer if needed
    if (indexCount > frame.indexCapacity) {
        frame.indexCapacity = indexCount + 10000;  // Growth buffer
        VkDeviceSize size = frame.indexCapacity * sizeof(ImDrawIdx);

        frame.indexBuffer = finevk::Buffer::createIndexBuffer(
            device_, size,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
}

// ============================================================================
// Rendering
// ============================================================================

void ImGuiBackend::render(finevk::CommandBuffer& cmd, uint32_t frameIndex) {
    ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData || drawData->TotalVtxCount == 0) {
        return;
    }

    // Ensure buffers are large enough
    ensureBufferCapacity(frameIndex,
                         static_cast<size_t>(drawData->TotalVtxCount),
                         static_cast<size_t>(drawData->TotalIdxCount));

    auto& frame = frameData_[frameIndex];

    // Upload vertex/index data
    ImDrawVert* vtxDst = static_cast<ImDrawVert*>(frame.vertexBuffer->mappedPtr());
    ImDrawIdx* idxDst = static_cast<ImDrawIdx*>(frame.indexBuffer->mappedPtr());

    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList* cmdList = drawData->CmdLists[n];
        std::memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        std::memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtxDst += cmdList->VtxBuffer.Size;
        idxDst += cmdList->IdxBuffer.Size;
    }

    // Bind pipeline
    cmd.bindPipeline(pipeline_.get());

    // Set viewport
    cmd.setViewport(0, 0,
                    drawData->DisplaySize.x * drawData->FramebufferScale.x,
                    drawData->DisplaySize.y * drawData->FramebufferScale.y);

    // Setup push constants
    PushConstantBlock pushConstants;
    pushConstants.scale[0] = 2.0f / drawData->DisplaySize.x;
    pushConstants.scale[1] = 2.0f / drawData->DisplaySize.y;
    pushConstants.translate[0] = -1.0f - drawData->DisplayPos.x * pushConstants.scale[0];
    pushConstants.translate[1] = -1.0f - drawData->DisplayPos.y * pushConstants.scale[1];

    cmd.pushConstants(pipelineLayout_->handle(),
                      VK_SHADER_STAGE_VERTEX_BIT,
                      0, sizeof(PushConstantBlock), &pushConstants);

    // Bind vertex/index buffers
    cmd.bindVertexBuffer(*frame.vertexBuffer);
    cmd.bindIndexBuffer(*frame.indexBuffer,
                        sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

    // Render command lists
    int globalVtxOffset = 0;
    int globalIdxOffset = 0;

    ImVec2 clipOff = drawData->DisplayPos;
    ImVec2 clipScale = drawData->FramebufferScale;

    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList* cmdList = drawData->CmdLists[n];

        for (int cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; cmdIdx++) {
            const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmdIdx];

            if (pcmd->UserCallback != nullptr) {
                // User callback (not commonly used, but supported)
                if (pcmd->UserCallback != ImDrawCallback_ResetRenderState) {
                    pcmd->UserCallback(cmdList, pcmd);
                }
            } else {
                // Calculate scissor rect
                ImVec2 clipMin((pcmd->ClipRect.x - clipOff.x) * clipScale.x,
                               (pcmd->ClipRect.y - clipOff.y) * clipScale.y);
                ImVec2 clipMax((pcmd->ClipRect.z - clipOff.x) * clipScale.x,
                               (pcmd->ClipRect.w - clipOff.y) * clipScale.y);

                // Clamp to viewport
                if (clipMin.x < 0.0f) clipMin.x = 0.0f;
                if (clipMin.y < 0.0f) clipMin.y = 0.0f;

                float fbWidth = drawData->DisplaySize.x * clipScale.x;
                float fbHeight = drawData->DisplaySize.y * clipScale.y;
                if (clipMax.x > fbWidth) clipMax.x = fbWidth;
                if (clipMax.y > fbHeight) clipMax.y = fbHeight;

                if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y) {
                    continue;
                }

                // Set scissor
                cmd.setScissor(
                    static_cast<int32_t>(clipMin.x),
                    static_cast<int32_t>(clipMin.y),
                    static_cast<uint32_t>(clipMax.x - clipMin.x),
                    static_cast<uint32_t>(clipMax.y - clipMin.y));

                // Get texture descriptor
                uint64_t texId = reinterpret_cast<uint64_t>(pcmd->GetTexID());
                VkDescriptorSet texDescriptor = getTextureDescriptor(texId);

                // Bind descriptor set
                cmd.bindDescriptorSet(*pipelineLayout_, texDescriptor, 0);

                // Draw
                cmd.drawIndexed(pcmd->ElemCount,
                                1,
                                pcmd->IdxOffset + globalIdxOffset,
                                pcmd->VtxOffset + globalVtxOffset,
                                0);
            }
        }

        globalVtxOffset += cmdList->VtxBuffer.Size;
        globalIdxOffset += cmdList->IdxBuffer.Size;
    }
}

void ImGuiBackend::renderDrawData(finevk::CommandBuffer& cmd, uint32_t frameIndex,
                                   const GuiDrawData& data)
{
    if (data.empty()) {
        return;
    }

    // Ensure buffers are large enough
    ensureBufferCapacity(frameIndex, data.vertices.size(), data.indices.size());

    auto& frame = frameData_[frameIndex];

    // Upload captured vertex/index data
    std::memcpy(frame.vertexBuffer->mappedPtr(),
                data.vertices.data(),
                data.vertices.size() * sizeof(ImDrawVert));
    std::memcpy(frame.indexBuffer->mappedPtr(),
                data.indices.data(),
                data.indices.size() * sizeof(ImDrawIdx));

    // Bind pipeline
    cmd.bindPipeline(pipeline_.get());

    // Set viewport
    cmd.setViewport(0, 0,
                    data.displaySize.x * data.framebufferScale.x,
                    data.displaySize.y * data.framebufferScale.y);

    // Setup push constants
    PushConstantBlock pushConstants;
    pushConstants.scale[0] = 2.0f / data.displaySize.x;
    pushConstants.scale[1] = 2.0f / data.displaySize.y;
    pushConstants.translate[0] = -1.0f;
    pushConstants.translate[1] = -1.0f;

    cmd.pushConstants(pipelineLayout_->handle(),
                      VK_SHADER_STAGE_VERTEX_BIT,
                      0, sizeof(PushConstantBlock), &pushConstants);

    // Bind vertex/index buffers
    cmd.bindVertexBuffer(*frame.vertexBuffer);
    cmd.bindIndexBuffer(*frame.indexBuffer,
                        sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

    // Render captured commands
    float fbWidth = data.displaySize.x * data.framebufferScale.x;
    float fbHeight = data.displaySize.y * data.framebufferScale.y;

    for (const auto& drawCmd : data.commands) {
        // Calculate scissor from clip rect
        float clipMinX = static_cast<float>(drawCmd.scissorRect.x) * data.framebufferScale.x;
        float clipMinY = static_cast<float>(drawCmd.scissorRect.y) * data.framebufferScale.y;
        float clipMaxX = clipMinX + static_cast<float>(drawCmd.scissorRect.z) * data.framebufferScale.x;
        float clipMaxY = clipMinY + static_cast<float>(drawCmd.scissorRect.w) * data.framebufferScale.y;

        // Clamp to viewport
        if (clipMinX < 0.0f) clipMinX = 0.0f;
        if (clipMinY < 0.0f) clipMinY = 0.0f;
        if (clipMaxX > fbWidth) clipMaxX = fbWidth;
        if (clipMaxY > fbHeight) clipMaxY = fbHeight;

        if (clipMaxX <= clipMinX || clipMaxY <= clipMinY) {
            continue;
        }

        // Set scissor
        cmd.setScissor(
            static_cast<int32_t>(clipMinX),
            static_cast<int32_t>(clipMinY),
            static_cast<uint32_t>(clipMaxX - clipMinX),
            static_cast<uint32_t>(clipMaxY - clipMinY));

        // Get texture descriptor
        VkDescriptorSet texDescriptor = getTextureDescriptor(drawCmd.texture.id);

        // Bind descriptor set
        cmd.bindDescriptorSet(*pipelineLayout_, texDescriptor, 0);

        // Draw
        cmd.drawIndexed(drawCmd.indexCount,
                        1,
                        drawCmd.indexOffset,
                        drawCmd.vertexOffset,
                        0);
    }
}

void ImGuiBackend::rebuildFontTexture() {
    if (!commandPool_) {
        throw std::runtime_error("ImGuiBackend::rebuildFontTexture: not initialized");
    }

    // Wait for GPU to finish using current font texture
    device_->waitIdle();

    // Free old font descriptor
    if (fontDescriptorSet_ != VK_NULL_HANDLE) {
        descriptorPool_->free(fontDescriptorSet_);
        fontDescriptorSet_ = VK_NULL_HANDLE;
    }

    // Get new font atlas data from ImGui
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Create new texture
    fontTexture_ = finevk::Texture::fromMemory(
        device_,
        pixels,
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        commandPool_,
        false,  // No mipmaps
        false   // Not sRGB
    );

    // Allocate new descriptor set
    fontDescriptorSet_ = allocateTextureDescriptor(fontTexture_.get(), fontSampler_.get());

    // Update ImGui's texture ID
    io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(static_cast<uint64_t>(0)));
}

} // namespace backend
} // namespace finegui
