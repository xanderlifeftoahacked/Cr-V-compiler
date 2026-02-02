cmake_minimum_required(VERSION 3.15)
project(cr_v_compiler)
set(CMAKE_C_STANDARD 11)

if (NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "Should be compiled with GCC")
endif ()

set(TARGET_NAME "crv")
set(TARGET_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(TARGET_SOURCES_NO_MAIN
        ${PROJECT_SOURCE_DIR}/src/lexer/lexer.c
        ${PROJECT_SOURCE_DIR}/src/lexer/token.c
        ${PROJECT_SOURCE_DIR}/src/utils/diagnostic.c
        ${PROJECT_SOURCE_DIR}/src/parser/parser.c
        ${PROJECT_SOURCE_DIR}/src/parser/ast_printer.c
)

add_executable(${TARGET_NAME} ${TARGET_SOURCES_NO_MAIN} ${PROJECT_SOURCE_DIR}/src/compiler/main.c)
target_include_directories(${TARGET_NAME} PRIVATE ${TARGET_INCLUDE_DIR})

if (CMAKE_BUILD_TYPE STREQUAL "")
    message(WARNING "CMAKE_BUILD_TYPE is not set, fallback to debug build")
    set(CMAKE_BUILD_TYPE "Debug")
endif ()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${TARGET_NAME} PRIVATE DEBUG)
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic -fsanitize=address)
    target_link_options(${TARGET_NAME} PRIVATE -fsanitize=address)

    set(TEST_TARGET_NAME "crv_tests")
    add_executable(${TEST_TARGET_NAME} ${TARGET_SOURCES_NO_MAIN} ${PROJECT_SOURCE_DIR}/tests/test_runner.c)
    target_include_directories(${TEST_TARGET_NAME} PRIVATE ${TARGET_INCLUDE_DIR})
    target_compile_definitions(${TEST_TARGET_NAME} PRIVATE TEST_ROOT="${PROJECT_SOURCE_DIR}/tests")

    target_compile_definitions(${TEST_TARGET_NAME} PRIVATE DEBUG)
    target_compile_options(${TEST_TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic -fsanitize=address)
    target_link_options(${TEST_TARGET_NAME} PRIVATE -fsanitize=address)
endif ()
