option(WITH_INCLUDE_WHAT_YOU_USE "Use Include What You Use (IWYU) to check for unnecessary includes" OFF)

if (WITH_INCLUDE_WHAT_YOU_USE)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    find_program(IWYU_PATH NAMES include-what-you-use iwyu REQUIRED)
    find_program(IWYU_TOOL_PATH NAMES iwyu_tool REQUIRED)
    add_custom_target(iwyu ${iwyu_path}
        COMMAND echo "Call ${IWYU_TOOL_PATH} -v -p ${CMAKE_BINARY_DIR}"
        COMMAND ${IWYU_TOOL_PATH} -v -p ${CMAKE_BINARY_DIR}
        COMMENT "Run iwyo_tool on compilation database"
    )
endif()