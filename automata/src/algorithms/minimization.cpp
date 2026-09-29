#include "algorithms/minimization.hpp"
#include "algorithms/conversions.hpp"

#include <algorithm>
#include <iterator>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

namespace automata {

[[nodiscard]] DFA remove_unreachable(const DFA& dfa) {
  StateSet visited{dfa.initial_state()};

  std::vector<State> stack{dfa.initial_state()};

  while (!stack.empty()) {
    const State state = stack.back();

    stack.pop_back();

    for (const Symbol symbol : dfa.alphabet()) {
      const auto target = dfa.successor(state, symbol);

      if (target && visited.insert(*target).second) {
        stack.push_back(*target);
      }
    }
  }

  DFA::Transitions transitions;

  for (const auto& [key, target] : dfa.transitions()) {
    const auto& [source, symbol] = key;

    if (visited.contains(source)) {
      transitions.emplace(key, target);
    }
  }

  StateSet final_states;

  for (const State state : dfa.final_states()) {
    if (visited.contains(state)) {
      final_states.insert(state);
    }
  }

  return DFA{std::move(visited), dfa.alphabet(), dfa.initial_state(),
      std::move(final_states), std::move(transitions)};
}

[[nodiscard]] MinimizationResult minimize_with_report(const DFA& dfa) {
  DFA prepared = remove_unreachable(complete(dfa));

  std::vector<StateSet> blocks;

  StateSet final_states = prepared.final_states();

  StateSet non_final_states;

  for (const State state : prepared.states()) {
    if (!prepared.final_states().contains(state)) {
      non_final_states.insert(state);
    }
  }

  if (!final_states.empty()) {
    blocks.push_back(std::move(final_states));
  }

  if (!non_final_states.empty()) {
    blocks.push_back(std::move(non_final_states));
  }

  std::vector<std::vector<StateSet>> history;
  history.push_back(blocks);

  std::vector<Symbol> alphabet{
      prepared.alphabet().begin(), prepared.alphabet().end()};

  std::ranges::sort(alphabet);

  while (true) {
    std::unordered_map<State, std::size_t> block_of;

    for (std::size_t number = 0; number < blocks.size(); ++number) {
      for (const State state : blocks[number]) {
        block_of[state] = number;
      }
    }

    std::vector<StateSet> new_blocks;

    for (const auto& block : blocks) {
      using Signature = std::vector<std::size_t>;

      std::map<Signature, std::size_t> group_index;

      std::vector<StateSet> groups;

      std::vector<State> ordered_states{block.begin(), block.end()};

      std::ranges::sort(ordered_states);

      for (const State state : ordered_states) {
        Signature signature;
        signature.reserve(alphabet.size());

        for (const Symbol symbol : alphabet) {
          const auto target = prepared.successor(state, symbol);

          signature.push_back(block_of.at(target.value()));
        }

        const auto it = group_index.find(signature);

        if (it == group_index.end()) {
          const std::size_t index = groups.size();

          group_index.emplace(std::move(signature), index);

          groups.emplace_back(StateSet{state});
        } else {
          groups[it->second].insert(state);
        }
      }

      new_blocks.insert(new_blocks.end(),
          std::make_move_iterator(groups.begin()),
          std::make_move_iterator(groups.end()));
    }

    if (new_blocks.size() == blocks.size()) {
      break;
    }

    blocks = std::move(new_blocks);
    history.push_back(blocks);
  }

  std::map<State, State> mapping;

  for (std::size_t number = 0; number < blocks.size(); ++number) {
    const State new_state = static_cast<State>(number);

    for (const State state : blocks[number]) {
      mapping.emplace(state, new_state);
    }
  }

  DFA::Transitions transitions;
  StateSet minimized_final_states;

  for (std::size_t number = 0; number < blocks.size(); ++number) {
    const State new_state = static_cast<State>(number);

    const State representative = *std::ranges::min_element(blocks[number]);

    if (prepared.final_states().contains(representative)) {
      minimized_final_states.insert(new_state);
    }

    for (const Symbol symbol : alphabet) {
      const State target = prepared.successor(representative, symbol).value();

      transitions[{new_state, symbol}] = mapping.at(target);
    }
  }

  StateSet minimized_states;

  for (std::size_t i = 0; i < blocks.size(); ++i) {
    minimized_states.insert(static_cast<State>(i));
  }

  DFA result{std::move(minimized_states), prepared.alphabet(),
      mapping.at(prepared.initial_state()), std::move(minimized_final_states),
      std::move(transitions)};

  return {std::move(result), std::move(prepared), std::move(history),
      std::move(mapping)};
}

[[nodiscard]] DFA minimize(const DFA& dfa) {
  auto result = minimize_with_report(dfa);

  return std::move(result.dfa);
}

[[nodiscard]] DFA minimize_brzozowski(const NFA& nfa) {
  DFA first = determinize(reverse(nfa));

  return determinize(reverse(dfa_to_nfa(first)));
}

}
