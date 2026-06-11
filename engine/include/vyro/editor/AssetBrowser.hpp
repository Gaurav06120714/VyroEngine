// VyroEngine — Asset browser model
// Phase 8.4: a filesystem-backed listing of project assets with type
// classification and filtering. The asset browser panel renders this model.
#pragma once

#include "vyro/core/Types.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace vyro {

enum class AssetType : u8 {
    Model,
    Texture,
    Audio,
    Script,
    Scene,
    Other,
};

struct AssetEntry {
    std::string name;
    std::string path;
    AssetType type = AssetType::Other;
};

class AssetBrowser
{
public:
    // Classify by file extension (.obj/.gltf, .png/.jpg, .wav/.ogg, .vs/.lua…).
    [[nodiscard]] static AssetType classify(std::string_view filename);

    // Scan a directory (recursively) into the entry list. Returns entry count.
    usize scan(std::string_view directory);

    [[nodiscard]] const std::vector<AssetEntry>& entries() const { return m_entries; }

    // Entries of one type (e.g. the Models tab).
    [[nodiscard]] std::vector<AssetEntry> filter(AssetType type) const;

private:
    std::vector<AssetEntry> m_entries;
};

} // namespace vyro
