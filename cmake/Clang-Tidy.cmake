find_program(CLANGTIDY clang-tidy NAMES clang-tidy clang-tidy-6.0 clang-tidy-7 clang-tidy-8 clang-tidy-9 clang-tidy-10)
if(CLANGTIDY)
    option(BUILD_WITH_CLANG_TIDY_FIX "Build with Static Analysis Tool Clang-Tidy and Perform Fixes" OFF)

    execute_process(COMMAND ${CLANGTIDY} --version
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        RESULT_VARIABLE CLANGTIDYRESULT
        OUTPUT_VARIABLE CLANGTIDYVERSION
        ERROR_VARIABLE CLANGTIDYERROR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(CLANGTIDYRESULT EQUAL 0)
        set(CLANGTIDY_ARGS
            -extra-arg=-Wno-unknown-warning-option
            -header-filter=.*
            #-format-style=file
            --
            -std=c++17
            -I${CMAKE_SOURCE_DIR}/src
        )

        if(BUILD_WITH_CLANG_TIDY_FIX)
            find_program(CLANGREPLACEMENTS clang-apply-replacements)
            if(CLANGREPLACEMENTS)
                list(APPEND CLANGTIDY_ARGS
                    -fix # Should only be used with single checks only (e.g., 'clang-tidy -checks=-*,modernize-use-auto -fix'). In most cases this flag is not recommended.
                )
            else()
                message(SEND_ERROR "clang-apply-replacements requested but executable not found")
            endif()
        endif()

        set(CMAKE_CXX_CLANG_TIDY ${CLANGTIDY} ${CLANGTIDY_ARGS})
        message(STATUS "Found ${CLANGTIDYVERSION}: " ${CLANGTIDY})

        file(DOWNLOAD "https://raw.githubusercontent.com/llvm/llvm-project/master/clang-tools-extra/clang-tidy/tool/run-clang-tidy.py" ${PROJECT_BINARY_DIR}/run-clang-tidy.py
            STATUS DOWNLOAD_STATUS)
        list(GET DOWNLOAD_STATUS 0 ERROR_CODE)
        if(ERROR_CODE)
            message(FATAL_ERROR "Error downloading run-clang-tidy.py")
        endif()

        file(DOWNLOAD "https://raw.githubusercontent.com/austinbhale/Clang-Visualizer/master/checks.py" ${PROJECT_BINARY_DIR}/checks.py
            STATUS DOWNLOAD_STATUS)
        list(GET DOWNLOAD_STATUS 0 ERROR_CODE)
        if(ERROR_CODE)
            message(FATAL_ERROR "Error downloading checks.py")
        endif()

        file(DOWNLOAD "https://raw.githubusercontent.com/austinbhale/Clang-Visualizer/master/clang_visualizer.py" ${PROJECT_BINARY_DIR}/clang_visualizer.py
            STATUS DOWNLOAD_STATUS)
        list(GET DOWNLOAD_STATUS 0 ERROR_CODE)
        if(ERROR_CODE)
            message(FATAL_ERROR "Error downloading clang_visualizer.py")
        endif()

        add_custom_target(Tidy
            COMMAND python3 run-clang-tidy.py ${CLANGTIDY_ARGS} | tee clang-tidy.log
            COMMAND python3 clang_visualizer.py clang-tidy.log
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            VERBATIM # Protect arguments to commands
            COMMENT "Running clang-tidy."
        )
        set_target_properties(Tidy PROPERTIES
            FOLDER "${META_PROJECT_NAME}/Tools"
        )

        add_custom_target(Tidy-List
            COMMAND ${CLANGTIDY} -list-checks
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            VERBATIM # Protect arguments to commands
            COMMENT "List all enabled clang-tidy checks."
        )
        set_target_properties(Tidy-List PROPERTIES
            FOLDER "${META_PROJECT_NAME}/Tools"
        )

        add_custom_target(Tidy-Dump
            COMMAND ${CLANGTIDY} -dump-config
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            VERBATIM # Protect arguments to commands
            COMMENT "Dumps clang-tidy configuration file."
        )
        set_target_properties(Tidy-Dump PROPERTIES
            FOLDER "${META_PROJECT_NAME}/Tools"
        )

        function(set_clang_tidy_target target)
            set_property(TARGET ${target} PROPERTY CXX_CLANG_TIDY ${CLANGTIDY} ${CLANGTIDY_ARGS})
        endfunction()
    else()
        message(SEND_ERROR "clang-tidy found but cannot retrieve version information")
    endif()
else()
    message(STATUS "clang-tidy requested but executable not found")
endif()
