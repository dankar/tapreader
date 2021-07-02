add_library(lib
  tap.h
  tap.cpp
  wav.h
  wav.cpp)

target_link_libraries(lib)
target_include_directories(lib PUBLIC .)
