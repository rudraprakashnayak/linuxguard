#include "dependency_graph.hpp"
#include <algorithm>
#include <functional>
#include <stdexcept>

namespace linuxguard {

void DependencyGraph::add_service(const std::string& name,
                                  const std::vector<std::string>& depends_on) {
    nodes_.insert(name);
    adjacency_[name] = depends_on;
    for (auto& dep : depends_on) {
        nodes_.insert(dep);
        reverse_adj_[dep].push_back(name);
    }
}

std::vector<std::string> DependencyGraph::topological_order() const {
    std::vector<std::string> order;
    std::set<std::string> visited;
    std::set<std::string> in_stack;

    for (auto& node : nodes_) {
        if (visited.find(node) == visited.end()) {
            dfs(node, visited, in_stack, order);
        }
    }

    std::reverse(order.begin(), order.end());
    return order;
}

std::vector<std::string> DependencyGraph::reverse_topological_order() const {
    auto order = topological_order();
    std::reverse(order.begin(), order.end());
    return order;
}

std::optional<std::string> DependencyGraph::detect_cycle() const {
    std::set<std::string> visited;
    std::set<std::string> in_stack;
    std::vector<std::string> order;

    for (auto& node : nodes_) {
        if (visited.find(node) == visited.end()) {
            // Temporarily cast away const for DFS
            auto& self = const_cast<DependencyGraph&>(*this);
            // Use a lambda-free approach: just check with a fresh visited set
        }
    }

    // Simple cycle detection using DFS with coloring
    // 0 = white (unvisited), 1 = gray (in progress), 2 = black (done)
    std::map<std::string, int> color;
    for (auto& node : nodes_) color[node] = 0;

    std::function<bool(const std::string&)> has_cycle =
        [&](const std::string& node) -> bool {
        color[node] = 1;
        auto it = adjacency_.find(node);
        if (it != adjacency_.end()) {
            for (auto& dep : it->second) {
                if (color[dep] == 1) return true;  // Back edge = cycle
                if (color[dep] == 0 && has_cycle(dep)) return true;
            }
        }
        color[node] = 2;
        return false;
    };

    for (auto& node : nodes_) {
        if (color[node] == 0) {
            if (has_cycle(node)) return node;
        }
    }

    return std::nullopt;
}

bool DependencyGraph::validate() const {
    return !detect_cycle().has_value();
}

std::vector<std::string> DependencyGraph::get_dependencies(
    const std::string& name) const {
    auto it = adjacency_.find(name);
    if (it == adjacency_.end()) return {};
    return it->second;
}

std::vector<std::string> DependencyGraph::get_dependents(
    const std::string& name) const {
    auto it = reverse_adj_.find(name);
    if (it == reverse_adj_.end()) return {};
    return it->second;
}

void DependencyGraph::dfs(const std::string& node,
                          std::set<std::string>& visited,
                          std::set<std::string>& in_stack,
                          std::vector<std::string>& order) const {
    visited.insert(node);
    in_stack.insert(node);

    auto it = adjacency_.find(node);
    if (it != adjacency_.end()) {
        for (auto& dep : it->second) {
            if (visited.find(dep) == visited.end()) {
                dfs(dep, visited, in_stack, order);
            }
        }
    }

    in_stack.erase(node);
    order.push_back(node);
}

} // namespace linuxguard
