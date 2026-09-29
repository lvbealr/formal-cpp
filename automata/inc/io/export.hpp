#ifndef AUTOMATA_IO_EXPORT_HPP
#define AUTOMATA_IO_EXPORT_HPP

#include "algorithms/reports.hpp"
#include "core/models.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace automata {

using StateLabels = std::map<State, std::string>;

namespace export_detail {

[[nodiscard]] std::vector<State> sorted_states(const StateSet& states);

[[nodiscard]] std::vector<Symbol> sorted_symbols(const Alphabet& alphabet);

[[nodiscard]] std::string dot_quote(const std::string_view value);

[[nodiscard]] std::string states_text(const StateSet& states);

void write_text_file(
    const std::filesystem::path& path, const std::string_view text);

}

[[nodiscard]] std::string to_dot(
    const DFA& dfa, const StateLabels& labels = {});

[[nodiscard]] std::string to_dot(
    const NFA& nfa, const StateLabels& labels = {});

void write_dot(const std::filesystem::path& path, const DFA& dfa,
    const StateLabels& labels = {});

void write_dot(const std::filesystem::path& path, const NFA& nfa,
    const StateLabels& labels = {});

[[nodiscard]] std::string format_transition_table(const DFA& dfa);

[[nodiscard]] std::string format_transition_table(const NFA& nfa);

[[nodiscard]] std::string format_determinization_report(
    const DeterminizationResult& result);

[[nodiscard]] std::string format_minimization_report(
    const MinimizationResult& result);

}

#endif
