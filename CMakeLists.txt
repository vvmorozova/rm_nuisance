cmake_minimum_required(VERSION 3.12)
set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VERSION 0.0.${PATCH_VERSION})
project(rm_nuisance VERSION ${PROJECT_VERSION})

set(SRC_DIR "./src/")
set(INC_DIR "./inc/")
set(TEST_DIR "./test/")

file(GLOB_RECURSE SRC_FILES "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE TEST_FILES "${TEST_DIR}/*.cpp")

option(WHISPER_OPENBLAS OFF)
option(WHISPER_METAL OFF)

set(WHISPER_DIR "${CMAKE_SOURCE_DIR}/external/whisper.cpp")

if(NOT EXISTS "${WHISPER_DIR}/CMakeLists.txt")
  message(FATAL_ERROR "whisper.cpp not found at ${WHISPER_DIR}")
endif()

add_subdirectory(${WHISPER_DIR} whisper_build EXCLUDE_FROM_ALL)

list(FILTER SRC_FILES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(${PROJECT_NAME}_lib STATIC ${SRC_FILES})
target_include_directories(${PROJECT_NAME}_lib PUBLIC
    ${INC_DIR}
    "${WHISPER_DIR}"
    "${WHISPER_DIR}/include"
    "${WHISPER_DIR}/ggml/include"
)

set_target_properties(${PROJECT_NAME}_lib PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if (MSVC)
    target_compile_options(${PROJECT_NAME}_lib PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME}_lib PRIVATE -Wall -Wextra -pedantic -Werror -g)
endif()

target_link_libraries(${PROJECT_NAME}_lib PUBLIC whisper)

add_executable(${PROJECT_NAME} ${SRC_DIR}/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${PROJECT_NAME}_lib
    whisper
)

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

set(GTEST_DIR "${TEST_DIR}/googletest")
add_subdirectory(${GTEST_DIR} ${CMAKE_BINARY_DIR}/googletest EXCLUDE_FROM_ALL)

enable_testing()

add_executable(${PROJECT_NAME}_tests ${TEST_FILES})
target_include_directories(${PROJECT_NAME}_tests PRIVATE
    ${INC_DIR}
    ${GTEST_DIR}/googletest/include
)

target_link_libraries(${PROJECT_NAME}_tests PRIVATE
    ${PROJECT_NAME}_lib
    whisper
    gtest
    gtest_main
)

set_target_properties(${PROJECT_NAME}_tests PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

if (MSVC)
    target_compile_options(${PROJECT_NAME}_tests PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME}_tests PRIVATE -Wall -Wextra -pedantic -Werror -O3 -march=native -g)
endif()

add_test(NAME ${PROJECT_NAME}_tests COMMAND ${PROJECT_NAME}_tests)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT vasuyan4ik@gmail.com)

include(CPack)

# clean target

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E rm -rf
        ${CMAKE_BINARY_DIR}/CMakeCache.txt
        ${CMAKE_BINARY_DIR}/CMakeFiles
        ${CMAKE_BINARY_DIR}/Makefile
        ${CMAKE_BINARY_DIR}/cmake_install.cmake
        ${CMAKE_BINARY_DIR}/CPackConfig.cmake
        ${CMAKE_BINARY_DIR}/CPackSourceConfig.cmake
        ${CMAKE_BINARY_DIR}/_CPack_Packages
        ${CMAKE_BINARY_DIR}/googletest
        ${CMAKE_BINARY_DIR}/whisper_build
    COMMAND ${CMAKE_COMMAND} -E echo "Full clean done"
)