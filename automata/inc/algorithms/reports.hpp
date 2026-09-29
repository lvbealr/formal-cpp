#ifndef AUTOMATA_ALGORITHMS_REPORTS_HPP
#define AUTOMATA_ALGORITHMS_REPORTS_HPP

#include "core/models.hpp"

#include <map>
#include <vector>

namespace automata {

struct DeterminizationResult {
  DFA dfa;
  std::map<State, StateSet> subsets;
};

struct MinimizationResult {
  DFA dfa;
  DFA prepared;

  std::vector<std::vector<StateSet>> history;

  std::map<State, State> mapping;
};

}

#endif
