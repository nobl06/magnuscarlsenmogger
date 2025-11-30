cmake_minimum_required(VERSION 3.10)

project(MagnusCarlsenMogger)

set(CMAKE_CXX_STANDARD 17)

add_executable(MagnusCarlsenMogger
    src/main.cpp
    src/debugger.cpp
    src/board.cpp
    src/move.cpp
    src/gen.cpp
)

# Perft test executable
add_executable(perft_test
    test/perft.cpp
    src/board.cpp
    src/move.cpp
    src/gen.cpp
)

# Unit test executable
add_executable(unit_test
    test/unit_test.cpp
    src/board.cpp
    src/move.cpp
    src/gen.cpp
)