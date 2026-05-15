cmake_minimum_required(VERSION 3.28)

project(ProyectoCompleto)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(libspatialindex) # Compila la librería
add_executable(Proyecto main.cpp
        Parser/ast.cpp
        Parser/parser.cpp
        Parser/scanner.cpp
        Parser/token.cpp
        Parser/visitor.cpp
        Index/BPTree.cpp
        Files/SequentialFile.cpp
        Index/ExtendibleHashing.h
        rtree/KNNVisitor.h
        rtree/MiPagedDiskStorageManager.h
        rtree/RangeSearchVisitor.h
        rtree/Rtree_helpers.cpp
        rtree/Rtree_helpers.h
        Parser/disk_counter.cpp
        Parser/disk_counter.h
)
# Para que encuentre los .h
target_include_directories(Proyecto PUBLIC libspatialindex/include)

# Para que encuentre el código de las funciones (el .lib/.a)
target_link_libraries(Proyecto PRIVATE spatialindex)