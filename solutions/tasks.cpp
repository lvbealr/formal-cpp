#include "tasks.hpp"

#include "algorithms/constructions.hpp"
#include "algorithms/conversions.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace homework {
using namespace automata;

NFA build_task_1a() {
  return regex_to_nfa(parse_regex("ab(ab)*aab(aab)*"), {'a', 'b'});
}

NFA build_task_1b() {
  return regex_to_nfa(parse_regex("a(a(ab)*a(ab)*+b)*"), {'a', 'b'});
}

NFA build_task_1c() {
  const std::string symbols = "abc";
  std::vector<std::string> suffixes{""};

  for (char c : symbols) {
    suffixes.emplace_back(1, c);
  }

  for (char first : symbols) {
    for (char second : symbols) {
      suffixes.push_back(std::string{first, second});
    }
  }

  std::map<std::string, State> ids;
  StateSet states, finals;

  for (std::size_t i = 0; i < suffixes.size(); ++i) {
    ids[suffixes[i]] = static_cast<State>(i);
    states.insert(static_cast<State>(i));
    finals.insert(static_cast<State>(i));
  }

  const State sink = static_cast<State>(ids.size());
  states.insert(sink);

  DFA::Transitions transitions;

  for (const auto& suffix : suffixes) {
    for (char symbol : symbols) {
      std::string next = suffix + symbol;

      if (next.size() > 2) {
        next.erase(0, next.size() - 2);
      }

      transitions[{ids.at(suffix), symbol}] =
          suffix.size() == 2 && suffix[0] == 'b' && symbol == 'a'
              ? sink
              : ids.at(next);
    }
  }

  for (char symbol : symbols) {
    transitions[{sink, symbol}] = sink;
  }

  return dfa_to_nfa(DFA{states, {'a', 'b', 'c'}, 0, finals, transitions});
}

NFA build_task_2b() {
  NFA::Transitions a, b;

  for (State count = 0; count < 3; ++count) {
    a[{count, 'a'}] = {std::min(count + 1, 2)};
    a[{count, 'b'}] = {count};
  }

  for (State count = 0; count < 4; ++count) {
    b[{count, 'a'}] = {count};
    b[{count, 'b'}] = {std::min(count + 1, 3)};
  }

  return unite(NFA{{0, 1, 2}, {'a', 'b'}, {0}, {2}, a},
      NFA{{0, 1, 2, 3}, {'a', 'b'}, {0}, {3}, b});
}

NFA build_task_2c() {
  const std::string symbols = "abc";
  NFA::Transitions transitions;
  State waiting = 1;

  for (char remembered : symbols) {
    transitions[{0, remembered}] = {0, waiting};

    for (char symbol : symbols) {
      StateSet targets{waiting};

      if (symbol == remembered) {
        targets.insert(4);
      }

      transitions[{waiting, symbol}] = targets;
    }

    ++waiting;
  }

  return NFA{{0, 1, 2, 3, 4}, {'a', 'b', 'c'}, {0}, {4}, transitions};
}
}
