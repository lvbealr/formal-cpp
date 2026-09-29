#ifndef AUTOMATA_ALGORITHMS_ANALYSIS_HPP
#define AUTOMATA_ALGORITHMS_ANALYSIS_HPP

#include "core/models.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace automata {

[[nodiscard]] std::optional<std::string> find_counterexample(
    const DFA& left, const DFA& right);

[[nodiscard]] bool equivalent_to(const DFA& left, const DFA& right);

[[nodiscard]] std::optional<std::string> distinguishing_word(
    const DFA& dfa, const State lhs, const State rhs);

[[nodiscard]] bool is_infinite(const NFA& nfa);

template <typename Accepts>
[[nodiscard]] bool is_infinite_via_oracle(Accepts&& accepts,
    const Alphabet& alphabet, const std::ptrdiff_t state_bound) {
  if (state_bound < 1) {
    throw std::invalid_argument{"The state bound must be positive."};
  }

  auto symbols = std::vector<Symbol>{alphabet.begin(), alphabet.end()};

  std::ranges::sort(symbols);

  if (symbols.empty()) {
    return false;
  }

  const auto bound = static_cast<std::size_t>(state_bound);

  if (bound > (std::numeric_limits<std::size_t>::max() / 2) + 1) {
    throw std::overflow_error{"state_bound is too large"};
  }

  const std::size_t limit = 2 * bound - 1;

  std::vector<std::string> stack{std::string{}};

  while (!stack.empty()) {
    std::string word = std::move(stack.back());

    stack.pop_back();

    if (word.size() >= bound && std::invoke(accepts, word)) {
      return true;
    }

    if (word.size() >= limit) {
      continue;
    }

    for (auto it = symbols.rbegin(); it != symbols.rend(); ++it) {
      stack.push_back(word + *it);
    }
  }

  return false;
}

}

#endif
