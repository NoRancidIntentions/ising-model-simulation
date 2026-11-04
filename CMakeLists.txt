cmake_minimum_required(VERSION 3.20)
project(ising_model_simulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 3 REQUIRED COMPONENTS Graphics)
find_package(OpenMP REQUIRED COMPONENTS CXX)
find_path(RANDOM123_INCLUDE_DIR NAMES Random123/philox.h REQUIRED)

add_executable(ising_model src/main.cpp)
add_executable(varyingTemp src/varyingTemp.cpp)
target_include_directories(ising_model PRIVATE ${RANDOM123_INCLUDE_DIR})
target_link_libraries(ising_model PRIVATE SFML::Graphics OpenMP::OpenMP_CXX)
