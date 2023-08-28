include(CMakeParseArguments)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(WARNING "Code coverage results with an optimized (non-Debug) build may be misleading")
    endif()

    find_program(GCOV_PATH gcov)
    find_program(LCOV_PATH lcov NAMES lcov lcov.bat lcov.exe lcov.perl)
    find_program(GENHTML_PATH genhtml NAMES genhtml genhtml.perl genhtml.bat)
    find_program(GCOVR_PATH gcovr)
    find_program(CPPFILT_PATH NAMES c++filt)
    if(NOT GCOV_PATH)
        message(FATAL_ERROR "gcov not found! Aborting...")
    endif()

    function(add_coverage)
        set(options NO_DEMANGLE)
        set(oneValueArgs BASE_DIRECTORY NAME)
        set(multiValueArgs EXCLUDE EXECUTABLE EXECUTABLE_ARGS DEPENDENCIES LCOV_ARGS GENHTML_ARGS)
        cmake_parse_arguments(Coverage "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

        if(NOT LCOV_PATH)
            message(FATAL_ERROR "lcov not found! Aborting...")
        endif()
        if(NOT GENHTML_PATH)
            message(FATAL_ERROR "genhtml not found! Aborting...")
        endif()

        # Set base directory (as absolute path), or default to PROJECT_SOURCE_DIR
        if(${Coverage_BASE_DIRECTORY})
            get_filename_component(BASEDIR ${Coverage_BASE_DIRECTORY} ABSOLUTE)
        else()
            set(BASEDIR ${PROJECT_SOURCE_DIR})
        endif()

        # Collect excludes
        set(LCOV_EXCLUDES "")
        foreach(EXCLUDE ${Coverage_EXCLUDE} ${COVERAGE_EXCLUDES} ${COVERAGE_LCOV_EXCLUDES})
            get_filename_component(EXCLUDE ${EXCLUDE} ABSOLUTE BASE_DIR ${BASEDIR})
            list(APPEND LCOV_EXCLUDES "${EXCLUDE}")
        endforeach()
        list(REMOVE_DUPLICATES LCOV_EXCLUDES)

         # Conditional arguments
        if(CPPFILT_PATH AND NOT ${Coverage_NO_DEMANGLE})
            set(GENHTML_EXTRA_ARGS "--demangle-cpp")
        endif()

        add_custom_target(${Coverage_NAME}
            # Cleanup lcov
            COMMAND ${LCOV_PATH} ${Coverage_LCOV_ARGS} --gcov-tool ${GCOV_PATH} -directory . --base-directory ${BASEDIR} --zerocounters
            # Need to create an lcov "baseline" before running any tests.
            # The result is a coverage data file that contains zero coverage for every instrumented line of the project.
            # At a later stage, this data file will be combined with coverage data files captured after the test run.
            # This way the percentage of total lines covered will always be correct, even when not all source code files were loaded during the test(s).
            COMMAND ${LCOV_PATH} ${Coverage_LCOV_ARGS} --gcov-tool ${GCOV_PATH} --capture --initial --directory . --base-directory ${BASEDIR} -o ${Coverage_NAME}.base

            # Run tests
            COMMAND ${Coverage_EXECUTABLE} ${Coverage_EXECUTABLE_ARGS}

            # Capturing lcov counters and generating report
            COMMAND ${LCOV_PATH} ${Coverage_LCOV_ARGS} --gcov-tool ${GCOV_PATH} --directory . --base-directory ${BASEDIR} --capture --output-file ${Coverage_NAME}.capture
            # Add baseline counters
            COMMAND ${LCOV_PATH} ${Coverage_LCOV_ARGS} --gcov-tool ${GCOV_PATH} --add-tracefile ${Coverage_NAME}.base --add-tracefile ${Coverage_NAME}.capture --output-file ${Coverage_NAME}.total
            # Filter collected data to final coverage report
            COMMAND ${LCOV_PATH} ${Coverage_LCOV_ARGS} --gcov-tool ${GCOV_PATH} --remove ${Coverage_NAME}.total ${LCOV_EXCLUDES} --output-file ${Coverage_NAME}.info
            COMMAND ${LCOV_PATH} ${Coverage_LCOV_ARGS} --gcov-tool ${GCOV_PATH} --list ${Coverage_NAME}.info

            # Generate HTML output
            COMMAND ${GENHTML_PATH} ${GENHTML_EXTRA_ARGS} ${Coverage_GENHTML_ARGS} -o ${Coverage_NAME} ${Coverage_NAME}.info

            # Set output files as GENERATED (will be removed on 'make clean')
            BYPRODUCTS
                ${PROJECT_BINARY_DIR}/${Coverage_NAME}.base
                ${PROJECT_BINARY_DIR}/${Coverage_NAME}.capture
                ${PROJECT_BINARY_DIR}/${Coverage_NAME}.total
                ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info
                ${PROJECT_BINARY_DIR}/${Coverage_NAME}

            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            DEPENDS ${Coverage_DEPENDENCIES}
            VERBATIM # Protect arguments to commands
            COMMENT "Resetting code coverage counters to zero.\nProcessing code coverage counters and generating report."
        )

        # Show where to find the lcov info report
        add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
            COMMAND ;
            COMMENT "Lcov code coverage info report saved in ${Coverage_NAME}.info."
        )

        # Show info where to find the report
        add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
            COMMAND ;
            COMMENT "Open ./${Coverage_NAME}/index.html in your browser to view the coverage report."
        )
    endfunction()

    function(add_coverage_cobertura)
        set(options "")
        set(oneValueArgs BASE_DIRECTORY NAME)
        set(multiValueArgs EXECUTABLE EXECUTABLE_ARGS DEPENDENCIES)
        cmake_parse_arguments(Coverage "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

        if(NOT GCOVR_PATH)
            message(FATAL_ERROR "gcovr not found! Aborting...")
        endif()

        # Set base directory (as absolute path), or default to PROJECT_SOURCE_DIR
        if(${Coverage_BASE_DIRECTORY})
            get_filename_component(BASEDIR ${Coverage_BASE_DIRECTORY} ABSOLUTE)
        else()
            set(BASEDIR ${PROJECT_SOURCE_DIR})
        endif()

        # Collect excludes
        set(GCOVR_EXCLUDES "")
        foreach(EXCLUDE ${Coverage_EXCLUDE} ${COVERAGE_EXCLUDES} ${COVERAGE_GCOVR_EXCLUDES})
            get_filename_component(EXCLUDE ${EXCLUDE} ABSOLUTE BASE_DIR ${BASEDIR})
            list(APPEND GCOVR_EXCLUDES "${EXCLUDE}")
        endforeach()
        list(REMOVE_DUPLICATES GCOVR_EXCLUDES)

        # Combine excludes to several -e arguments
        set(GCOVR_EXCLUDE_ARGS "")
        foreach(EXCLUDE ${GCOVR_EXCLUDES})
            list(APPEND GCOVR_EXCLUDE_ARGS "-e")
            list(APPEND GCOVR_EXCLUDE_ARGS "${EXCLUDE}")
        endforeach()

        add_custom_target(${Coverage_NAME}
            # Run tests
            ${Coverage_EXECUTABLE} ${Coverage_EXECUTABLE_ARGS}

            # Running gcovr
            COMMAND ${GCOVR_PATH} --xml
                -r ${BASEDIR} ${GCOVR_EXCLUDE_ARGS}
                --object-directory=${PROJECT_BINARY_DIR}
                -o ${Coverage_NAME}.xml
            BYPRODUCTS ${PROJECT_BINARY_DIR}/${Coverage_NAME}.xml
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            DEPENDS ${Coverage_DEPENDENCIES}
            VERBATIM # Protect arguments to commands
            COMMENT "Running gcovr to produce Cobertura code coverage report."
        )

        # Show info where to find the report
        add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
            COMMAND ;
            COMMENT "Cobertura code coverage report saved in ${Coverage_NAME}.xml."
        )
    endfunction()

    function(add_coverage_gcovr)
        set(options "")
        set(oneValueArgs BASE_DIRECTORY NAME)
        set(multiValueArgs EXECUTABLE EXECUTABLE_ARGS DEPENDENCIES)
        cmake_parse_arguments(Coverage "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

        if(NOT GCOVR_PATH)
            message(FATAL_ERROR "gcovr not found! Aborting...")
        endif()

        # Set base directory (as absolute path), or default to PROJECT_SOURCE_DIR
        if(${Coverage_BASE_DIRECTORY})
            get_filename_component(BASEDIR ${Coverage_BASE_DIRECTORY} ABSOLUTE)
        else()
            set(BASEDIR ${PROJECT_SOURCE_DIR})
        endif()

        # Collect excludes
        set(GCOVR_EXCLUDES "")
        foreach(EXCLUDE ${Coverage_EXCLUDE} ${COVERAGE_EXCLUDES} ${COVERAGE_GCOVR_EXCLUDES})
            get_filename_component(EXCLUDE ${EXCLUDE} ABSOLUTE BASE_DIR ${BASEDIR})
            list(APPEND GCOVR_EXCLUDES "${EXCLUDE}")
        endforeach()
        list(REMOVE_DUPLICATES GCOVR_EXCLUDES)

        # Combine excludes to several -e arguments
        set(GCOVR_EXCLUDE_ARGS "")
        foreach(EXCLUDE ${GCOVR_EXCLUDES})
            list(APPEND GCOVR_EXCLUDE_ARGS "-e")
            list(APPEND GCOVR_EXCLUDE_ARGS "${EXCLUDE}")
        endforeach()

        add_custom_target(${Coverage_NAME}
            # Run tests
            ${Coverage_EXECUTABLE} ${Coverage_EXECUTABLE_ARGS}

            # Create folder
            COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/${Coverage_NAME}

            # Running gcovr
            COMMAND ${GCOVR_PATH} --html --html-details
                -r ${BASEDIR} ${GCOVR_EXCLUDE_ARGS}
                --object-directory=${PROJECT_BINARY_DIR}
                -o ${Coverage_NAME}/index.html
            BYPRODUCTS ${PROJECT_BINARY_DIR}/${Coverage_NAME}
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            DEPENDS ${Coverage_DEPENDENCIES}
            VERBATIM # Protect arguments to commands
            COMMENT "Running gcovr to produce HTML code coverage report."
        )

        # Show info where to find the report
        add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
            COMMAND ;
            COMMENT "Open ./${Coverage_NAME}/index.html in your browser to view the coverage report."
        )
    endfunction()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    include(FetchContent)
    FetchContent_Declare(
        ReportGenerator
        URL https://github.com/danielpalme/ReportGenerator/releases/download/v4.5.8/ReportGenerator_4.5.8.zip
    )

    FetchContent_GetProperties(ReportGenerator)
    if(NOT ReportGenerator)
        FetchContent_Populate(ReportGenerator)
        set(CPP_COVERAGE_REPORT_GENERATOR_TOOL "${FETCHCONTENT_BASE_DIR}/reportgenerator-src${ReportGenerator_SOURCE_DIR}/net47/ReportGenerator.exe")
    endif()

    function(add_coverage)
        cmake_parse_arguments(
            Coverage
            "REPORT_FOR_TARGET;REPORT_FOR_PROJECT;CONTINUE_AFTER_CPP_EXCEPTION;COVER_CHILDREN;VERBOSE;QUIET"
            "TARGET;EXCLUDED_LINE_REGEX"
            "SOURCES;MODULES;EXCLUDED_SOURCES;EXCLUDED_MODULES;DEPENDENCIES;TARGET_ARGS"
            ${ARGN}
        )

        # Build OpenCppCoverage base command
        # See https://github.com/OpenCppCoverage/OpenCppCoverage/wiki/Command-line-reference
        if(Coverage_CONTINUE_AFTER_CPP_EXCEPTION)
            list(APPEND Coverage_COVERAGE_ARGS "continue_after_cpp_exception=1")
        endif()
        if(Coverage_COVER_CHILDREN)
            list(APPEND Coverage_COVERAGE_ARGS "cover_children=1")
        endif()
        if(Coverage_NO_AGGERGATE_BY_FILE)
            list(APPEND Coverage_COVERAGE_ARGS "no_aggregate_by_file=1")
        endif()
        if(Coverage_EXCLUDED_LINE_REGEX)
            list(APPEND Coverage_COVERAGE_ARGS "excluded_line_regex=${Coverage_EXCLUDED_LINE_REGEX}")
        endif()
        if(Coverage_VERBOSE AND Coverage_QUIET)
            message(FATAL_ERROR "Both VERBOSE and QUIET specified. Only one may be specified.")
        endif()
        if(Coverage_VERBOSE)
            list(APPEND Coverage_CLI_ARGS "--verbose")
        endif()
        if(Coverage_QUIET)
            list(APPEND Coverage_CLI_ARGS "--quiet")
        endif()

        set(Coverage_BINARY_OUTPUT_FILE "${PROJECT_BINARY_DIR}/${Coverage_TARGET}.cov")
        file(TO_NATIVE_PATH ${Coverage_BINARY_OUTPUT_FILE} Coverage_NATIVE_BINARY_OUTPUT_FILE)
        set(Coverage_CONFIG_INPUT_FILE "${PROJECT_BINARY_DIR}/${Coverage_TARGET}.cov.txt")
        file(TO_NATIVE_PATH ${Coverage_CONFIG_INPUT_FILE} Coverage_CONFIG_INPUT_FILE)

        # Setup coverage report target for individual test target
        get_target_property(TARGET_BINARY_DIR ${Coverage_TARGET} BINARY_DIR)
        file(TO_NATIVE_PATH ${TARGET_BINARY_DIR} TARGET_BINARY_DIR)

        # Create native path arguments for coverage
        string(APPEND Coverage_COVERAGE_ARGS_MULTILINE)
        foreach(Coverage_MODULE ${Coverage_MODULES})
            file(TO_NATIVE_PATH ${Coverage_MODULE} Coverage_MODULE)
            string(APPEND Coverage_COVERAGE_ARGS_MULTILINE "modules=${Coverage_MODULE}\n")
        endforeach()
        foreach(Coverage_EXCLUDED_MODULE ${Coverage_EXCLUDED_MODULES})
            file(TO_NATIVE_PATH ${Coverage_EXCLUDED_MODULE} Coverage_EXCLUDED_MODULE)
            string(APPEND Coverage_COVERAGE_ARGS_MULTILINE "excluded_modules=${Coverage_EXCLUDED_MODULE}\n")
        endforeach()
        foreach(Coverage_SOURCE_FILE ${Coverage_SOURCES})
            file(TO_NATIVE_PATH ${Coverage_SOURCE_FILE} SOURCE_NATIVE_PATH)
            string(APPEND Coverage_COVERAGE_ARGS_MULTILINE "sources=${SOURCE_NATIVE_PATH}\n")
        endforeach()
        foreach(Coverage_EXCLUDED_SOURCE ${Coverage_EXCLUDED_SOURCES})
            file(TO_NATIVE_PATH ${Coverage_EXCLUDED_SOURCE} Coverage_EXCLUDED_SOURCE)
            string(APPEND Coverage_COVERAGE_ARGS_MULTILINE "excluded_sources=${Coverage_EXCLUDED_SOURCE}\n")
        endforeach()

        foreach(COVERAGE_ARG IN LISTS Coverage_COVERAGE_ARGS)
            string(APPEND Coverage_COVERAGE_ARGS_MULTILINE "${COVERAGE_ARG}\n")
        endforeach()

        # Generate configuration file for test target used to output .cov
        file(WRITE ${Coverage_CONFIG_INPUT_FILE}
            "# Auto-generated config file for OpenCppCoverage to produce coverage output\n"
            "export_type=binary:${Coverage_NATIVE_BINARY_OUTPUT_FILE}\n"
            "export_type=html:${TARGET_BINARY_DIR}/${Coverage_TARGET}Coverage\n"
            "${Coverage_COVERAGE_ARGS_MULTILINE}"
        )

        # Add custom command to generate coverage file
        add_custom_command(
            OUTPUT ${Coverage_BINARY_OUTPUT_FILE}
            COMMENT "Running OpenCppCoverage on test target ${Coverage_TARGET} to collect test coverage..."
            COMMAND OpenCppCoverage
                ${Coverage_CLI_ARGS}
                --config_file=${Coverage_CONFIG_INPUT_FILE}
                -- $<TARGET_FILE:${Coverage_TARGET}> ${Coverage_TARGET_ARGS}
            DEPENDS
                ${Coverage_TARGET}
                ${Coverage_DEPENDENCIES}
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            VERBATIM # Protect arguments to commands
        )

        # Add target that generates coverage data
        add_custom_target(${Coverage_TARGET}Coverage
            DEPENDS ${Coverage_BINARY_OUTPUT_FILE}
        )

        set_target_properties(${Coverage_TARGET}Coverage PROPERTIES
            FOLDER "${META_PROJECT_NAME}/Coverage"
        )

        # Test-target report handling
        set(TEST_COVERAGE_REPORT_TARGET ${Coverage_TARGET}CoverageReport)
        add_coverage_report(
            "${Coverage_TARGET}"
            "${TARGET_BINARY_DIR}"
            "${Coverage_BINARY_OUTPUT_FILE}"
            "${TEST_COVERAGE_REPORT_TARGET}"
            "${Coverage_CLI_ARGS}"
        )
        set_property(TARGET ${TEST_COVERAGE_REPORT_TARGET}
            APPEND_STRING PROPERTY
            CPP_COVERAGE_INPUT_FILES "input_coverage=${Coverage_NATIVE_BINARY_OUTPUT_FILE}\n"
        )
        set_target_properties(${TEST_COVERAGE_REPORT_TARGET} PROPERTIES
            FOLDER "${META_PROJECT_NAME}/Coverage"
        )

        # Project report handling
        set(PROJECT_COVERAGE_REPORT_TARGET ${META_PROJECT_NAME}CoverageReport)
        if (NOT TARGET ${PROJECT_COVERAGE_REPORT_TARGET})
            add_coverage_report(
                "${META_PROJECT_NAME}" 
                "${PROJECT_BINARY_DIR}" 
                "${Coverage_BINARY_OUTPUT_FILE}" 
                "${PROJECT_COVERAGE_REPORT_TARGET}"
                "${Coverage_CLI_ARGS}"
            )
        endif()
        set_property(TARGET ${PROJECT_COVERAGE_REPORT_TARGET}
            APPEND_STRING PROPERTY 
            CPP_COVERAGE_INPUT_FILES "input_coverage=${Coverage_NATIVE_BINARY_OUTPUT_FILE}\n"
        )
        set_target_properties(${PROJECT_COVERAGE_REPORT_TARGET} PROPERTIES
            FOLDER "${META_PROJECT_NAME}/Coverage"
        )

        # Make project report target depend on target generating coverage
        add_dependencies(${PROJECT_COVERAGE_REPORT_TARGET} ${Coverage_TARGET}Coverage)
    endfunction()

    function(add_coverage_report
        TARGET_NAME
        TARGET_BINARY_DIR
        CPP_COVERAGE_BINARY_OUTPUT_FILE
        REPORT_TARGET
        OPENCPPCOVERAGE_CLI_ARGS)

        set(REPORT_CONFIG_FILE ${TARGET_BINARY_DIR}/${TARGET_NAME}.cov.report.txt)
        set(REPORT_COBERTURA_FILE ${TARGET_BINARY_DIR}/${TARGET_NAME}.cobertura.xml)
        set(REPORT_DIR ${TARGET_BINARY_DIR}/${TARGET_NAME}Coverage)
        set(REPORT ${REPORT_DIR}/index.html)

        add_custom_target(${REPORT_TARGET}
            DEPENDS ${REPORT}
            VERBATIM
        )

        string(APPEND REPORT_ARGS_MULTILINE "# Auto-generated config file for OpenCppCoverage to produce coverage report\n")
        string(APPEND REPORT_ARGS_MULTILINE "export_type=html:${REPORT_DIR}\n")
        string(APPEND REPORT_ARGS_MULTILINE "export_type=cobertura:${REPORT_COBERTURA_FILE}\n")

        add_custom_command(OUTPUT ${REPORT}
            COMMAND OpenCppCoverage
                ${OPENCPPCOVERAGE_CLI_ARGS}
                --config_file ${REPORT_CONFIG_FILE}
            COMMAND ${CPP_COVERAGE_REPORT_GENERATOR_TOOL} -reports:${REPORT_COBERTURA_FILE} -reporttypes:Html;HtmlChart;Badges -targetdir:${TARGET_BINARY_DIR}/CustomReport -historydir:${TARGET_BINARY_DIR}/history
            DEPENDS
                ${REPORT_CONFIG_FILE}
                ${CPP_COVERAGE_BINARY_OUTPUT_FILE}
            WORKING_DIRECTORY ${REPORT_DIR}
            COMMENT "Running OpenCppCoverage to generate code coverage report for \"${REPORT_TARGET}\" in \"${REPORT}\""
            VERBATIM
        )

        file(GENERATE
            OUTPUT ${REPORT_CONFIG_FILE}
            CONTENT "${REPORT_ARGS_MULTILINE}$<TARGET_PROPERTY:${REPORT_TARGET},CPP_COVERAGE_INPUT_FILES>"
        )
    endfunction()
endif()

function(set_project_coverage_flags project_name)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${project_name}
            INTERFACE --coverage)

        target_link_options(${project_name}
            INTERFACE --coverage)

        target_link_libraries(${project_name}
            INTERFACE gcov)
    endif()
endfunction()
