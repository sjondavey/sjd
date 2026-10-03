cmake_minimum_required(VERSION 3.24)
project(sjd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(QuantLib CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

# Keep binaries in the top build directory (build/debug/sjdTest.exe), not under src/ or tests/.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)

# ---------------------------------------------------------------------------
# sjd Python module (pybind11). Target cannot be named "sjd" — that is the lib.
# Official CPython has no debug .lib; ignore python310_d.lib in Debug.
# ---------------------------------------------------------------------------
option(SJD_BUILD_PYTHON "Build Python bindings" ON)

if(SJD_BUILD_PYTHON)
    if(NOT Python3_EXECUTABLE)
        find_program(Python3_EXECUTABLE NAMES python python3 REQUIRED)
    endif()
    get_filename_component(_sjd_python_root "${Python3_EXECUTABLE}" DIRECTORY)

    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c "import pybind11; print(pybind11.get_include())"
        OUTPUT_VARIABLE _sjd_pybind11_include
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _sjd_pybind11_rc
    )
    if(NOT _sjd_pybind11_rc EQUAL 0)
        message(FATAL_ERROR "pybind11 is not installed for ${Python3_EXECUTABLE}. Run: python -m pip install pybind11")
    endif()

    add_library(sjd_python MODULE python/sjd_module.cpp)
    set_target_properties(sjd_python PROPERTIES
        PREFIX ""
        OUTPUT_NAME sjd
        SUFFIX ".pyd"
        ARCHIVE_OUTPUT_NAME sjd_python
    )
    target_include_directories(sjd_python PRIVATE
        "${_sjd_python_root}/Include"
        "${_sjd_pybind11_include}"
    )
    target_link_libraries(sjd_python PRIVATE
        sjd::sjd
        "${_sjd_python_root}/libs/python310.lib"
    )
    if(MSVC)
        target_compile_options(sjd_python PRIVATE /W3 /bigobj)
        target_link_options(sjd_python PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:python310_d.lib>)
    endif()

    # One staging folder for Debug and Release so notebooks never import from build/.
    set(SJD_PYTHON_STAGING_DIR "${CMAKE_SOURCE_DIR}/python/sjd_runtime")
    add_custom_command(TARGET sjd_python POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SJD_PYTHON_STAGING_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:sjd_python>"
            "${SJD_PYTHON_STAGING_DIR}/$<TARGET_FILE_NAME:sjd_python>"
        COMMENT "Staging sjd Python module to python/sjd_runtime"
        VERBATIM
    )

    add_test(
        NAME sjdPythonValidation
        COMMAND "${CMAKE_COMMAND}" -E env
                "SJD_PYTHON_MODULE_DIR=${SJD_PYTHON_STAGING_DIR}"
                "SJD_NO_DEBUG_DIALOGS=1"
                "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/python/tests/test_validation.py"
    )
    add_test(
        NAME sjdForwardCurve
        COMMAND "${CMAKE_COMMAND}" -E env
                "SJD_PYTHON_MODULE_DIR=${SJD_PYTHON_STAGING_DIR}"
                "SJD_NO_DEBUG_DIALOGS=1"
                "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/python/tests/test_forward_curve.py"
    )
    add_test(
        NAME sjdMarketSpecs
        COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/python/tests/test_market_specs.py"
    )
    add_test(
        NAME sjdMarketSchema
        COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/python/tests/test_market_schema.py"
    )
    add_test(
        NAME sjdMarketSample
        COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/python/tests/test_market_sample.py"
    )
    add_test(
        NAME sjdMarketRates
        COMMAND "${CMAKE_COMMAND}" -E env
                "SJD_PYTHON_MODULE_DIR=${SJD_PYTHON_STAGING_DIR}"
                "SJD_NO_DEBUG_DIALOGS=1"
                "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/python/tests/test_market_rates.py"
    )
    add_test(
        NAME sjdSimulate
        COMMAND "${CMAKE_COMMAND}" -E env
                "SJD_PYTHON_MODULE_DIR=${SJD_PYTHON_STAGING_DIR}"
                "SJD_NO_DEBUG_DIALOGS=1"
                "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/python/tests/test_simulate.py"
    )

    message(STATUS "Python bindings: ${Python3_EXECUTABLE}")
    message(STATUS "pybind11 include: ${_sjd_pybind11_include}")
    message(STATUS "Python module staging: ${SJD_PYTHON_STAGING_DIR}")
endif()
