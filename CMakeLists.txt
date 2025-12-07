cmake_minimum_required(VERSION 3.10)

project(MagnusCarlsenMogger)

set(CMAKE_CXX_STANDARD 17)

# Original executable for teacher evaluation (file-based interface)
add_executable(MagnusCarlsenMogger
    src/main.cpp
    src/debugger.cpp
    src/board.cpp
    src/move.cpp
    src/gen.cpp
    src/search.cpp
    src/eval/evaluate.cpp
    src/eval/psqt.cpp
    src/eval/material.cpp
    src/eval/positional.cpp
)

# UCI executable for Elo testing with cutechess-cli
add_executable(MagnusCarlsenMogger_UCI
    test/uci_main.cpp
    src/debugger.cpp
    src/board.cpp
    src/move.cpp
    src/gen.cpp
    src/search.cpp
    src/eval/evaluate.cpp
    src/eval/psqt.cpp
    src/eval/material.cpp
    src/eval/positional.cpp
)
