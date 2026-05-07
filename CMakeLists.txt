cmake_minimum_required(VERSION 3.31)
project(Proyecto)

set(CMAKE_CXX_STANDARD 20)

add_executable(Proyecto main.cpp
        Parser/ast.cpp
        Parser/parser.cpp
        Parser/scanner.cpp
        Parser/token.cpp
        Parser/visitor.cpp
        Index/BPTree.cpp
        pruebas/pruebaBPTree.cpp
        Files/SequentialFile.cpp
        Index/ExtendibleHashing.h
        pruebas/pruebaEHashing.cpp)
