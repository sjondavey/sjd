cmake_minimum_required(VERSION 3.24)

# pip install does not use a preset. Take vcpkg from the environment, and do not
# fall through to vcpkg's x64-windows triplet (that one links the static CRT).
# A preset sets both variables, so this block does not replace them.
if(NOT CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg toolchain" FORCE)
endif()
# WIN32 is not set until project(), so the host variables are the ones that exist here.
if(NOT VCPKG_TARGET_TRIPLET AND NOT DEFINED ENV{VCPKG_TARGET_TRIPLET})
    if(CMAKE_HOST_WIN32)
        set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "vcpkg triplet")
    elseif(CMAKE_HOST_UNIX)
        set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "vcpkg triplet")
    endif()
endif()

project(sjd VERSION 0.1.0 LANGUAGES CXX)

# This version is the wheel version (pyproject.toml reads this line), sjd.__version__,
# and build_info.
find_package(Git QUIET)
set(SJD_GIT_COMMIT "unknown")
set(SJD_GIT_DIRTY 0)
if(Git_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE SJD_GIT_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _sjd_git_rc
    )
    if(NOT _sjd_git_rc EQUAL 0 OR SJD_GIT_COMMIT STREQUAL "")
        set(SJD_GIT_COMMIT "unknown")
    endif()
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" status --porcelain
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE _sjd_git_status
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _sjd_git_status_rc
    )
    if(_sjd_git_status_rc EQUAL 0 AND NOT _sjd_git_status STREQUAL "")
        set(SJD_GIT_DIRTY 1)
    endif()
endif()
if(CMAKE_BUILD_TYPE)
    set(SJD_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
else()
    set(SJD_BUILD_TYPE "unknown")
endif()
set(SJD_COMPILER "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
configure_file(cmake/sjd_build_info.h.in "${CMAKE_CURRENT_BINARY_DIR}/sjd_build_info.h" @ONLY)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SJD_BUILD_TESTS "Build the Boost.Test suite" ON)
option(SJD_BUILD_PYTHON "Build Python bindings" ON)
option(SJD_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(SJD_SANITIZERS "Build with AddressSanitizer and UndefinedBehaviorSanitizer (GCC/Clang)" OFF)
option(SJD_COVERAGE "Instrument the library, tests, and Python extension with gcov" OFF)

find_package(QuantLib CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

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
if(SJD_COVERAGE)
    if(MSVC OR SJD_SANITIZERS)
        message(FATAL_ERROR "SJD_COVERAGE is a gcc/clang build and is not combined with sanitizers.")
    endif()
    target_compile_options(sjd_warnings INTERFACE --coverage)
    target_link_options(sjd_warnings INTERFACE --coverage)
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
    target_include_directories(sjd_python PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(sjd_python PRIVATE sjd::sjd sjd_warnings)
    if(MSVC)
        target_compile_options(sjd_python PRIVATE /bigobj)
    endif()

    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c "import pybind11_stubgen"
        RESULT_VARIABLE _sjd_stubgen_rc
    )
    if(NOT _sjd_stubgen_rc EQUAL 0)
        message(FATAL_ERROR
            "pybind11-stubgen is not installed for ${Python3_EXECUTABLE}. "
            "Run: python -m pip install pybind11-stubgen")
    endif()

    # import sjd loads _core from the package directory. The source tree stays
    # free of the binary. Preset tests import this stage. A scikit-build install
    # puts the same binary, and the stub, into the wheel's sjd package.
    set(SJD_PYTHON_STAGE "${CMAKE_BINARY_DIR}/python")
    add_custom_target(sjd_python_stage ALL
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${SJD_PYTHON_STAGE}/sjd"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${PROJECT_SOURCE_DIR}/python/sjd"
            "${SJD_PYTHON_STAGE}/sjd"
        COMMAND "${CMAKE_COMMAND}" -E copy
            "$<TARGET_FILE:sjd_python>"
            "${SJD_PYTHON_STAGE}/sjd/$<TARGET_FILE_NAME:sjd_python>"
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/python/generate_core_stub.py"
            "${SJD_PYTHON_STAGE}" "${PROJECT_SOURCE_DIR}"
        DEPENDS sjd_python
        COMMENT "Staging sjd and generating _core.pyi"
        VERBATIM
    )

    if(SKBUILD)
        # COMPONENT applies only to the artifact group it follows. A Windows
        # module is one of these two, so both groups name the python component.
        install(TARGETS sjd_python
            LIBRARY DESTINATION sjd COMPONENT python
            RUNTIME DESTINATION sjd COMPONENT python)
        install(CODE
            "file(INSTALL \"${SJD_PYTHON_STAGE}/sjd/_core.pyi\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/sjd\")"
            COMPONENT python)
    else()
        add_test(
            NAME sjdPython
            COMMAND "${CMAKE_COMMAND}" -E env
                    "PYTHONPATH=${SJD_PYTHON_STAGE}"
                    "SJD_NO_DEBUG_DIALOGS=1"
                    "${Python3_EXECUTABLE}" -m pytest "${PROJECT_SOURCE_DIR}/python/tests"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        )
    endif()

    message(STATUS "Python bindings: ${Python3_EXECUTABLE}")
    message(STATUS "pybind11: ${pybind11_DIR}")
    message(STATUS "Python stage: ${SJD_PYTHON_STAGE}")
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
