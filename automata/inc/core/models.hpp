#ifndef AUTOMATA_CORE_MODELS_HPP
#define AUTOMATA_CORE_MODELS_HPP

#include <map>
#include <optional>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace automata {

using State = int;
using Symbol = char;
using StateSet = std::unordered_set<State>;
using Alphabet = std::unordered_set<Symbol>;

class NFA {
 public:
  using State = automata::State;
  using Symbol = automata::Symbol;
  using StateSet = automata::StateSet;
  using Input = std::optional<Symbol>;
  using TransitionKey = std::pair<State, Input>;
  using Transitions = std::map<TransitionKey, StateSet>;

 public:
  NFA(StateSet states, Alphabet alphabet, StateSet initial_states,
      StateSet final_states, Transitions transitions)
      : states_{std::move(states)},
        alphabet_{std::move(alphabet)},
        initial_states_{std::move(initial_states)},
        final_states_{std::move(final_states)},
        transitions_{std::move(transitions)} {}

 public:
  [[nodiscard]] const StateSet& states() const { return states_; }
  [[nodiscard]] const Alphabet& alphabet() const { return alphabet_; }
  [[nodiscard]] const StateSet& initial_states() const {
    return initial_states_;
  }
  [[nodiscard]] const StateSet& final_states() const { return final_states_; }
  [[nodiscard]] const Transitions& transitions() const { return transitions_; }
  [[nodiscard]] const StateSet& successors(State state, Input symbol) const;
  [[nodiscard]] StateSet epsilon_closure(StateSet states) const;
  [[nodiscard]] StateSet step(const StateSet& states, Symbol symbol) const;
  [[nodiscard]] bool accepts(std::string_view word) const;

 private:
  StateSet states_;
  Alphabet alphabet_;
  StateSet initial_states_;
  StateSet final_states_;
  Transitions transitions_;
};

class DFA {
 public:
  using TransitionKey = std::pair<State, Symbol>;
  using Transitions = std::map<TransitionKey, State>;

 public:
  DFA(StateSet states, Alphabet alphabet, State initial_state,
      StateSet final_states, Transitions transitions)
      : states_{std::move(states)},
        alphabet_{std::move(alphabet)},
        initial_state_{initial_state},
        final_states_{std::move(final_states)},
        transitions_{std::move(transitions)} {}

 public:
  [[nodiscard]] const StateSet& states() const { return states_; }
  [[nodiscard]] const Alphabet& alphabet() const { return alphabet_; }
  [[nodiscard]] State initial_state() const { return initial_state_; }
  [[nodiscard]] const StateSet& final_states() const { return final_states_; }
  [[nodiscard]] const Transitions& transitions() const { return transitions_; }
  [[nodiscard]] std::optional<State> successor(
      State state, Symbol symbol) const;
  [[nodiscard]] bool accepts(std::string_view word) const;
  [[nodiscard]] bool is_complete() const;

 private:
  StateSet states_;
  Alphabet alphabet_;
  State initial_state_;
  StateSet final_states_;
  Transitions transitions_;
};

}
#endif
