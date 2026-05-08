#cmake_minimum_required(VERSION 3.31)
#project(Proyecto)
#
#set(CMAKE_CXX_STANDARD 20)
#
#add_executable(Proyecto main.cpp
#                ast.cpp
#                parser.cpp
#                scanner.cpp
#                token.cpp
#                visitor.cpp
#                BPTree.cpp
#                pruebaBPTree.cpp
#                SequentialFile.cpp
#        ExtendibleHashing.cpp
#        ExtendibleHashing.h)
cmake_minimum_required(VERSION 3.29)

project(ProyectoCompleto)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(libspatialindex)


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
