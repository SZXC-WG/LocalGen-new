cmake_minimum_required(VERSION 3.10)

project(LocalGen VERSION 4.2.5)

# constants
set(TARGET_NAME LGen)
set(BUILD_NUMBER 2812)
set(VERSIONINFO_OUTPUT_RC "LocalGen.rc")
string(TIMESTAMP CURRENT_YEAR "%Y")

# configure files
configure_file(LocalGen.rc.in ${VERSIONINFO_OUTPUT_RC})
configure_file(verinfo.h.in verinfo.h)

# add resource file to project
set_source_files_properties(${PROJECT_BINARY_DIR}/${VERSIONINFO_OUTPUT_RC} PROPERTIES LANGUAGE RC)

# set include & link directories
include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${CMAKE_SOURCE_DIR}/lib)

# add executable target
add_executable(${TARGET_NAME} LGmain.cpp ${PROJECT_BINARY_DIR}/${VERSIONINFO_OUTPUT_RC})
target_link_libraries(${TARGET_NAME} -static-libgcc -mwindows graphics gdi32 gdiplus ws2_32)

# set cxx standard
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_14)

target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_BINARY_DIR})