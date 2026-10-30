#pragma once

/**
 * @file skin_loader.hpp
 * @brief Build a Skin from a .fgs definition: a finescript file that evaluates to a map
 */

#include "finegui/skin.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace finescript {
class ScriptEngine;
class Value;
class ExecutionContext;
}

namespace finegui {

class GuiSystem;

/// What a .fgs names, as read from it. Separate from anything touching the GPU.
struct SkinDefinition {
    Skin skin;
    std::string atlasFile;  ///< As written in the definition
    std::string fontFile;   ///< Empty for ImGui's own
    float fontSize = 16.0f;
};

/// @param error Set to why, when this returns nullopt
[[nodiscard]] std::optional<SkinDefinition> readSkinDefinition(const finescript::Value& definition,
                                                               finescript::ScriptEngine& engine,
                                                               std::string& error);

/// Load a .fgs and the atlas and font it names, which are looked for beside it. The GuiSystem
/// keeps the atlas registered for as long as it lives; the caller keeps the Skin.
[[nodiscard]] std::unique_ptr<Skin> loadSkin(GuiSystem& gui, const std::filesystem::path& path,
                                             finescript::ScriptEngine& engine,
                                             finescript::ExecutionContext& context,
                                             std::string& error);

}  // namespace finegui
