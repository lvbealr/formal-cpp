#include "algorithms/conversions.hpp"
#include "algorithms/constructions.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace automata {

namespace detail {

[[nodiscard]] std::vector<State> subset_key(const StateSet& states) {
  std::vector<State> result{states.begin(), states.end()};

  std::ranges::sort(result);
  return result;
}

[[nodiscard]] std::vector<Symbol> sorted_alphabet(const Alphabet& alphabet) {
  std::vector<Symbol> result{alphabet.begin(), alphabet.end()};

  std::ranges::sort(result);
  return result;
}

}

[[nodiscard]] NFA dfa_to_nfa(const DFA& dfa) {
  NFA::Transitions transitions;

  for (const auto& [key, target] : dfa.transitions()) {
    const auto& [state, symbol] = key;

    transitions.emplace(NFA::TransitionKey{state, symbol}, StateSet{target});
  }

  return NFA{dfa.states(), dfa.alphabet(), StateSet{dfa.initial_state()},
      dfa.final_states(), std::move(transitions)};
}

[[nodiscard]] NFA remove_epsilon(const NFA& nfa) {
  NFA::Transitions transitions;

  for (const State state : nfa.states()) {
    for (const Symbol symbol : nfa.alphabet()) {
      StateSet targets = nfa.step(StateSet{state}, symbol);

      if (!targets.empty()) {
        transitions.emplace(
            NFA::TransitionKey{state, symbol}, std::move(targets));
      }
    }
  }

  StateSet final_states;

  for (const State state : nfa.states()) {
    const StateSet closure = nfa.epsilon_closure(StateSet{state});

    for (const State candidate : closure) {
      if (nfa.final_states().contains(candidate)) {
        final_states.insert(state);
        break;
      }
    }
  }

  return NFA{nfa.states(), nfa.alphabet(), nfa.initial_states(),
      std::move(final_states), std::move(transitions)};
}

[[nodiscard]] DeterminizationResult determinize_with_report(const NFA& nfa) {
  StateSet start = nfa.epsilon_closure(nfa.initial_states());

  using SubsetKey = std::vector<State>;

  std::map<SubsetKey, State> subset_to_id;

  subset_to_id.emplace(detail::subset_key(start), 0);

  std::vector<StateSet> queue;
  queue.push_back(start);

  std::size_t position = 0;

  DFA::Transitions transitions;
  StateSet final_states;
  std::map<State, StateSet> subsets;

  const auto alphabet = detail::sorted_alphabet(nfa.alphabet());

  while (position < queue.size()) {
    const StateSet current = queue[position++];

    const auto current_key = detail::subset_key(current);

    const State current_id = subset_to_id.at(current_key);

    subsets.emplace(current_id, current);

    for (const State state : current) {
      if (nfa.final_states().contains(state)) {
        final_states.insert(current_id);
        break;
      }
    }

    for (const Symbol symbol : alphabet) {
      StateSet next_subset = nfa.step(current, symbol);

      const auto next_key = detail::subset_key(next_subset);

      auto it = subset_to_id.find(next_key);

      if (it == subset_to_id.end()) {
        if (subset_to_id.size()
            > static_cast<std::size_t>(std::numeric_limits<State>::max())) {
          throw std::overflow_error{"DFA state id overflow"};
        }

        const State new_id = static_cast<State>(subset_to_id.size());

        auto [inserted, _] = subset_to_id.emplace(next_key, new_id);

        it = inserted;

        queue.push_back(std::move(next_subset));
      }

      transitions[{current_id, symbol}] = it->second;
    }
  }

  StateSet states;

  for (const auto& [id, _] : subsets) {
    states.insert(id);
  }

  DFA dfa{std::move(states), nfa.alphabet(), 0, std::move(final_states),
      std::move(transitions)};

  return {std::move(dfa), std::move(subsets)};
}

[[nodiscard]] DFA determinize(const NFA& nfa) {
  auto result = determinize_with_report(nfa);

  return std::move(result.dfa);
}

[[nodiscard]] DFA complete(const DFA& dfa) {
  if (dfa.is_complete()) {
    return dfa;
  }

  StateSet states = dfa.states();
  DFA::Transitions transitions = dfa.transitions();

  State sink = 0;

  if (!states.empty()) {
    const State maximum = *std::ranges::max_element(states);

    if (maximum == std::numeric_limits<State>::max()) {
      throw std::overflow_error{"DFA state id overflow"};
    }

    sink = maximum + 1;
  }

  states.insert(sink);

  for (const State state : dfa.states()) {
    for (const Symbol symbol : dfa.alphabet()) {
      transitions.try_emplace(DFA::TransitionKey{state, symbol}, sink);
    }
  }

  for (const Symbol symbol : dfa.alphabet()) {
    transitions[{sink, symbol}] = sink;
  }

  return DFA{std::move(states), dfa.alphabet(), dfa.initial_state(),
      dfa.final_states(), std::move(transitions)};
}

