cmake_minimum_required(VERSION 3.6)

file(GLOB cengine_src "src/*.hpp" "src/*.cpp")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -Wextra -pedantic -march=native -flto")

add_executable(cengine ${cengine_src})
