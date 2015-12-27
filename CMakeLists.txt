cmake_minimum_required(VERSION 3.2)
project(Ebc)

add_definitions(-std=c++11)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fsanitize=address")

file(GLOB SOURCE_FILES "src/*.cpp")
include_directories("include" "src/teee")
add_executable(Ebc "main.cpp" ${SOURCE_FILES}
        include/Constructor.h include/ast/Expr.h include/State.h include/ast/Statement.h
        include/ast/Token.h include/Tokenizer.h include/ast/Type.h include/Checker.h
        include/Builder.h include/Compiler.h include/ast/Item.h)

target_link_libraries(Ebc LLVM-3.4)

add_subdirectory(test)