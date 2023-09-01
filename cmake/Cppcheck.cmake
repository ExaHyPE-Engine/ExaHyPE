find_program(CPPCHECK cppcheck)
if(CPPCHECK)
  execute_process(COMMAND ${CPPCHECK} --version
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE CPPCHECKRESULT
    OUTPUT_VARIABLE CPPCHECKVERSION
    ERROR_VARIABLE CPPCHECKERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if(CPPCHECKRESULT EQUAL 0)
    include(ProcessorCount)
    ProcessorCount(CPU_CORES)

    set(CPPCHECK_ARGS
      #--suppress=missingInclude
      --enable=all
      #--enable=warning,performance,portability,information,missingInclude,unusedFunction # We want do disable style since we are using clang-format.
      --inconclusive
      --template="[{severity}][{id}] {message} {callstack} \(On {file}:{line}\)"
      #-j ${CPU_CORES} # Use all the available CPU cores
      --force
      --quiet # Only show found errors
      #--verbose
      --suppress=*:${CMAKE_SOURCE_DIR}/submodules*
      --std=c++20 # Optional: Specified C++ version
      #--error-exitcode=1
    )

    set(CMAKE_CXX_CPPCHECK ${CPPCHECK} ${CPPCHECK_ARGS})
    message(STATUS "Found ${CPPCHECKVERSION}: " ${CPPCHECK})

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      set(CPPCHECK_PROJECT compile_commands.json)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
      set(CPPCHECK_PROJECT ${META_PROJECT_NAME}.sln)
    endif()

        add_custom_target(Cppcheck
            COMMAND ${CPPCHECK}
                --project=${CPPCHECK_PROJECT}
                ${CPPCHECK_ARGS}
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            VERBATIM # Protect arguments to commands
            COMMENT "Running Cppcheck."
        )
        set_target_properties(Cppcheck PROPERTIES
            FOLDER "${META_PROJECT_NAME}/Tools"
        )

        find_program(CPPCHECK_HMTL cppcheck-htmlreport)
        if(CPPCHECK_HMTL)
            message(STATUS "Found Cppcheck-htmlreport: " ${CPPCHECK_HMTL})

            add_custom_target(CppcheckReport
                COMMAND ${CPPCHECK}
                    --project=${CPPCHECK_PROJECT}
                    ${CPPCHECK_ARGS}
                    --xml
                    --xml-version=2
                    --output-file=CppcheckReport.xml

                COMMAND ${CPPCHECK_HMTL}
                    --source-dir=.
                    --file=CppcheckReport.xml
                    --title=CppcheckReport
                    --report-dir=CppcheckReport

                BYPRODUCTS
                    ${PROJECT_BINARY_DIR}/CppcheckReport.xml
                    ${PROJECT_BINARY_DIR}/CppcheckReport

                WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
                VERBATIM # Protect arguments to commands
                COMMENT "Running Cppcheck and Cppcheck-htmlreport to produce HTML report."
            )
            set_target_properties(CppcheckReport PROPERTIES
                FOLDER "${META_PROJECT_NAME}/Tools"
            )
        endif()
    else()
        message(SEND_ERROR "Cppcheck found but cannot retrieve version information")
    endif()
else()
    message(STATUS "Cppcheck requested but executable not found")
endif()
