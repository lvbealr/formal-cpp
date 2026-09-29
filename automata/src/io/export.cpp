#include "io/export.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace automata {

using StateLabels = std::map<State, std::string>;

namespace export_detail {

std::string table_symbol(Symbol symbol) {
  switch (symbol) {
    case '|':
      return "\\|";
    case '\\':
      return "\\\\";
    case '\n':
      return "\\n";
    case '\r':
      return "\\r";
    case '\t':
      return "\\t";
    default:
      return std::string{symbol};
  }
}

[[nodiscard]] std::vector<State> sorted_states(const StateSet& states) {
  std::vector<State> result{states.begin(), states.end()};

  std::ranges::sort(result);
  return result;
}

[[nodiscard]] std::vector<Symbol> sorted_symbols(const Alphabet& alphabet) {
  std::vector<Symbol> result{alphabet.begin(), alphabet.end()};

  std::ranges::sort(result);
  return result;
}

[[nodiscard]] std::string dot_quote(const std::string_view value) {
  std::string result{"\""};

  for (const char c : value) {
    switch (c) {
      case '\\':
        result += "\\\\";
        break;

      case '"':
        result += "\\\"";
        break;

      case '\n':
        result += "\\n";
        break;

      case '\r':
        result += "\\r";
        break;

      default:
        result += c;
        break;
    }
  }

  result += '"';

  return result;
}

[[nodiscard]] std::string states_text(const StateSet& states) {
  if (states.empty()) {
    return "∅";
  }

  const auto ordered = sorted_states(states);

  std::ostringstream out;

  out << '{';

  for (std::size_t i = 0; i < ordered.size(); ++i) {
    if (i != 0) {
      out << ", ";
    }

    out << ordered[i];
  }

  out << '}';

  return out.str();
}

void write_text_file(
    const std::filesystem::path& path, const std::string_view text) {
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }

  std::ofstream output{path};

  if (!output) {
    throw std::runtime_error{"Cannot open file: " + path.string()};
  }

  output << text;

  if (!output) {
    throw std::runtime_error{"Cannot write file: " + path.string()};
  }
}

}

[[nodiscard]] std::string to_dot(const DFA& dfa, const StateLabels& labels) {
  std::ostringstream out;

  out << R"(digraph automaton {
  rankdir=LR;
  graph [
    bgcolor="white",
    pad="0.3",
    nodesep="0.55",
    ranksep="0.75"
  ];

  node [
    shape=circle,
    style=filled,
    fillcolor="#f8fafc",
    color="#334155",
    penwidth=1.5,
    fontname="DejaVu Sans"
  ];

  edge [
    color="#64748b",
    fontcolor="#334155",
    penwidth=1.3,
    arrowsize=0.8,
    fontname="DejaVu Sans"
  ];
)";

  for (const State state : export_detail::sorted_states(dfa.states())) {
    const auto label_it = labels.find(state);

    const std::string label =
        label_it == labels.end() ? std::to_string(state) : label_it->second;

    out << "  " << export_detail::dot_quote("q_" + std::to_string(state))
        << " [shape="
        << (dfa.final_states().contains(state) ? "doublecircle" : "circle")
        << ", label=" << export_detail::dot_quote(label) << "];\n";
  }

  out << R"(
  __start [
    shape=point,
    label="",
    color="#2563eb"
  ];
)";

  out << "  __start -> "
      << export_detail::dot_quote("q_" + std::to_string(dfa.initial_state()))
      << " [color=\"#2563eb\", penwidth=2];\n";

  std::map<std::pair<State, State>, std::vector<std::string> > edges;

  for (const auto& [key, target] : dfa.transitions()) {
    const auto [source, symbol] = key;

    edges[{source, target}].push_back(std::string{symbol});
  }

  for (auto& [edge, symbols] : edges) {
    std::ranges::sort(symbols);

    const auto [source, target] = edge;

    std::string label;

    for (std::size_t i = 0; i < symbols.size(); ++i) {
      if (i != 0) {
        label += ", ";
      }

      label += symbols[i];
    }

    out << "  " << export_detail::dot_quote("q_" + std::to_string(source))
        << " -> " << export_detail::dot_quote("q_" + std::to_string(target))
        << " [label=" << export_detail::dot_quote(label) << "];\n";
  }

  out << "}\n";

  return out.str();
}

