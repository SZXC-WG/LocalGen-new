# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\LocalGen_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\LocalGen_autogen.dir\\ParseCache.txt"
  "LocalGen_autogen"
  )
endif()
