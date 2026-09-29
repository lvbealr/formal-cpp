#ifndef AUTOMATA_ALGORITHMS_CONVERSIONS_HPP
#define AUTOMATA_ALGORITHMS_CONVERSIONS_HPP

#include "algorithms/reports.hpp"
#include "core/models.hpp"
#include "regex/regex.hpp"

namespace automata {

[[nodiscard]] NFA dfa_to_nfa(const DFA& dfa);

[[nodiscard]] NFA remove_epsilon(const NFA& nfa);

[[nodiscard]] DeterminizationResult determinize_with_report(const NFA& nfa);

[[nodiscard]] DFA determinize(const NFA& nfa);

[[nodiscard]] DFA complete(const DFA& dfa);

[[nodiscard]] DFA complement(const DFA& dfa);

[[nodiscard]] NFA reverse(const NFA& nfa);

[[nodiscard]] NFA regex_to_nfa(const Regex& expr, const Alphabet& alphabet);

[[nodiscard]] Regex nfa_to_regex(const NFA& nfa);

}

#endif