[[nodiscard]] std::string to_dot(const NFA& nfa, const StateLabels& labels) {
  std::ostringstream out;

  out << R"(digraph automaton {
  rankdir=LR;
  graph [
    bgcolor="white",
    pad="0.3",
    nodesep="0.55",
    ranksep="0.75"
  ];

  node [
    shape=circle,
    style=filled,
    fillcolor="#f8fafc",
    color="#334155",
    penwidth=1.5,
    fontname="DejaVu Sans"
  ];

  edge [
    color="#64748b",
    fontcolor="#334155",
    penwidth=1.3,
    arrowsize=0.8,
    fontname="DejaVu Sans"
  ];
)";

  for (const State state : export_detail::sorted_states(nfa.states())) {
    const auto label_it = labels.find(state);

    const std::string label =
        label_it == labels.end() ? std::to_string(state) : label_it->second;

    out << "  " << export_detail::dot_quote("q_" + std::to_string(state))
        << " [shape="
        << (nfa.final_states().contains(state) ? "doublecircle" : "circle")
        << ", label=" << export_detail::dot_quote(label) << "];\n";
  }

  for (const State state : export_detail::sorted_states(nfa.initial_states())) {
    const std::string marker = "__start_" + std::to_string(state);

    out << "  " << export_detail::dot_quote(marker)
        << " [shape=point, label=\"\", "
           "color=\"#2563eb\"];\n";

    out << "  " << export_detail::dot_quote(marker) << " -> "
        << export_detail::dot_quote("q_" + std::to_string(state))
        << " [color=\"#2563eb\", penwidth=2];\n";
  }

  std::map<std::pair<State, State>, std::vector<std::string> > edges;

  for (const auto& [key, targets] : nfa.transitions()) {
    const auto& [source, symbol] = key;

    const std::string symbol_name = symbol ? std::string{*symbol} : "ε";

    for (const State target : targets) {
      edges[{source, target}].push_back(symbol_name);
    }
  }

  for (auto& [edge, symbols] : edges) {
    std::ranges::sort(symbols);

    const auto [source, target] = edge;

    std::string label;

    for (std::size_t i = 0; i < symbols.size(); ++i) {
      if (i != 0) {
        label += ", ";
      }

      label += symbols[i];
    }

    out << "  " << export_detail::dot_quote("q_" + std::to_string(source))
        << " -> " << export_detail::dot_quote("q_" + std::to_string(target))
        << " [label=" << export_detail::dot_quote(label) << "];\n";
  }

  out << "}\n";

  return out.str();
}

void write_dot(const std::filesystem::path& path, const DFA& dfa,
    const StateLabels& labels) {
  export_detail::write_text_file(path, to_dot(dfa, labels));
}

void write_dot(const std::filesystem::path& path, const NFA& nfa,
    const StateLabels& labels) {
  export_detail::write_text_file(path, to_dot(nfa, labels));
}

[[nodiscard]] std::string format_transition_table(const DFA& dfa) {
  const auto symbols = export_detail::sorted_symbols(dfa.alphabet());

  std::ostringstream out;

  out << "| Состояние";

  for (const Symbol symbol : symbols) {
    out << " | " << export_detail::table_symbol(symbol);
  }

  out << " |\n| ---";

  for ([[maybe_unused]] const Symbol symbol : symbols) {
    out << " | ---";
  }

  out << " |\n";

  for (const State state : export_detail::sorted_states(dfa.states())) {
    if (state == dfa.initial_state()) {
      out << "| →";
    } else {
      out << "| ";
    }

    if (dfa.final_states().contains(state)) {
      out << '*';
    }

    out << state;

    for (const Symbol symbol : symbols) {
      const auto target = dfa.successor(state, symbol);

      out << " | ";

      if (target) {
        out << *target;
      } else {
        out << "∅";
      }
    }

    out << " |\n";
  }

  return out.str();
}

[[nodiscard]] std::string format_transition_table(const NFA& nfa) {
  const auto symbols = export_detail::sorted_symbols(nfa.alphabet());

  std::ostringstream out;

  out << "| Состояние | ε";

  for (const Symbol symbol : symbols) {
    out << " | " << export_detail::table_symbol(symbol);
  }

  out << " |\n| --- | ---";

  for ([[maybe_unused]] const Symbol symbol : symbols) {
    out << " | ---";
  }

  out << " |\n";

  for (const State state : export_detail::sorted_states(nfa.states())) {
    out << "| ";

    if (nfa.initial_states().contains(state)) {
      out << "→";
    }

    if (nfa.final_states().contains(state)) {
      out << '*';
    }

    out << state;

    out << " | "
        << export_detail::states_text(nfa.successors(state, std::nullopt));

    for (const Symbol symbol : symbols) {
      out << " | " << export_detail::states_text(nfa.successors(state, symbol));
    }

    out << " |\n";
  }

  return out.str();
}

[[nodiscard]] std::string format_determinization_report(
    const DeterminizationResult& result) {
  std::ostringstream out;

  for (const auto& [state, subset] : result.subsets) {
    out << 'D' << state << " = " << export_detail::states_text(subset) << '\n';
  }

  out << '\n';
  out << format_transition_table(result.dfa);

  return out.str();
}

[[nodiscard]] std::string format_minimization_report(
    const MinimizationResult& result) {
  std::ostringstream out;

  for (std::size_t number = 0; number < result.history.size(); ++number) {
    out << "Разбиение " << number << ": ";

    const auto& partition = result.history[number];

    for (std::size_t i = 0; i < partition.size(); ++i) {
      if (i != 0) {
        out << " | ";
      }

      out << export_detail::states_text(partition[i]);
    }

    out << '\n';
  }

  out << '\n';

  for (const auto& [state, minimized] : result.mapping) {
    out << 'q' << state << " → M" << minimized << '\n';
  }

  out << '\n';
  out << format_transition_table(result.dfa);

  return out.str();
}

}
