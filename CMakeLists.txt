cmake_minimum_required(VERSION 3.8)
project(itc2021 LANGUAGES CXX)

list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/easylocal-3/cmake)
list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake)


#if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
#endif()

add_subdirectory(easylocal-3)
add_subdirectory(solver)
#add_subdirectory(cplex)
