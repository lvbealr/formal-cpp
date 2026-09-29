cmake_minimum_required(VERSION 3.20)
project(formal_cpp LANGUAGES CXX)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(nlohmann_json 3.11 QUIET)
if(NOT nlohmann_json_FOUND)
  include(FetchContent)
  FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d)

  FetchContent_MakeAvailable(nlohmann_json)
endif()

add_library(automata
  automata/src/core/models.cpp
  automata/src/core/graph.cpp

  automata/src/algorithms/constructions.cpp
  automata/src/algorithms/conversions.cpp
  automata/src/algorithms/minimization.cpp
  automata/src/algorithms/analysis.cpp

  automata/src/regex/regex.cpp

  automata/src/io/export.cpp
  automata/src/io/serialization.cpp)

target_include_directories(automata PUBLIC automata/inc)
target_compile_features(automata PUBLIC cxx_std_20)
target_link_libraries(automata PUBLIC nlohmann_json::nlohmann_json)

if(MSVC)
  target_compile_options(automata PUBLIC /utf-8 PRIVATE /W4)
else()
  target_compile_options(automata PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(homework solutions/tasks.cpp)
target_include_directories(homework PUBLIC solutions)
target_link_libraries(homework PUBLIC automata)

add_executable(hw2_3 solutions/hw2_3.cpp)
target_link_libraries(hw2_3 PRIVATE homework)

include(CTest)
option(AUTOMATA_TEST_RENDERING "Test the Python renderer (requires requirements.txt)" OFF)

if(BUILD_TESTING)
  add_executable(test_automata tests/test_automata.cpp)
  target_link_libraries(test_automata PRIVATE homework)

  add_test(NAME automata COMMAND test_automata)
  add_test(NAME homework_export COMMAND hw2_3 --output ${CMAKE_CURRENT_BINARY_DIR}/test-output)
  set_tests_properties(homework_export PROPERTIES FIXTURES_SETUP homework_data)

  if(AUTOMATA_TEST_RENDERING)
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)

    add_test(NAME rendering COMMAND ${Python3_EXECUTABLE} -m tests.test_rendering
      --exports ${CMAKE_CURRENT_BINARY_DIR}/test-output)

    set_tests_properties(rendering PROPERTIES
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      FIXTURES_REQUIRED homework_data ENVIRONMENT "MPLBACKEND=Agg")
  endif()
endif()
