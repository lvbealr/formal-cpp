#ifndef AUTOMATA_REGEX_REGEX_HPP
#define AUTOMATA_REGEX_REGEX_HPP

#include "core/models.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace automata {

struct Empty {};
struct Epsilon {};

struct Literal {
  Symbol symbol;
};

struct Union;
struct Concat;
struct Star;

class Regex {
 public:
  Regex() : node_{Empty{}} {}

  Regex(Empty value) : node_{value} {}

  Regex(Epsilon value) : node_{value} {}

  Regex(Literal value) : node_{value} {}

  Regex(Union value);
  Regex(Concat value);
  Regex(Star value);

  template <typename T>
  [[nodiscard]] bool is() const noexcept {
    if constexpr (std::is_same_v<T, Empty> || std::is_same_v<T, Epsilon>
                  || std::is_same_v<T, Literal>) {
      return std::holds_alternative<T>(node_);
    } else {
      return std::holds_alternative<std::shared_ptr<const T>>(node_);
    }
  }

  template <typename T>
  [[nodiscard]] const T& as() const {
    if constexpr (std::is_same_v<T, Empty> || std::is_same_v<T, Epsilon>
                  || std::is_same_v<T, Literal>) {
      return std::get<T>(node_);
    } else {
      return *std::get<std::shared_ptr<const T>>(node_);
    }
  }

 private:
  using Node =
      std::variant<Empty, Epsilon, Literal, std::shared_ptr<const Union>,
          std::shared_ptr<const Concat>, std::shared_ptr<const Star>>;

  Node node_;
};

struct Union {
  std::vector<Regex> terms;
};

struct Concat {
  std::vector<Regex> parts;
};

struct Star {
  Regex inner;
};

[[nodiscard]] std::string format_regex(const Regex& expr);
[[nodiscard]] Regex simplify_regex(const Regex& expr);
[[nodiscard]] Regex parse_regex(std::string_view text);

}

#endif
