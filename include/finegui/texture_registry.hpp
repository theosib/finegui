#pragma once

#include "texture_handle.hpp"
#include <string>
#include <unordered_map>

namespace finegui {

/// Registry mapping string names to TextureHandle values.
///
/// C++ code registers textures by name; scripts reference them by the same
/// name.  MapRenderer uses this registry to resolve texture name strings
/// into ImTextureID values for ImGui::Image().
///
/// Usage:
///   TextureRegistry registry;
///   registry.registerTexture("sword_icon", swordHandle);
///   // In script: ui.image "sword_icon" 48 48
class TextureRegistry {
public:
    /// Register (or replace) a texture by name.
    void registerTexture(const std::string& name, TextureHandle handle);

    /// Remove a texture by name.
    void unregisterTexture(const std::string& name);

    /// Look up a texture by name.  Returns an invalid handle if not found.
    TextureHandle get(const std::string& name) const;

    /// Check whether a texture name is registered.
    bool has(const std::string& name) const;

    /// Remove all registered textures.
    void clear();

    /// Return the number of registered textures.
    size_t size() const { return textures_.size(); }

private:
    std::unordered_map<std::string, TextureHandle> textures_;
};

} // namespace finegui
