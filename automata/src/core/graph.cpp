#include "core/graph.hpp"

#include <vector>

namespace automata {

[[nodiscard]] DirectedGraphs graphs(const NFA& nfa) {
  DirectedGraphs result;

  for (const auto& [key, targets] : nfa.transitions()) {
    const auto& [source, symbol] = key;
    (void)symbol;

    for (const State target : targets) {
      result.forward[source].insert(target);
      result.backward[target].insert(source);
    }
  }

  return result;
}

[[nodiscard]] StateSet reachable(const Graph& graph, const StateSet& sources) {
  StateSet visited = sources;

  std::vector<State> stack{sources.begin(), sources.end()};

  while (!stack.empty()) {
    const State state = stack.back();
    stack.pop_back();

    const auto it = graph.find(state);

    if (it == graph.end()) {
      continue;
    }

    for (const State target : it->second) {
      if (visited.insert(target).second) {
        stack.push_back(target);
      }
    }
  }

  return visited;
}

}
