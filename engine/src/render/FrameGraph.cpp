// VyroEngine — Frame graph implementation
#include "vyro/render/FrameGraph.hpp"

#include <algorithm>

namespace vyro {

FrameGraph::PassId FrameGraph::add_pass(std::string name, std::vector<std::string> reads,
                                        std::vector<std::string> writes)
{
    m_passes.push_back(Pass{std::move(name), std::move(reads), std::move(writes)});
    return static_cast<PassId>(m_passes.size() - 1);
}

std::expected<std::vector<FrameGraph::PassId>, FrameGraphError> FrameGraph::compile() const
{
    const usize n = m_passes.size();

    // Edge u -> v when u writes a resource v reads.
    std::vector<std::vector<usize>> edges(n);
    std::vector<u32> in_degree(n, 0);
    for (usize u = 0; u < n; ++u) {
        for (usize v = 0; v < n; ++v) {
            if (u == v) {
                continue;
            }
            for (const std::string& w : m_passes[u].writes) {
                if (std::find(m_passes[v].reads.begin(), m_passes[v].reads.end(), w)
                    != m_passes[v].reads.end()) {
                    edges[u].push_back(v);
                    ++in_degree[v];
                    break;
                }
            }
        }
    }

    // Kahn's algorithm, preferring declaration order for stable output.
    std::vector<PassId> order;
    std::vector<usize> ready;
    for (usize i = 0; i < n; ++i) {
        if (in_degree[i] == 0) {
            ready.push_back(i);
        }
    }
    while (!ready.empty()) {
        const usize u = ready.front();
        ready.erase(ready.begin());
        order.push_back(static_cast<PassId>(u));
        for (const usize v : edges[u]) {
            if (--in_degree[v] == 0) {
                ready.push_back(v);
            }
        }
    }

    if (order.size() != n) {
        return std::unexpected(FrameGraphError::Cycle);
    }
    return order;
}

} // namespace vyro
