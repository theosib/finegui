#include <finegui/texture_registry.hpp>

namespace finegui {

void TextureRegistry::registerTexture(const std::string& name, TextureHandle handle) {
    textures_[name] = handle;
}

void TextureRegistry::unregisterTexture(const std::string& name) {
    textures_.erase(name);
}

TextureHandle TextureRegistry::get(const std::string& name) const {
    auto it = textures_.find(name);
    if (it != textures_.end()) {
        return it->second;
    }
    return TextureHandle{};
}

bool TextureRegistry::has(const std::string& name) const {
    return textures_.find(name) != textures_.end();
}

void TextureRegistry::clear() {
    textures_.clear();
}

} // namespace finegui
