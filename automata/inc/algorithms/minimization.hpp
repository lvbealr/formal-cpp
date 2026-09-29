#ifndef AUTOMATA_ALGORITHMS_MINIMIZATION_HPP
#define AUTOMATA_ALGORITHMS_MINIMIZATION_HPP

#include "algorithms/reports.hpp"
#include "core/models.hpp"

namespace automata {

[[nodiscard]] DFA remove_unreachable(const DFA& dfa);

[[nodiscard]] MinimizationResult minimize_with_report(const DFA& dfa);

[[nodiscard]] DFA minimize(const DFA& dfa);

[[nodiscard]] DFA minimize_brzozowski(const NFA& nfa);

}

#endif
