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


add_executable(Proyecto
        main.cpp
        ast.cpp
        parser.cpp
        scanner.cpp
        token.cpp
        visitor.cpp
        BPTree.cpp
        pruebas/pruebaBPTree.cpp
        SequentialFile.cpp
        ExtendibleHashing.h
)

target_compile_features(Proyecto PRIVATE cxx_std_20)


add_executable(rtree
        ../pruebas/pruebaRTree.cpp
        ../RangeSearchVisitor.h
        ../KNNVisitor.h
        ../MiPagedDiskStorageManager.h
)

target_include_directories(rtree PRIVATE
        libspatialindex/include
)

target_link_libraries(rtree PRIVATE spatialindex)

target_compile_features(rtree PRIVATE cxx_std_17)