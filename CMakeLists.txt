cmake_minimum_required(VERSION 3.12)
set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VERSION 0.0.${PATCH_VERSION})
project(rm_nuisance VERSION ${PROJECT_VERSION})

set(SRC_DIR "./src/")
set(INC_DIR "./inc/")
set(TEST_DIR "./test/")

file(GLOB_RECURSE SRC_FILES "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE TEST_FILES "${TEST_DIR}/*.cpp")

# main exe
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INC_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror -g)
endif()

# tests
set(GTEST_DIR "${TEST_DIR}/googletest")
add_subdirectory(${GTEST_DIR} ${CMAKE_BINARY_DIR}/googletest EXCLUDE_FROM_ALL)
enable_testing()

add_executable(${PROJECT_NAME}_tests ${TEST_FILES})
target_include_directories(${PROJECT_NAME}_tests PRIVATE
    ${INC_DIR}
    ${GTEST_DIR}/googletest/include
)
target_link_libraries(${PROJECT_NAME}_tests PRIVATE gtest gtest_main)
set_target_properties(${PROJECT_NAME}_tests PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)
if (MSVC)
    target_compile_options(${PROJECT_NAME}_tests PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME}_tests PRIVATE -Wall -Wextra -pedantic -Werror -g)
endif()

add_test(NAME ${PROJECT_NAME}_tests COMMAND ${PROJECT_NAME}_tests)

# installing
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT vasuyan4ik@gmail.com)
include(CPack)