[[nodiscard]] DFA complement(const DFA& dfa) {
  DFA full = complete(dfa);

  StateSet final_states;

  for (const State state : full.states()) {
    if (!full.final_states().contains(state)) {
      final_states.insert(state);
    }
  }

  return DFA{full.states(), full.alphabet(), full.initial_state(),
      std::move(final_states), full.transitions()};
}

[[nodiscard]] NFA reverse(const NFA& nfa) {
  NFA::Transitions transitions;

  for (const auto& [key, targets] : nfa.transitions()) {
    const auto& [source, symbol] = key;

    for (const State target : targets) {
      transitions[{target, symbol}].insert(source);
    }
  }

  return NFA{nfa.states(), nfa.alphabet(), nfa.final_states(),
      nfa.initial_states(), std::move(transitions)};
}

[[nodiscard]] NFA regex_to_nfa(const Regex& expr, const Alphabet& alphabet) {
  if (expr.is<Empty>()) {
    return NFA{StateSet{0}, alphabet, StateSet{0}, {}, {}};
  }

  if (expr.is<Epsilon>()) {
    NFA::Transitions transitions;

    transitions[{0, std::nullopt}] = StateSet{1};

    return NFA{StateSet{0, 1}, alphabet, StateSet{0}, StateSet{1},
        std::move(transitions)};
  }

  if (expr.is<Literal>()) {
    if (!alphabet.contains(expr.as<Literal>().symbol)) {
      throw std::invalid_argument{"Regex literal is not in the alphabet"};
    }
    NFA::Transitions transitions;

    transitions[{0, expr.as<Literal>().symbol}] = StateSet{1};

    return NFA{StateSet{0, 1}, alphabet, StateSet{0}, StateSet{1},
        std::move(transitions)};
  }

  if (expr.is<Union>()) {
    const auto& terms = expr.as<Union>().terms;

    if (terms.empty()) {
      return regex_to_nfa(Regex{Empty{}}, alphabet);
    }

    NFA result = regex_to_nfa(terms.front(), alphabet);

    for (std::size_t i = 1; i < terms.size(); ++i) {
      result = unite(result, regex_to_nfa(terms[i], alphabet));
    }

    return result;
  }

  if (expr.is<Concat>()) {
    const auto& parts = expr.as<Concat>().parts;

    if (parts.empty()) {
      return regex_to_nfa(Regex{Epsilon{}}, alphabet);
    }

    NFA result = regex_to_nfa(parts.front(), alphabet);

    for (std::size_t i = 1; i < parts.size(); ++i) {
      result = concatenate(result, regex_to_nfa(parts[i], alphabet));
    }

    return result;
  }

  if (expr.is<Star>()) {
    return kleene_star(regex_to_nfa(expr.as<Star>().inner, alphabet));
  }

  throw std::logic_error{"unknown regular expression node"};
}

[[nodiscard]] Regex nfa_to_regex(const NFA& nfa) {
  State start = 0;

  if (!nfa.states().empty()) {
    const State maximum = *std::ranges::max_element(nfa.states());

    if (maximum > std::numeric_limits<State>::max() - 2) {
      throw std::overflow_error{"NFA state id overflow"};
    }

    start = maximum + 1;
  }

  const State final = start + 1;

  StateSet remaining = nfa.states();
  remaining.insert(start);
  remaining.insert(final);

  using Edge = std::pair<State, State>;

  std::map<Edge, Regex> labels;

  const auto get_label = [&](const State source, const State target) -> Regex {
    const auto it = labels.find({source, target});

    if (it == labels.end()) {
      return Empty{};
    }

    return it->second;
  };

  const auto add_label = [&](const State source, const State target,
                             const Regex& expression) {
    Regex previous = get_label(source, target);

    labels[{source, target}] =
        simplify_regex(Union{{std::move(previous), expression}});
  };

  for (const auto& [key, targets] : nfa.transitions()) {
    const auto& [source, symbol] = key;

    Regex expression = symbol ? Regex{Literal{*symbol}} : Regex{Epsilon{}};

    for (const State target : targets) {
      add_label(source, target, expression);
    }
  }

  for (const State state : nfa.initial_states()) {
    add_label(start, state, Epsilon{});
  }

  for (const State state : nfa.final_states()) {
    add_label(state, final, Epsilon{});
  }

  auto removed_states = detail::subset_key(nfa.states());

  for (const State removed : removed_states) {
    remaining.erase(removed);

    Regex loop{Star{get_label(removed, removed)}};

    auto remaining_sorted = detail::subset_key(remaining);

    for (const State source : remaining_sorted) {
      Regex incoming = get_label(source, removed);

      if (incoming.is<Empty>()) {
        continue;
      }

      for (const State target : remaining_sorted) {
        Regex outgoing = get_label(removed, target);

        if (outgoing.is<Empty>()) {
          continue;
        }

        add_label(source, target, Concat{{incoming, loop, outgoing}});
      }
    }

    for (auto it = labels.begin(); it != labels.end();) {
      const auto& [source, target] = it->first;

      if (source == removed || target == removed) {
        it = labels.erase(it);
      } else {
        ++it;
      }
    }
  }

  return simplify_regex(get_label(start, final));
}

}
