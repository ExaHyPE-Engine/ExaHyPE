include(CMakeDependentOption)

option(WITH_CLANG_TIDY "Enable clang-tidy as a target" OFF)
cmake_dependent_option(WITH_CLANG_TIDY_FIX "Enable clang-tidy to fix" OFF "WITH_CLANG_TIDY" OFF)

if(WITH_CLANG_TIDY)
  find_program(CLANG_TIDY clang-tidy REQUIRED)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)

  set(EXPORT_COMPILE_COMMANDS ON)

  execute_process(
    COMMAND ${CLANG_TIDY} --version
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE CLANG_TIDY_RESULT
    OUTPUT_VARIABLE CLANG_TIDYVERSION
    ERROR_VARIABLE CLANG_TIDYERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(CLANG_TIDY_RESULT EQUAL 0)
    # Check if clang_html python module is available, otherwise 
    execute_process(
      COMMAND pip show clang_html
      RESULT_VARIABLE EXIT_CODE
      OUTPUT_QUIET)

    # Making CLANG_HTML optional. In case it changes in the future, if there is the module we will add the it as a custom target
    # To visualise the clang-tidy output, otherwise not
    if(NOT ${EXIT_CODE} EQUAL 0)
      message(WARNING "Clang_html not found, it is required for the visualization, can be installed with pip3 install clang-html")
      set(CLANG_HTML OFF)
    else()
      set(CLANG_HTML ON)
    endif()

    # I want the clang-tidy to run on every C++ source file header and cpph, also for the generated files (inside build directory)
    file(
      GLOB_RECURSE
      CLANG_TIDY_SOURCES
      "${CMAKE_SOURCE_DIR}/src/*.cpp"
      "${CMAKE_SOURCE_DIR}/src/*.h"
      "${CMAKE_SOURCE_DIR}/src/*.cpph"
      "${CMAKE_BINARY_DIR}/*.cpp"
      "${CMAKE_BINARY_DIR}/*.h"
      "${CMAKE_BINARY_DIR}/*.cpph")

    # If the user asks for the clang-tidy to fix, then we include -fix and -fix-errors to the options, otherwise empty
    # The argument as list might be problematic in the command, but the list in the Comment string will have ; inside
    # Therefore I decided to define 2 different variables that has the same options in it.
    if (WITH_CLANG_TIDY_FIX)
      set(CLANG_TIDY_FIX_OPTIONS -fix -fix-errors)
      set(CLANG_TIDY_FIX_OPTIONS_STR "-fix -fix-errors")
    else()
      set(CLANG_TIDY_FIX_OPTIONS )
      set(CLANG_TIDY_FIX_OPTIONS_STR "")
    endif()

    # The custom target calls clang-tidy for every source file in the project:
    # It includes every header accross /src/** and /<build_dir>/** 
    # It needs to a compile command database, which is exported by CMake (EXPORT_COMPILE_COMMANDS=ON)
    # It appends the fix options, if the user requests
    # It goes through every cpp, h or templated header (cpph) files both in src and build directory
    set(CLANG_TIDY_CHECKS -*,cppcoreguidelines-*,mpi-*,openmp-*,performance-*,-cppcoreguidelines-avoid-magic-numbers,-cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    add_custom_target(
      Tidy
      COMMAND clang-tidy -header-filter=${CMAKE_SOURCE_DIR}/** -checks=${CLANG_TIDY_CHECKS} -p=${CMAKE_BINARY_DIR} ${CLANG_TIDY_FIX_OPTIONS} ${CLANG_TIDY_SOURCES} | tee clang-tidy.log
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      VERBATIM
      COMMENT "Running clang-tidy: clang-tidy header-filter=${CMAKE_SOURCE_DIR}/** -checks=${CLANG_TIDY_CHECKS} -p=${CMAKE_BINARY_DIR} ${CLANG_TIDY_FIX_OPTIONS_STR} <All sources...>")

    if(CLANG_HTML)
      add_custom_target(
        VisualizeClangTidy
        COMMAND ${Python3_EXECUTABLE} -m clang_html clang-tidy.log
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Visualizing clang-tidy output: ${Python3_EXECUTABLE} -m clang_html clang-tidy.log"
        DEPENDS Tidy)
    endif()

    set_target_properties(Tidy PROPERTIES FOLDER "${META_PROJECT_NAME}/Tools")

    add_custom_target(
      Tidy-List
      COMMAND ${CLANG_TIDY} -list-checks
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      VERBATIM # Protect arguments to commands
      COMMENT "List all enabled clang-tidy checks.")
    set_target_properties(Tidy-List PROPERTIES FOLDER "${META_PROJECT_NAME}/Tools")

    add_custom_target(
      Tidy-Dump
      COMMAND ${CLANG_TIDY} ${CLANG_TIDY_CHECKS} -dump-config
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      VERBATIM # Protect arguments to commands
      COMMENT "Dumps clang-tidy configuration file.")
    set_target_properties(Tidy-Dump PROPERTIES FOLDER "${META_PROJECT_NAME}/Tools")

    function(set_clang_tidy_target target)
      set_property(TARGET ${target} PROPERTY CXX_CLANG_TIDY ${CLANG_TIDY} ${CLANG_TIDY_ARGS})
    endfunction()
  else()
    message(SEND_ERROR "clang-tidy found but cannot retrieve version information")
  endif()
else()
  message(STATUS "clang-tidy requested but executable not found")
endif()