#ifndef AUTOMATA_CORE_GRAPH_HPP
#define AUTOMATA_CORE_GRAPH_HPP

#include "core/models.hpp"

#include <unordered_map>

namespace automata {

using Graph = std::unordered_map<State, StateSet>;

struct DirectedGraphs {
  Graph forward;
  Graph backward;
};

[[nodiscard]] DirectedGraphs graphs(const NFA& nfa);

[[nodiscard]] StateSet reachable(const Graph& graph, const StateSet& sources);

}

#endif
