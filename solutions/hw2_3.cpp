#include "tasks.hpp"

#include "algorithms/analysis.hpp"
#include "algorithms/constructions.hpp"
#include "algorithms/conversions.hpp"
#include "algorithms/minimization.hpp"
#include "io/serialization.hpp"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

using namespace automata;
using namespace homework;
using Builder = NFA (*)();

class Gallery {
 public:
  explicit Gallery(std::filesystem::path output) : output_{std::move(output)} {}

  template <typename Automaton>
  void add(const std::string& name, const std::string& title,
      const Automaton& automaton, DrawingOptions drawing = {}) {
    drawing.title = title;
    save_json(automaton, output_ / (name + ".json"), drawing);

    items_.push_back({{"file", name + ".json"}, {"title", title}});
  }

  void finish(const std::string& report) const {
    const json manifest = {
        {"schema_version", 1},
        {"type", "gallery"},
        {"title", "Формальные языки · ДЗ №2–3"},
        {"items", items_},
    };

    write_json(manifest, output_ / "manifest.json");
    export_detail::write_text_file(output_ / "report.txt", report);

    std::cout << report << "\nСохранено " << items_.size()
              << " описаний рисунков: " << output_ << '\n';
  }

 private:
  std::filesystem::path output_;
  json items_ = json::array();
};

void solve_task_1(Gallery& gallery, std::ostream& out) {
  const std::pair<std::string, Builder> examples[] = {
      {"1a", build_task_1a},
      {"1b", build_task_1b},
      {"1c", build_task_1c},
  };

  for (const auto& [name, build] : examples) {
    const NFA nfa = build();

    out << "\n№" << name << ": НКА (→ — старт, * — финал)\n"
        << format_transition_table(nfa);

    gallery.add(name + "_nfa", "№" + name + " · Исходный НКА", nfa);
  }
}

void solve_task_2(Gallery& gallery, std::ostream& out) {
  const std::pair<std::string, Builder> examples[] = {
      {"2a", build_task_1b},
      {"2b", build_task_2b},
      {"2c", build_task_2c},
  };

  for (const auto& [name, build] : examples) {
    const NFA nfa = build();
    const auto deterministic = determinize_with_report(nfa);
    const auto minimal = minimize_with_report(deterministic.dfa);

    const std::string prefix = "№" + name + " · ";
    const std::string subsets = format_determinization_report(deterministic);
    const std::string partitions = format_minimization_report(minimal);

    out << "\n№" << name << ": детерминизация\n"
        << subsets << "Минимизация:\n"
        << partitions;

    gallery.add(name + "_nfa", prefix + "НКА", nfa);
    gallery.add(
        name + "_no_epsilon", prefix + "Без ε-переходов", remove_epsilon(nfa));

    DrawingOptions dfa_drawing;
    dfa_drawing.notes = subsets;
    gallery.add(name + "_dfa", prefix + "ДКА", deterministic.dfa, dfa_drawing);

    for (std::size_t step = 0; step < minimal.history.size(); ++step) {
      DrawingOptions drawing;
      drawing.groups = minimal.history[step];
      drawing.notes = partitions;

      const std::string number = std::to_string(step);
      gallery.add(name + "_partition_" + number, prefix + "Разбиение " + number,
          minimal.prepared, drawing);
    }

    gallery.add(
        name + "_minimal", prefix + "Минимальный полный ДКА", minimal.dfa);
  }
}

void solve_task_3(Gallery& gallery, std::ostream& out) {
  NFA::Transitions edges = {
      {{0, std::nullopt}, {1}},
      {{1, std::nullopt}, {2}},
      {{2, std::nullopt}, {1}},
  };
  const auto original_edges = edges;

  DrawingOptions drawing;
  drawing.positions = {{0, {0, 0}}, {1, {2, 1}}, {2, {2, -1}}};

  out << "\n№3: все рёбра помечены ε.\n";

  for (int step = 0; step < 3; ++step) {
    const NFA nfa{{0, 1, 2}, {'a'}, {0}, {2}, edges};
    const std::string number = std::to_string(step);

    gallery.add("3_step_" + number, "№3 · Шаг " + number, nfa, drawing);
    out << "Шаг " << step << ":\n" << format_transition_table(nfa);

    if (step == 2) {
      break;
    }

    auto& targets = edges.at({0, std::nullopt});
    const State target = *std::ranges::min_element(targets);
    const auto outgoing = edges.at({target, std::nullopt});

    targets.insert(outgoing.begin(), outgoing.end());
    targets.erase(target);
  }

  out << "Исходный граф повторился: " << (edges == original_edges)
      << "\nПри повторении этого допустимого выбора алгоритм не завершится.\n";
}

void solve_task_4(Gallery& gallery, std::ostream& out) {
  const std::pair<std::string, std::string> expressions[] = {
      {"4a", "(ab+ba)*(1+a+ba)"},
      {"4b", "(ab)*a*+((a+b)(a+b))*"},
  };

  for (const auto& [name, source] : expressions) {
    const NFA nfa = regex_to_nfa(parse_regex(source), {'a', 'b'});
    const DFA result = minimize(complement(determinize(nfa)));

    const std::string text = format_regex(nfa_to_regex(dfa_to_nfa(result)));
    const DFA restored =
        determinize(regex_to_nfa(parse_regex(text), {'a', 'b'}));

    if (!equivalent_to(result, restored)) {
      throw std::logic_error{"Task 4 regex round-trip failed"};
    }

    out << "\n№" << name << ": " << text
        << "\nПроверка итогового выражения: true\n";

    DrawingOptions drawing;
    drawing.notes = text;
    gallery.add(name + "_complement", "№" + name + " · Автомат дополнения",
        result, drawing);
  }
}

