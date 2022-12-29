message(STATUS "Configuring convert")

file(GLOB_RECURSE CONVERT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(dimension IN LISTS PEANO_DIMENSIONS)
	foreach(mode IN LISTS BUILD_MODES)
		add_executable(convert_${dimension}d${SUFFIX_${mode}})
		target_sources(convert_${dimension}d${SUFFIX_${mode}} PRIVATE ${CONVERT_SOURCES})
		target_include_directories(convert_${dimension}d${SUFFIX_${mode}} PRIVATE ${CMAKE_SOURCE_DIR}/src)
		target_link_libraries(convert_${dimension}d${SUFFIX_${mode}} PRIVATE Tarch${SUFFIX_${mode}})
		# stdc++ before stdc++fs is needed for intel oneapi compiler
		target_link_libraries(convert_${dimension}d${SUFFIX_${mode}} PRIVATE stdc++ stdc++fs)
		target_link_options(convert_${dimension}d${SUFFIX_${mode}} PRIVATE -lstdc++ -lstdc++fs ${${mode}_LDFLAGS})
		target_compile_options(convert_${dimension}d${SUFFIX_${mode}} PRIVATE ${${mode}_FLAGS})
		target_compile_definitions(convert_${dimension}d${SUFFIX_${mode}} PRIVATE ${${mode}_DEFINITIONS})
	 endforeach()
endforeach()