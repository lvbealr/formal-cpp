#include "core/models.hpp"

#include <vector>

namespace automata {

[[nodiscard]] const StateSet& NFA::successors(State state, Input symbol) const {
  const auto it = transitions_.find({state, symbol});

  if (it == transitions_.end()) {
    static const StateSet empty;
    return empty;
  }

  return it->second;
}

[[nodiscard]] StateSet NFA::epsilon_closure(StateSet states) const {
  std::vector<State> stack;
  stack.reserve(states.size());

  for (const State state : states) {
    stack.push_back(state);
  }

  while (!stack.empty()) {
    const State state = stack.back();
    stack.pop_back();

    for (const State successor : successors(state, std::nullopt)) {
      if (states.contains(successor)) {
        continue;
      }

      states.insert(successor);
      stack.push_back(successor);
    }
  }

  return states;
}

StateSet NFA::step(const StateSet& states, Symbol symbol) const {
  StateSet next;
  for (const State state : epsilon_closure(states)) {
    const auto& targets = successors(state, symbol);
    next.insert(targets.begin(), targets.end());
  }
  return epsilon_closure(std::move(next));
}

bool NFA::accepts(std::string_view word) const {
  StateSet current = epsilon_closure(initial_states_);
  for (const Symbol symbol : word) {
    if (!alphabet_.contains(symbol)) {
      return false;
    }
    current = step(current, symbol);
  }
  for (const State state : current) {
    if (final_states_.contains(state)) {
      return true;
    }
  }
  return false;
}

std::optional<State> DFA::successor(State state, Symbol symbol) const {
  const auto it = transitions_.find({state, symbol});
  return it == transitions_.end() ? std::nullopt : std::optional{it->second};
}

bool DFA::accepts(std::string_view word) const {
  State current = initial_state_;
  for (const Symbol symbol : word) {
    if (!alphabet_.contains(symbol)) {
      return false;
    }
    const auto next = successor(current, symbol);
    if (!next) {
      return false;
    }
    current = *next;
  }
  return final_states_.contains(current);
}

bool DFA::is_complete() const {
  for (const State state : states_) {
    for (const Symbol symbol : alphabet_) {
      if (!successor(state, symbol)) {
        return false;
      }
    }
  }
  return true;
}

}
