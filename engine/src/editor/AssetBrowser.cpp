// VyroEngine — Asset browser implementation
#include "vyro/editor/AssetBrowser.hpp"

#include <algorithm>
#include <filesystem>

namespace vyro {

AssetType AssetBrowser::classify(std::string_view filename)
{
    const auto dot = filename.find_last_of('.');
    if (dot == std::string_view::npos) {
        return AssetType::Other;
    }
    std::string ext(filename.substr(dot + 1));
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (ext == "obj" || ext == "gltf" || ext == "glb" || ext == "fbx") {
        return AssetType::Model;
    }
    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "ktx" || ext == "bmp") {
        return AssetType::Texture;
    }
    if (ext == "wav" || ext == "ogg" || ext == "mp3" || ext == "flac") {
        return AssetType::Audio;
    }
    if (ext == "vs" || ext == "lua") {
        return AssetType::Script;
    }
    if (ext == "scene" || ext == "vyro") {
        return AssetType::Scene;
    }
    return AssetType::Other;
}

usize AssetBrowser::scan(std::string_view directory)
{
    m_entries.clear();
    std::error_code ec;
    const std::filesystem::path root{std::string(directory)};
    if (!std::filesystem::exists(root, ec)) {
        return 0;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root, ec)) {
        if (!entry.is_regular_file(ec)) {
            continue;
        }
        AssetEntry e;
        e.name = entry.path().filename().string();
        e.path = entry.path().string();
        e.type = classify(e.name);
        m_entries.push_back(std::move(e));
    }
    std::sort(m_entries.begin(), m_entries.end(),
              [](const AssetEntry& a, const AssetEntry& b) { return a.name < b.name; });
    return m_entries.size();
}

std::vector<AssetEntry> AssetBrowser::filter(AssetType type) const
{
    std::vector<AssetEntry> out;
    for (const AssetEntry& e : m_entries) {
        if (e.type == type) {
            out.push_back(e);
        }
    }
    return out;
}

} // namespace vyro
