cmake_minimum_required(VERSION 3.21)
project(confetti LANGUAGES C VERSION 1.0.0)

include(CMakePackageConfigHelpers)

# Subdirectory with Confetti source files.
set(CONFETTI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/confetti")

find_package(Python REQUIRED COMPONENTS Interpreter Development)

if (NOT EXISTS "${CONFETTI_DIR}/confetti_unidata.c")
    message(FATAL_ERROR "Please run unicode.py before configuring the project! Alternatively, build from the release tarball (instead of repo checkout) to skip this step.")
endif()

# C11 is required to build Confetti.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

set(PY_SOURCE
    ${CONFETTI_DIR}/confetti.c
    ${CONFETTI_DIR}/confetti_unidata.c
    ${CONFETTI_DIR}/confetti.h
    src/py_parse.c
    src/py_argument.c
    src/py_confetti.c
    src/py_confetti.h
    src/py_directive.c
    src/py_exception.c)

add_library(cconfetti SHARED ${PY_SOURCE})
target_link_libraries(cconfetti PUBLIC Python::Python)
target_include_directories(cconfetti PUBLIC src)
target_include_directories(cconfetti PUBLIC ${CONFETTI_DIR})
target_compile_definitions(cconfetti PUBLIC $<$<CONFIG:Debug>:DEBUG>)

if (NOT SKBUILD)
    # Compile an executable that behaves like the Python interpreter
    # except with the C extension pre-registered. This is for testing.
    add_executable(pymain src/py_main.c)
    target_link_libraries(pymain cconfetti)
endif()
