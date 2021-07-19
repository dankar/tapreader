set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required(VERSION 3.1)

project(tapreader
  LANGUAGES CXX C
  )

include( CheckCXXSourceCompiles )

add_subdirectory(lib)
add_subdirectory(src)
