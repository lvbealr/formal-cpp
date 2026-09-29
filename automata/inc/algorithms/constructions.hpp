#ifndef AUTOMATA_ALGORITHMS_CONSTRUCTIONS_HPP
#define AUTOMATA_ALGORITHMS_CONSTRUCTIONS_HPP

#include "core/models.hpp"

#include <cstddef>

namespace automata {

[[nodiscard]] NFA unite(const NFA& lhs, const NFA& rhs);

[[nodiscard]] NFA concatenate(const NFA& lhs, const NFA& rhs);

[[nodiscard]] NFA kleene_star(const NFA& nfa);

[[nodiscard]] NFA bounded_subsequences(const NFA& nfa, const std::ptrdiff_t k);

}

#endif
