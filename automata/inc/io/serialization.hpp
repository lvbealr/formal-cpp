#ifndef AUTOMATA_IO_SERIALIZATION_HPP
#define AUTOMATA_IO_SERIALIZATION_HPP

#include "io/export.hpp"

#include <array>
#include <nlohmann/json.hpp>

namespace automata {
using json = nlohmann::json;

struct DrawingOptions {
  std::string title;
  std::string notes;
  StateLabels labels;
  std::vector<StateSet> groups;
  std::map<State, std::array<double, 2>> positions;
  std::string layout = "layers";
};

[[nodiscard]] json to_json(const NFA& nfa, const DrawingOptions& drawing = {});
[[nodiscard]] json to_json(const DFA& dfa, const DrawingOptions& drawing = {});
void write_json(const json& document, const std::filesystem::path& filename);

template <typename Automaton>
void save_json(const Automaton& automaton,
    const std::filesystem::path& filename, const DrawingOptions& drawing = {}) {
  write_json(to_json(automaton, drawing), filename);
}
}
#endif
