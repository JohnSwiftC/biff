cmake_minimum_required(VERSION 3.15)
project(biff CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(WARN_FLAGS /W4)
    set(WERROR_FLAG /WX)
else()
    set(WARN_FLAGS -Wall -Wextra)
    set(WERROR_FLAG -Werror)
endif()

add_executable(biffc
    biffc.cpp
    lexer.cpp
    ast.cpp
    parser.cpp
    codegen.cpp
    compiler.cpp
)
target_compile_options(biffc PRIVATE ${WARN_FLAGS})

add_executable(ircompile
    ircompile.cpp
    ir.cpp
)

add_executable(bfopt bfopt.cpp)
target_compile_options(bfopt PRIVATE ${WARN_FLAGS} ${WERROR_FLAG})

add_executable(interpreter interpreter.cpp)
target_compile_options(interpreter PRIVATE ${WARN_FLAGS} ${WERROR_FLAG})
