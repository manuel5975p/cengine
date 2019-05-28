cmake_minimum_required(VERSION 3.6)
if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
   message(FATAL_ERROR "In-source builds are not allowed.")
endif("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
#set(CMAKE_DISABLE_SOURCE_CHANGES ON)
#set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
project(Cengine)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)
file(GLOB cengine_src "src/*.hpp" "src/*.cpp")

try_compile(POPCNT "${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/checks/popcount_support.cpp" COMPILE_DEFINITIONS -march=native)
set(BMI_MACROS "")
if(POPCNT)
	message("Supports popcnt...")
	set (BMI_MACROS "${BMI_MACROS} -DUSE_POPCOUNT")
else()
	message("Doesn't support popcnt...")
endif()

try_compile(PEXT "${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/checks/pext_support.cpp" COMPILE_DEFINITIONS -march=native)

if(PEXT)
	message("Supports pext...")
	set (BMI_MACROS "${BMI_MACROS} -DUSE_PEXT")
else()
	message("Doesn't support pext...")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${BMI_MACROS} -Wall -Wextra -pedantic -march=native -flto")

add_executable(cengine ${cengine_src})