void solve_task_5(Gallery& gallery, std::ostream& out) {
  const DFA original{{0}, {'a'}, 0, {0}, {}};
  const DFA wrong{{0}, {'a'}, 0, {}, {}};
  const DFA correct = complement(original);

  const auto word = find_counterexample(wrong, correct);
  if (!word) {
    throw std::logic_error{"Task 5 witness is missing"};
  }

  out << "\n№5: различающее слово: " << std::quoted(*word)
      << "\nИсходный: " << original.accepts(*word)
      << "\nОшибочное дополнение: " << wrong.accepts(*word)
      << "\nПравильное дополнение: " << correct.accepts(*word) << '\n';

  gallery.add("5_original", "№5 · Исходный ДКА", original);
  gallery.add("5_wrong", "№5 · Ошибочное дополнение", wrong);
  gallery.add("5_correct", "№5 · Правильное дополнение", correct);
}

void solve_task_6(Gallery& gallery, std::ostream& out) {
  const std::pair<std::string, std::string> expressions[] = {
      {"finite", "1+ab"},
      {"infinite", "a*"},
  };

  for (const auto& [name, source] : expressions) {
    const NFA nfa = regex_to_nfa(parse_regex(source), {'a', 'b'});
    const auto bound = static_cast<std::ptrdiff_t>(nfa.states().size());

    const bool by_graph = is_infinite(nfa);
    const bool by_oracle = is_infinite_via_oracle(
        [&](const std::string& word) { return nfa.accepts(word); },
        nfa.alphabet(), bound);

    out << "\n№6: " << source << ": по графу " << by_graph << ", по функции "
        << by_oracle << '\n';

    gallery.add("6_" + name, "№6 · Язык " + source, minimize(determinize(nfa)));
  }

  const NFA epsilon_cycle{{0}, {'a'}, {0}, {0}, {{{0, std::nullopt}, {0}}}};
  out << "Только ε-цикл — бесконечность: " << is_infinite(epsilon_cycle)
      << '\n';
}

void solve_task_7(Gallery& gallery, std::ostream& out) {
  const NFA original = regex_to_nfa(parse_regex("abcda"), {'a', 'b', 'c', 'd'});

  DrawingOptions drawing;
  drawing.notes = "Показан минимальный полный ДКА построенного языка.";

  for (int k = 0; k < 4; ++k) {
    const NFA result = bounded_subsequences(original, k);

    out << "\n№7: k=" << k;
    for (const std::string word : {"", "aa", "ada", "bcd"}) {
      out << ' ' << std::quoted(word) << ": " << result.accepts(word);
    }
    out << '\n';

    const std::string number = std::to_string(k);
    gallery.add("7_k" + number, "№7 · Подпоследовательности, k = " + number,
        minimize(determinize(result)), drawing);
  }
}

void solve_task_8(Gallery& gallery, std::ostream& out) {
  const NFA original = build_task_1b();
  const NFA reversed = reverse(original);
  const DFA first = determinize(reversed);
  const NFA second = reverse(dfa_to_nfa(first));
  const DFA result = determinize(second);
  const DFA ordinary = minimize(determinize(original));

  out << "\n№8: совпадение языков: " << equivalent_to(ordinary, result)
      << "\nЧисло состояний двумя способами: " << ordinary.states().size()
      << ' ' << result.states().size() << '\n';

  const auto states = export_detail::sorted_states(result.states());
  for (std::size_t i = 0; i < states.size(); ++i) {
    for (std::size_t j = i + 1; j < states.size(); ++j) {
      const auto witness = distinguishing_word(result, states[i], states[j]);
      if (!witness) {
        throw std::logic_error{"Task 8 result is not minimal"};
      }

      out << "Состояния " << states[i] << ", " << states[j]
          << ": различающее слово " << std::quoted(*witness) << '\n';
    }
  }

  gallery.add("8_step_1", "№8 · Первый разворот", reversed);
  gallery.add("8_step_2", "№8 · Первая детерминизация", first);
  gallery.add("8_step_3", "№8 · Второй разворот", second);
  gallery.add("8_step_4", "№8 · Вторая детерминизация", result);
}

void solve_homework(Gallery& gallery, std::ostream& out) {
  out << std::boolalpha;

  solve_task_1(gallery, out);
  solve_task_2(gallery, out);
  solve_task_3(gallery, out);
  solve_task_4(gallery, out);
  solve_task_5(gallery, out);
  solve_task_6(gallery, out);
  solve_task_7(gallery, out);
  solve_task_8(gallery, out);
}

}

int main(int argc, char** argv) {
  try {
    std::filesystem::path output = "output/automata";

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];

      if (arg == "--help") {
        std::cout << "Usage: hw2_3 [--output DIRECTORY]\n";
        return 0;
      }

      if (arg != "--output" || i + 1 == argc) {
        throw std::invalid_argument{"Expected --output DIRECTORY or --help"};
      }

      output = argv[++i];
    }

    Gallery gallery{output};
    std::ostringstream report;

    solve_homework(gallery, report);
    gallery.finish(report.str());
  } catch (const std::exception& error) {
    std::cerr << "Ошибка: " << error.what() << '\n';
    return 1;
  }
}
