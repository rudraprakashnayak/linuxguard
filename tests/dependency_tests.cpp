#include <cassert>
#include <iostream>

#include "../src/dependency/dependency_graph.hpp"

using namespace linuxguard;

void test_topological_order() {
    std::cout << "Test: topological order... ";

    DependencyGraph graph;
    graph.add_service("A", {});
    graph.add_service("B", {"A"});
    graph.add_service("C", {"A", "B"});

    auto order = graph.topological_order();
    assert(order.size() == 3);

    // A must come before B, B must come before C
    auto pos_a = std::find(order.begin(), order.end(), "A") - order.begin();
    auto pos_b = std::find(order.begin(), order.end(), "B") - order.begin();
    auto pos_c = std::find(order.begin(), order.end(), "C") - order.begin();

    assert(pos_a < pos_b);
    assert(pos_b < pos_c);
    std::cout << "PASSED\n";
}

void test_no_dependencies() {
    std::cout << "Test: no dependencies... ";

    DependencyGraph graph;
    graph.add_service("X", {});
    graph.add_service("Y", {});
    graph.add_service("Z", {});

    auto order = graph.topological_order();
    assert(order.size() == 3);
    std::cout << "PASSED\n";
}

void test_cycle_detection() {
    std::cout << "Test: cycle detection... ";

    DependencyGraph graph;
    graph.add_service("A", {"C"});
    graph.add_service("B", {"A"});
    graph.add_service("C", {"B"});

    auto cycle = graph.detect_cycle();
    assert(cycle.has_value());
    assert(!graph.validate());
    std::cout << "PASSED\n";
}

void test_no_cycle() {
    std::cout << "Test: no cycle... ";

    DependencyGraph graph;
    graph.add_service("A", {});
    graph.add_service("B", {"A"});
    graph.add_service("C", {"B"});

    auto cycle = graph.detect_cycle();
    assert(!cycle.has_value());
    assert(graph.validate());
    std::cout << "PASSED\n";
}

void test_reverse_order() {
    std::cout << "Test: reverse topological order... ";

    DependencyGraph graph;
    graph.add_service("A", {});
    graph.add_service("B", {"A"});
    graph.add_service("C", {"B"});

    auto order = graph.topological_order();
    auto reverse = graph.reverse_topological_order();

    assert(order.size() == reverse.size());
    assert(reverse[0] == order.back());
    assert(reverse.back() == order.front());
    std::cout << "PASSED\n";
}

void test_get_dependencies() {
    std::cout << "Test: get dependencies... ";

    DependencyGraph graph;
    graph.add_service("A", {});
    graph.add_service("B", {"A"});
    graph.add_service("C", {"A", "B"});

    auto deps_a = graph.get_dependencies("A");
    assert(deps_a.empty());

    auto deps_b = graph.get_dependencies("B");
    assert(deps_b.size() == 1);
    assert(deps_b[0] == "A");

    auto deps_c = graph.get_dependencies("C");
    assert(deps_c.size() == 2);
    std::cout << "PASSED\n";
}

void test_get_dependents() {
    std::cout << "Test: get dependents... ";

    DependencyGraph graph;
    graph.add_service("A", {});
    graph.add_service("B", {"A"});
    graph.add_service("C", {"A"});

    auto dependents = graph.get_dependents("A");
    assert(dependents.size() == 2);
    std::cout << "PASSED\n";
}

void test_empty_graph() {
    std::cout << "Test: empty graph... ";

    DependencyGraph graph;
    assert(graph.empty());

    auto order = graph.topological_order();
    assert(order.empty());

    auto cycle = graph.detect_cycle();
    assert(!cycle.has_value());
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== Dependency Graph Tests ===\n";

    test_topological_order();
    test_no_dependencies();
    test_cycle_detection();
    test_no_cycle();
    test_reverse_order();
    test_get_dependencies();
    test_get_dependents();
    test_empty_graph();

    std::cout << "\nAll dependency graph tests passed!\n";
    return 0;
}
