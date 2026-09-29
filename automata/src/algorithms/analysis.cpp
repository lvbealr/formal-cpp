#include "algorithms/analysis.hpp"
#include "algorithms/conversions.hpp"
#include "core/graph.hpp"

#include <map>

namespace automata {

namespace analysis_detail {

using ProductState = std::pair<State, State>;

struct Parent {
  ProductState previous{};
  Symbol symbol{};
  bool root{false};
};

[[nodiscard]] std::vector<Symbol> sorted_alphabet(const Alphabet& alphabet) {
  std::vector<Symbol> result{alphabet.begin(), alphabet.end()};

  std::ranges::sort(result);

  return result;
}

[[nodiscard]] std::optional<std::string> find_word(const DFA& left,
    const DFA& right, const State initial_left, const State initial_right) {
  const ProductState initial{initial_left, initial_right};

  std::vector<ProductState> queue{initial};
  std::size_t position = 0;

  std::map<ProductState, Parent> parents;

  parents.emplace(initial, Parent{.root = true});

  const auto alphabet = sorted_alphabet(left.alphabet());

  while (position < queue.size()) {
    const ProductState current = queue[position++];

    const auto [left_state, right_state] = current;

    const bool left_final = left.final_states().contains(left_state);

    const bool right_final = right.final_states().contains(right_state);

    if (left_final != right_final) {
      std::string word;

      ProductState cursor = current;

      while (!parents.at(cursor).root) {
        const Parent edge = parents.at(cursor);

        word.push_back(edge.symbol);
        cursor = edge.previous;
      }

      std::ranges::reverse(word);

      return word;
    }

    for (const Symbol symbol : alphabet) {
      const State next_left = left.successor(left_state, symbol).value();

      const State next_right = right.successor(right_state, symbol).value();

      const ProductState next{next_left, next_right};

      if (parents.contains(next)) {
        continue;
      }

      parents.emplace(
          next, Parent{.previous = current, .symbol = symbol, .root = false});

      queue.push_back(next);
    }
  }

  return std::nullopt;
}

}

[[nodiscard]] std::optional<std::string> find_counterexample(
    const DFA& left, const DFA& right) {
  if (left.alphabet() != right.alphabet()) {
    throw std::invalid_argument{
        "To compare automata, their alphabets must be identical."};
  }

  const DFA full_left = complete(left);
  const DFA full_right = complete(right);

  return analysis_detail::find_word(full_left, full_right,
      full_left.initial_state(), full_right.initial_state());
}

[[nodiscard]] bool equivalent_to(const DFA& left, const DFA& right) {
  return !find_counterexample(left, right).has_value();
}

[[nodiscard]] std::optional<std::string> distinguishing_word(
    const DFA& dfa, const State lhs, const State rhs) {
  if (!dfa.states().contains(lhs) || !dfa.states().contains(rhs)) {
    throw std::invalid_argument{"State does not belong to the DFA."};
  }

  const DFA full = complete(dfa);
  return analysis_detail::find_word(full, full, lhs, rhs);
}

[[nodiscard]] bool is_infinite(const NFA& nfa) {
  const NFA clean = remove_epsilon(nfa);

  const auto graph = graphs(clean);

  const StateSet reachable_from_start =
      reachable(graph.forward, clean.initial_states());

  const StateSet can_reach_final =
      reachable(graph.backward, clean.final_states());

  StateSet useful;

  for (const State state : reachable_from_start) {
    if (can_reach_final.contains(state)) {
      useful.insert(state);
    }
  }

  std::map<State, std::size_t> in_degree;

  for (const State state : useful) {
    std::size_t degree = 0;

    const auto it = graph.backward.find(state);

    if (it != graph.backward.end()) {
      for (const State source : it->second) {
        if (useful.contains(source)) {
          ++degree;
        }
      }
    }

    in_degree.emplace(state, degree);
  }

  std::vector<State> ordered_states{useful.begin(), useful.end()};

  std::ranges::sort(ordered_states);

  std::vector<State> queue;

  for (const State state : ordered_states) {
    if (in_degree.at(state) == 0) {
      queue.push_back(state);
    }
  }

  std::size_t position = 0;

  while (position < queue.size()) {
    const State state = queue[position++];

    const auto it = graph.forward.find(state);

    if (it == graph.forward.end()) {
      continue;
    }

    for (const State target : it->second) {
      if (!useful.contains(target)) {
        continue;
      }

      auto& degree = in_degree.at(target);

      --degree;

      if (degree == 0) {
        queue.push_back(target);
      }
    }
  }

  return queue.size() < useful.size();
}

}
