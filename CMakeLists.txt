cmake_minimum_required(VERSION 3.10)

project(LocalGen VERSION 4.3.4 LANGUAGES CXX)

# constants
set(TARGET_NAME LGen)
set(VERSIONINFO_OUTPUT_RC "${PROJECT_BINARY_DIR}/LocalGen.rc")

configure_file(LocalGen.rc.in ${VERSIONINFO_OUTPUT_RC})

# add resource file to project
set_source_files_properties(${VERSIONINFO_OUTPUT_RC} PROPERTIES LANGUAGE RC)

# set include & link directories
include_directories(${CMAKE_SOURCE_DIR}/ege/include)
link_directories(${CMAKE_SOURCE_DIR}/ege/lib)

# add executable target
add_executable(${TARGET_NAME} LGmain.cpp ${VERSIONINFO_OUTPUT_RC})
target_link_libraries(${TARGET_NAME} -static -static-libgcc -mwindows graphics gdi32 gdiplus ws2_32)

# set cxx standard
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_14)

# build number increment
add_custom_target(UpdateBuildNumber ALL
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/increment_build_number.cmake
)

add_dependencies(${TARGET_NAME} UpdateBuildNumber)