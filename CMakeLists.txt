cmake_minimum_required(VERSION 3.24)
project(sjd VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SJD_BUILD_TESTS "Build the Boost.Test suite" ON)
option(SJD_BUILD_PYTHON "Build Python bindings" ON)
option(SJD_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(SJD_SANITIZERS "Build with AddressSanitizer and UndefinedBehaviorSanitizer (GCC/Clang)" OFF)

find_package(QuantLib CONFIG REQUIRED)

include(GNUInstallDirs)

# Keep binaries in the top build directory (build/debug/sjdTest.exe), not under src/ or tests/.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# Warning and sanitizer flags for targets built from this repository. Not exported.
add_library(sjd_warnings INTERFACE)
if(MSVC)
    target_compile_options(sjd_warnings INTERFACE /W4 /permissive-)
    if(SJD_WARNINGS_AS_ERRORS)
        target_compile_options(sjd_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(sjd_warnings INTERFACE -Wall -Wextra -Wpedantic)
    if(SJD_WARNINGS_AS_ERRORS)
        target_compile_options(sjd_warnings INTERFACE -Werror)
    endif()
    if(SJD_SANITIZERS)
        target_compile_options(sjd_warnings INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(sjd_warnings INTERFACE -fsanitize=address,undefined)
    endif()
endif()

add_subdirectory(src)

enable_testing()
if(SJD_BUILD_TESTS)
    find_package(Boost REQUIRED COMPONENTS unit_test_framework)
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# sjd Python module (pybind11). Target cannot be named "sjd" — that is the lib.
# pybind11 is taken from the interpreter's pip install unless pybind11_DIR is set.
# ---------------------------------------------------------------------------
if(SJD_BUILD_PYTHON)
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter Development.Module)

    if(NOT pybind11_DIR)
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" -m pybind11 --cmakedir
            OUTPUT_VARIABLE _sjd_pybind11_dir
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE _sjd_pybind11_rc
        )
        if(NOT _sjd_pybind11_rc EQUAL 0)
            message(FATAL_ERROR "pybind11 is not installed for ${Python3_EXECUTABLE}. Run: python -m pip install pybind11")
        endif()
        set(pybind11_DIR "${_sjd_pybind11_dir}" CACHE PATH "pybind11 CMake package directory")
    endif()
    set(PYBIND11_FINDPYTHON ON)
    find_package(pybind11 CONFIG REQUIRED)

    pybind11_add_module(sjd_python MODULE python/sjd_module.cpp)
    set_target_properties(sjd_python PROPERTIES OUTPUT_NAME _core)
    target_link_libraries(sjd_python PRIVATE sjd::sjd sjd_warnings)
    if(MSVC)
        target_compile_options(sjd_python PRIVATE /bigobj)
    endif()

    # The compiled core lives inside the source package, so the editable install
    # (pip install -e python) and the tests import the same file for Debug and Release.
    set(SJD_PYTHON_PACKAGE_DIR "${PROJECT_SOURCE_DIR}/python/sjd")
    add_custom_command(TARGET sjd_python POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:sjd_python>"
            "${SJD_PYTHON_PACKAGE_DIR}/$<TARGET_FILE_NAME:sjd_python>"
        COMMENT "Staging sjd._core into python/sjd"
        VERBATIM
    )

    add_test(
        NAME sjdPython
        COMMAND "${CMAKE_COMMAND}" -E env
                "PYTHONPATH=${PROJECT_SOURCE_DIR}/python"
                "SJD_NO_DEBUG_DIALOGS=1"
                "${Python3_EXECUTABLE}" -m pytest "${PROJECT_SOURCE_DIR}/python/tests"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/python"
    )

    message(STATUS "Python bindings: ${Python3_EXECUTABLE}")
    message(STATUS "pybind11: ${pybind11_DIR}")
    message(STATUS "Python package: ${SJD_PYTHON_PACKAGE_DIR}")
endif()

# ---------------------------------------------------------------------------
# Install and export, so other CMake projects can find_package(sjd).
# ---------------------------------------------------------------------------
include(CMakePackageConfigHelpers)

install(TARGETS sjd EXPORT sjdTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/sjd DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT sjdTargets
    NAMESPACE sjd::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sjd
)
configure_package_config_file(
    cmake/sjdConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/sjdConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sjd
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/sjdConfigVersion.cmake"
    COMPATIBILITY SameMinorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/sjdConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/sjdConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sjd
)
