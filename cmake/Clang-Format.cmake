find_program(CLANG_FORMAT clang-format)

if(CLANG_FORMAT)
  execute_process(COMMAND ${CLANG_FORMAT} --version
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE CLANG_FORMAT_RESULT
    OUTPUT_VARIABLE CLANG_FORMAT_VERSION
    ERROR_VARIABLE CLANG_FORMAT_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if(CLANG_FORMAT_RESULT EQUAL 0)
    message(STATUS "Found ${CLANG_FORMAT_VERSION}: " ${CLANG_FORMAT})

    file(GLOB_RECURSE CLANG_FORMAT_SOURCES
      ${CMAKE_SOURCE_DIR}/src/*.[ch]pp
      ${CMAKE_SOURCE_DIR}/src/*.cpph
      ${CMAKE_SOURCE_DIR}/src/*.cu
      ${CMAKE_SOURCE_DIR}/src/*.cuh
    )

    if(NOT TARGET clang-format)
      add_custom_target(clang-format
        COMMAND ${CLANG_FORMAT}
          -i
          -style=file
          -fallback-style=none
          -verbose
          ${CLANG_FORMAT_SOURCES}
          SOURCES "${CMAKE_SOURCE_DIR}/.clang-format"
          COMMENT "Format all source files. This may take a while..."
      )
    endif()

    if(NOT TARGET clang-format-check)
      add_custom_target(clang-format-check
        # Use ! to negate the result for correct output
        COMMAND !
          ${CLANG_FORMAT}
            -style=file
            -output-replacements-xml
            -fallback-style=none
            -verbose
            ${CLANG_FORMAT_SOURCES}
            | grep -q "Replacement offset"
            SOURCES "${CMAKE_SOURCE_DIR}/.clang-format"
            COMMENT "Checking clang-format changes."
        )
    endif()

    if(NOT TARGET clang-format-dry)
      add_custom_target(clang-format-dry
        COMMAND
          ${CLANG_FORMAT}
          -style=file
          -dry-run
          -fallback-style=none
          ${CLANG_FORMAT_SOURCES}
          SOURCES "${CMAKE_SOURCE_DIR}/.clang-format"
          COMMENT "Running clang-format in dry mode."
      )
    endif()
  else()
    message(SEND_ERROR "clang-format found but cannot retrieve version information")
  endif()
else()
  message(WARNING "clang-format requested but executable not found")
endif()
