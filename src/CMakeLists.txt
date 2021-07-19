
add_custom_command(OUTPUT "${PROJECT_SOURCE_DIR}/src/cmdline.c" "${PROJECT_SOURCE_DIR}/src/cmdline.h"
  COMMAND "gengetopt" "-ioptions.ggo" "--unnamed-opts=TAP_FILES" "--long-help" "--show-required"
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/src/"
  DEPENDS "${PROJECT_SOURCE_DIR}/src/options.ggo")

add_executable(tapreader
  main.cpp
  cmdline.c
  cmdline.h)

target_link_libraries(tapreader lib)
