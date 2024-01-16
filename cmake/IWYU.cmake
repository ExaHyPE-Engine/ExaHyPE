find_program(IWYU_PATH NAMES include-what-you-use iwyu)

if(IWYU_PATH)
  find_program(IWYU_TOOL_PATH NAMES iwyu_tool REQUIRED)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  message(STATUS "Found IWYU: " ${IWYU_PATH})

  add_custom_target(iwyu ${iwyu_path}
    COMMAND echo "Call ${IWYU_TOOL_PATH} -v -p ${CMAKE_BINARY_DIR}"
    COMMAND ${IWYU_TOOL_PATH} -v -p ${CMAKE_BINARY_DIR}
    COMMENT "Run IWYU on compilation database"
  )
else()
  message(WARNING "IWYU requested but executable not found")
endif()
