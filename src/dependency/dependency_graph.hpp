#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <optional>

namespace linuxguard {

class DependencyGraph {
public:
    void add_service(const std::string& name,
                     const std::vector<std::string>& depends_on);

    std::vector<std::string> topological_order() const;
    std::vector<std::string> reverse_topological_order() const;

    std::optional<std::string> detect_cycle() const;
    bool validate() const;

    std::vector<std::string> get_dependencies(const std::string& name) const;
    std::vector<std::string> get_dependents(const std::string& name) const;

    bool empty() const { return adjacency_.empty(); }

private:
    std::map<std::string, std::vector<std::string>> adjacency_;   // name -> deps
    std::map<std::string, std::vector<std::string>> reverse_adj_; // name -> dependents
    std::set<std::string> nodes_;

    void dfs(const std::string& node,
             std::set<std::string>& visited,
             std::set<std::string>& in_stack,
             std::vector<std::string>& order) const;
};

} // namespace linuxguard
