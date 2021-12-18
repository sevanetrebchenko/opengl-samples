
cmake_minimum_required(VERSION 3.12)
project("OpenGL Samples"
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

add_subdirectory(lib)

# Include samples.
# add_subdirectory("${PROJECT_SOURCE_DIR}/src/samples/cube")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/samples/particle")
