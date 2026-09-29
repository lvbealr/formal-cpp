#include "regex/regex.hpp"

#include <cctype>
#include <optional>
#include <stdexcept>
#include <unordered_set>

namespace automata {

Regex::Regex(Union value)
    : node_{std::make_shared<const Union>(std::move(value))} {}

Regex::Regex(Concat value)
    : node_{std::make_shared<const Concat>(std::move(value))} {}

Regex::Regex(Star value)
    : node_{std::make_shared<const Star>(std::move(value))} {}

[[nodiscard]] std::string format_regex(const Regex& expr) {
  if (expr.is<Empty>()) {
    return "0";
  }

  if (expr.is<Epsilon>()) {
    return "1";
  }

  if (expr.is<Literal>()) {
    const Symbol symbol = expr.as<Literal>().symbol;
    const bool escape =
        std::string_view{"01+*()\\"}.find(symbol) != std::string_view::npos
        || std::isspace(static_cast<unsigned char>(symbol));
    return (escape ? std::string{"\\"} : std::string{}) + symbol;
  }

  if (expr.is<Union>()) {
    const auto& terms = expr.as<Union>().terms;

    if (terms.empty()) {
      return "0";
    }

    std::string result = "(";

    for (std::size_t i = 0; i < terms.size(); ++i) {
      if (i != 0) {
        result += '+';
      }

      result += format_regex(terms[i]);
    }

    result += ')';
    return result;
  }

  if (expr.is<Concat>()) {
    const auto& parts = expr.as<Concat>().parts;

    if (parts.empty()) {
      return "1";
    }

    std::string result = "(";

    for (const auto& part : parts) {
      result += format_regex(part);
    }

    result += ')';
    return result;
  }

  if (expr.is<Star>()) {
    return "(" + format_regex(expr.as<Star>().inner) + ")*";
  }

  throw std::logic_error{"unknown regular expression node"};
}

[[nodiscard]] Regex simplify_regex(const Regex& expr) {
  if (expr.is<Empty>() || expr.is<Epsilon>() || expr.is<Literal>()) {
    return expr;
  }

  if (expr.is<Union>()) {
    std::vector<Regex> terms;
    std::unordered_set<std::string> seen;

    for (const auto& child : expr.as<Union>().terms) {
      Regex simplified = simplify_regex(child);

      auto add_term = [&](const Regex& term) {
        if (term.is<Empty>()) {
          return;
        }

        const auto key = format_regex(term);

        if (seen.insert(key).second) {
          terms.push_back(term);
        }
      };

      if (simplified.is<Union>()) {
        for (const auto& term : simplified.as<Union>().terms) {
          add_term(term);
        }
      } else {
        add_term(simplified);
      }
    }

    if (terms.empty()) {
      return Empty{};
    }

    if (terms.size() == 1) {
      return terms.front();
    }

    return Union{std::move(terms)};
  }

  if (expr.is<Concat>()) {
    std::vector<Regex> parts;

    for (const auto& child : expr.as<Concat>().parts) {
      Regex simplified = simplify_regex(child);

      if (simplified.is<Empty>()) {
        return Empty{};
      }

      if (simplified.is<Epsilon>()) {
        continue;
      }

      if (simplified.is<Concat>()) {
        const auto& nested = simplified.as<Concat>().parts;

        parts.insert(parts.end(), nested.begin(), nested.end());
      } else {
        parts.push_back(std::move(simplified));
      }
    }

    if (parts.empty()) {
      return Epsilon{};
    }

    if (parts.size() == 1) {
      return parts.front();
    }

    return Concat{std::move(parts)};
  }

  if (expr.is<Star>()) {
    Regex inner = simplify_regex(expr.as<Star>().inner);

    if (inner.is<Empty>() || inner.is<Epsilon>()) {
      return Epsilon{};
    }

    if (inner.is<Star>()) {
      return inner;
    }

    return Star{std::move(inner)};
  }

  throw std::logic_error{"unknown regular expression node"};
}

namespace {

class RegexParser {
 public:
  explicit RegexParser(const std::string_view text) : text_{text} {}

  [[nodiscard]] Regex parse() {
    Regex result = expression();

    if (peek()) {
      throw std::invalid_argument{
          "extra character at position " + std::to_string(pos_)};
    }

    return result;
  }

 private:
  [[nodiscard]] std::optional<char> peek() {
    while (pos_ < text_.size()
           && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
      ++pos_;
    }

    if (pos_ == text_.size()) {
      return std::nullopt;
    }

    return text_[pos_];
  }

  [[nodiscard]] Regex expression() {
    std::vector<Regex> terms;
    terms.push_back(concatenation());

    while (peek() == '+') {
      ++pos_;
      terms.push_back(concatenation());
    }

    if (terms.size() == 1) {
      return terms.front();
    }

    return Union{std::move(terms)};
  }

  [[nodiscard]] Regex concatenation() {
    std::vector<Regex> parts;

    while (true) {
      const auto symbol = peek();

      if (!symbol || *symbol == '+' || *symbol == ')') {
        break;
      }

      parts.push_back(repetition());
    }

    if (parts.empty()) {
      throw std::invalid_argument{
          "expression expected at position " + std::to_string(pos_)};
    }

    if (parts.size() == 1) {
      return parts.front();
    }

    return Concat{std::move(parts)};
  }

  [[nodiscard]] Regex repetition() {
    Regex result = atom();

    while (peek() == '*') {
      ++pos_;
      result = Star{std::move(result)};
    }

    return result;
  }

  [[nodiscard]] Regex atom() {
    const auto symbol = peek();

    if (!symbol || *symbol == '+' || *symbol == ')' || *symbol == '*') {
      throw std::invalid_argument{
          "letter or parenthesis expected at position " + std::to_string(pos_)};
    }

    ++pos_;

    if (*symbol == '\\') {
      if (pos_ == text_.size()) {
        throw std::invalid_argument{"literal expected after escape"};
      }
      return Literal{text_[pos_++]};
    }

    if (*symbol == '(') {
      Regex result = expression();

      if (peek() != ')') {
        throw std::invalid_argument{
            "closing parenthesis expected at position " + std::to_string(pos_)};
      }

      ++pos_;
      return result;
    }

    if (*symbol == '0') {
      return Empty{};
    }

    if (*symbol == '1') {
      return Epsilon{};
    }

    return Literal{*symbol};
  }

 private:
  std::string_view text_;
  std::size_t pos_{0};
};

}

[[nodiscard]] Regex parse_regex(const std::string_view text) {
  return RegexParser{text}.parse();
}

}
