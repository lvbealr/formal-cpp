#include "algorithms/constructions.hpp"
#include "algorithms/conversions.hpp"
#include "core/graph.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace automata {

namespace detail {

[[nodiscard]] std::vector<State> sorted_states(const StateSet& states) {
  std::vector<State> result{states.begin(), states.end()};

  std::ranges::sort(result);
  return result;
}

struct RenamedNFA {
  NFA nfa;
  std::int64_t next_id;
};

[[nodiscard]] RenamedNFA rename(const NFA& nfa, const std::int64_t start_id) {
  std::map<State, State> mapping;

  std::int64_t next_id = start_id;

  const auto states = sorted_states(nfa.states());

  for (const State state : states) {
    if (next_id > std::numeric_limits<State>::max()) {
      throw std::overflow_error{"NFA state id overflow"};
    }
    mapping.emplace(state, static_cast<State>(next_id++));
  }

  StateSet new_states;
  StateSet new_initial_states;
  StateSet new_final_states;
  NFA::Transitions new_transitions;

  for (const auto& [old_state, new_state] : mapping) {
    new_states.insert(new_state);
  }

  for (const State state : nfa.initial_states()) {
    new_initial_states.insert(mapping.at(state));
  }

  for (const State state : nfa.final_states()) {
    new_final_states.insert(mapping.at(state));
  }

  for (const auto& [key, targets] : nfa.transitions()) {
    const auto& [state, symbol] = key;

    StateSet new_targets;

    for (const State target : targets) {
      new_targets.insert(mapping.at(target));
    }

    new_transitions.emplace(
        NFA::TransitionKey{mapping.at(state), symbol}, std::move(new_targets));
  }

  return {
      NFA{std::move(new_states), nfa.alphabet(), std::move(new_initial_states),
          std::move(new_final_states), std::move(new_transitions)},
      next_id};
}

}

[[nodiscard]] NFA unite(const NFA& lhs, const NFA& rhs) {
  Alphabet alphabet = lhs.alphabet();
  alphabet.insert(rhs.alphabet().begin(), rhs.alphabet().end());
  auto left = detail::rename(lhs, 0);

  auto right = detail::rename(rhs, left.next_id);

  StateSet states = left.nfa.states();
  states.insert(right.nfa.states().begin(), right.nfa.states().end());

  StateSet initial_states = left.nfa.initial_states();

  initial_states.insert(
      right.nfa.initial_states().begin(), right.nfa.initial_states().end());

  StateSet final_states = left.nfa.final_states();

  final_states.insert(
      right.nfa.final_states().begin(), right.nfa.final_states().end());

  NFA::Transitions transitions = left.nfa.transitions();

  for (const auto& [key, targets] : right.nfa.transitions()) {
    transitions[key] = targets;
  }

  return NFA{std::move(states), std::move(alphabet), std::move(initial_states),
      std::move(final_states), std::move(transitions)};
}

[[nodiscard]] NFA concatenate(const NFA& lhs, const NFA& rhs) {
  Alphabet alphabet = lhs.alphabet();
  alphabet.insert(rhs.alphabet().begin(), rhs.alphabet().end());
  auto left = detail::rename(lhs, 0);

  auto right = detail::rename(rhs, left.next_id);

  StateSet states = left.nfa.states();

  states.insert(right.nfa.states().begin(), right.nfa.states().end());

  NFA::Transitions transitions = left.nfa.transitions();

  for (const auto& [key, targets] : right.nfa.transitions()) {
    transitions[key] = targets;
  }

  for (const State final : left.nfa.final_states()) {
    auto& targets = transitions[{final, std::nullopt}];

    targets.insert(
        right.nfa.initial_states().begin(), right.nfa.initial_states().end());
  }

  return NFA{std::move(states), std::move(alphabet), left.nfa.initial_states(),
      right.nfa.final_states(), std::move(transitions)};
}

[[nodiscard]] NFA kleene_star(const NFA& nfa) {
  StateSet states = nfa.states();

  State new_start = 0;

  if (!states.empty()) {
    const State maximum = *std::ranges::max_element(states);

    if (maximum == std::numeric_limits<State>::max()) {
      throw std::overflow_error{"NFA state id overflow"};
    }

    new_start = maximum + 1;
  }

  states.insert(new_start);

  NFA::Transitions transitions = nfa.transitions();

  transitions[{new_start, std::nullopt}] = nfa.initial_states();

  for (const State final : nfa.final_states()) {
    transitions[{final, std::nullopt}].insert(new_start);
  }

  return NFA{std::move(states), nfa.alphabet(), StateSet{new_start},
      StateSet{new_start}, std::move(transitions)};
}

[[nodiscard]] NFA bounded_subsequences(const NFA& nfa, const std::ptrdiff_t k) {
  if (k < 0) {
    throw std::invalid_argument{"k must be non-negative"};
  }

  const auto limit = static_cast<std::size_t>(k);

  if (!nfa.states().empty()
      && limit >= (static_cast<std::size_t>(std::numeric_limits<State>::max())
                      + 1)
                      / nfa.states().size()) {
    throw std::overflow_error{"NFA state id overflow"};
  }
  const NFA clean = remove_epsilon(nfa);

  const auto graph = graphs(clean);
  const StateSet starts = reachable(graph.forward, clean.initial_states());
  const StateSet finishes = reachable(graph.backward, clean.final_states());

  using IdKey = std::pair<State, std::size_t>;

  std::map<IdKey, State> ids;

  for (const State state : detail::sorted_states(clean.states())) {
    for (std::size_t skipped = 0; skipped <= limit; ++skipped) {
      if (ids.size()
          > static_cast<std::size_t>(std::numeric_limits<State>::max())) {
        throw std::overflow_error{"NFA state id overflow"};
      }

      ids.emplace(IdKey{state, skipped}, static_cast<State>(ids.size()));
    }
  }

  NFA::Transitions transitions;

  for (const auto& [key, targets] : clean.transitions()) {
    const auto& [source, input] = key;

    if (!input) {
      continue;
    }

    for (std::size_t skipped = 0; skipped <= limit; ++skipped) {
      const State source_id = ids.at({source, skipped});

      for (const State target : targets) {
        transitions[{source_id, *input}].insert(ids.at({target, 0}));

        if (skipped < limit) {
          transitions[{source_id, std::nullopt}].insert(
              ids.at({target, skipped + 1}));
        }
      }
    }
  }

  StateSet initial_states;

  for (const State state : starts) {
    initial_states.insert(ids.at({state, 0}));
  }

  StateSet final_states;

  for (const State state : finishes) {
    for (std::size_t skipped = 0; skipped <= limit; ++skipped) {
      final_states.insert(ids.at({state, skipped}));
    }
  }

  StateSet result_states;

  for (const auto& [_, id] : ids) {
    result_states.insert(id);
  }

  return NFA{std::move(result_states), clean.alphabet(),
      std::move(initial_states), std::move(final_states),
      std::move(transitions)};
}

}
