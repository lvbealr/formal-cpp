#include "io/serialization.hpp"

namespace automata {
namespace {
json drawing_json(const DrawingOptions& drawing) {
  json labels = json::object();
  json positions = json::object();
  json groups = json::array();
  for (const auto& [state, label] : drawing.labels) {
    labels[std::to_string(state)] = label;
  }
  for (const auto& [state, point] : drawing.positions) {
    positions[std::to_string(state)] = point;
  }
  for (const auto& group : drawing.groups) {
    groups.push_back(export_detail::sorted_states(group));
  }
  return {{"title", drawing.title}, {"notes", drawing.notes},
      {"labels", labels}, {"positions", positions}, {"groups", groups},
      {"layout", drawing.layout}};
}

template <typename Automaton>
json common_json(const Automaton& automaton, const DrawingOptions& drawing) {
  json alphabet = json::array();
  for (const Symbol symbol :
      export_detail::sorted_symbols(automaton.alphabet())) {
    alphabet.push_back(std::string{symbol});
  }
  return {{"schema_version", 1},
      {"states", export_detail::sorted_states(automaton.states())},
      {"alphabet", alphabet},
      {"final_states", export_detail::sorted_states(automaton.final_states())},
      {"transitions", json::array()}, {"drawing", drawing_json(drawing)},
      {"table", format_transition_table(automaton)}};
}
}

json to_json(const NFA& nfa, const DrawingOptions& drawing) {
  json result = common_json(nfa, drawing);
  result["type"] = "nfa";
  result["initial_states"] = export_detail::sorted_states(nfa.initial_states());
  for (const auto& [key, targets] : nfa.transitions()) {
    const auto& [source, symbol] = key;
    result["transitions"].push_back({{"source", source},
        {"symbol", symbol ? json(std::string{*symbol}) : json(nullptr)},
        {"targets", export_detail::sorted_states(targets)}});
  }
  return result;
}

json to_json(const DFA& dfa, const DrawingOptions& drawing) {
  json result = common_json(dfa, drawing);
  result["type"] = "dfa";
  result["initial_state"] = dfa.initial_state();
  for (const auto& [key, target] : dfa.transitions()) {
    const auto& [source, symbol] = key;
    result["transitions"].push_back({{"source", source},
        {"symbol", std::string{symbol}}, {"target", target}});
  }
  return result;
}

void write_json(const json& document, const std::filesystem::path& filename) {
  export_detail::write_text_file(filename, document.dump(2) + '\n');
}
}
