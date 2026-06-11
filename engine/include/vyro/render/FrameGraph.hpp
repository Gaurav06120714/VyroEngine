// VyroEngine — Frame graph
// Phase 10.1: render passes declare the resources they read and write; the
// graph derives a dependency-correct execution order. This is the scheduling
// core an explicit-API (Vulkan) backend records command buffers from.
#pragma once

#include "vyro/core/Types.hpp"

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace vyro {

enum class FrameGraphError {
    Cycle,
};

class FrameGraph
{
public:
    using PassId = u32;

    PassId add_pass(std::string name,
                    std::vector<std::string> reads,
                    std::vector<std::string> writes);

    // Topologically order passes so every writer runs before its readers.
    [[nodiscard]] std::expected<std::vector<PassId>, FrameGraphError> compile() const;

    [[nodiscard]] std::string_view pass_name(PassId id) const { return m_passes[id].name; }
    [[nodiscard]] usize pass_count() const { return m_passes.size(); }

private:
    struct Pass {
        std::string name;
        std::vector<std::string> reads;
        std::vector<std::string> writes;
    };

    std::vector<Pass> m_passes;
};

} // namespace vyro
