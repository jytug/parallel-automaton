cmake_minimum_required (VERSION 3.1)
project (MIMUW-PW-AUTOMATON)

set(CMAKE_C_FLAGS "-g -Wall")

macro (add_executable _name)
    # invoke built-in add_executable
    _add_executable(${ARGV})
    if (TARGET ${_name})
        target_link_libraries(${_name} err rt m)
    endif()
endmacro()

set(SHARED_SRC_FILES
    src/config.h
    src/automaton.h
    src/automaton.c
    src/in_out.h
    src/in_out.c
    src/tester_info.h
)

add_library(err src/err.c)

add_executable(validator
               src/validator.c
               ${SHARED_SRC_FILES})

add_executable(tester
               src/tester.c
               ${SHARED_SRC_FILES})

add_executable(run
               src/run.c
               ${SHARED_SRC_FILES})

