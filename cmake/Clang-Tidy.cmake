find_program(CLANG_TIDY clang-tidy)

if(CLANG_TIDY)
  set(EXPORT_COMPILE_COMMANDS ON)

  execute_process(
    COMMAND ${CLANG_TIDY} --version
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE CLANG_TIDY_RESULT
    OUTPUT_VARIABLE CLANG_TIDY_VERSION
    ERROR_VARIABLE CLANG_TIDY_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if(CLANG_TIDY_RESULT EQUAL 0)
    message(STATUS "Found clang-tidy: " ${CLANG_TIDY})

    # Check if clang_html Python module is available
    execute_process(
      COMMAND pip3 show clang_html
      RESULT_VARIABLE CLANG_HTML_RESULT
      OUTPUT_QUIET
    )

    # Making CLANG_HTML optional
    if(NOT ${CLANG_HTML_RESULT} EQUAL 0)
      message(WARNING "Clang_html not found, it is required for the visualisation. Install with 'pip3 install clang-html'")
      set(CLANG_HTML OFF)
    else()
      message(STATUS "Found clang_html")
      set(CLANG_HTML ON)
    endif()

    file(GLOB_RECURSE CLANG_TIDY_SOURCES
      "${CMAKE_SOURCE_DIR}/src/*.cpp"
      "${CMAKE_SOURCE_DIR}/src/*.h"
      "${CMAKE_SOURCE_DIR}/src/*.cpph"
      "${CMAKE_BINARY_DIR}/*.cpp"
      "${CMAKE_BINARY_DIR}/*.h"
      "${CMAKE_BINARY_DIR}/*.cpph"
    )

    set(CLANG_TIDY_CHECKS -*,cppcoreguidelines-*,mpi-*,openmp-*,performance-*,-cppcoreguidelines-avoid-magic-numbers,-cppcoreguidelines-pro-bounds-array-to-pointer-decay,-cppcoreguidelines-pro-bounds-pointer-arithmetic)
    add_custom_target(
      clang-tidy
      COMMAND ${CLANG_TIDY} -header-filter=${CMAKE_SOURCE_DIR}/** -checks=${CLANG_TIDY_CHECKS} -p=${CMAKE_BINARY_DIR} ${CLANG_TIDY_SOURCES} | tee clang-tidy.log
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      VERBATIM
      COMMENT "Running clang-tidy: clang-tidy header-filter=${CMAKE_SOURCE_DIR}/** -checks=${CLANG_TIDY_CHECKS} -p=${CMAKE_BINARY_DIR} <All sources...>"
    )
    add_custom_target(
      clang-tidy-fix
      COMMAND ${CLANG_TIDY} -header-filter=${CMAKE_SOURCE_DIR}/** -checks=${CLANG_TIDY_CHECKS} -p=${CMAKE_BINARY_DIR} -fix ${CLANG_TIDY_SOURCES} | tee clang-tidy.log
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      VERBATIM
      COMMENT "Running clang-tidy: clang-tidy header-filter=${CMAKE_SOURCE_DIR}/** -checks=${CLANG_TIDY_CHECKS} -p=${CMAKE_BINARY_DIR} -fix <All sources...>"
    )

    if(CLANG_HTML)
      add_custom_target(
        clang-html
        COMMAND ${Python3_EXECUTABLE} -m clang_html ${CMAKE_BINARY_DIR}/clang-tidy.log
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Visualising clang-tidy output: ${Python3_EXECUTABLE} -m clang_html clang-tidy.log"
        DEPENDS clang-tidy)
    endif()

    add_custom_target(
      clang-tidy-list
      COMMAND ${CLANG_TIDY} -list-checks
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      VERBATIM # Protect arguments to commands
      COMMENT "List all enabled clang-tidy checks.")

    add_custom_target(
      clang-tidy-dump
      COMMAND ${CLANG_TIDY} -dump-config
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      VERBATIM # Protect arguments to commands
      COMMENT "Dumps clang-tidy configuration file.")
  else()
    message(SEND_ERROR "clang-tidy found but cannot retrieve version information")
  endif()
else()
  message(WARNING "clang-tidy requested but executable not found")
endif()
