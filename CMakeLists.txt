cmake_minimum_required(VERSION 3.10)

project(LocalGen)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -O2 -D__DEBUG__")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -O2 -D__DEBUG__")
# set(CMAKE_BUILD_TYPE Release)
# add_compile_options(-O2)
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--verbose")

set(TARGET_NAME LGen)

set_source_files_properties(LocalGen_private.rc PROPERTIES LANGUAGE RC)

include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(${TARGET_NAME} LGmain.cpp LocalGen_private.rc)
target_link_libraries(${TARGET_NAME} -static-libgcc -mwindows graphics gdi32 gdiplus ws2_32)

target_compile_features(${TARGET_NAME} PRIVATE cxx_std_14)