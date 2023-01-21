message(STATUS "Configuring swift2")

file(GLOB_RECURSE SWIFT2_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(mode IN LISTS BUILD_MODES)
	string(TOUPPER ${mode} mode_upper)
	foreach(dimension IN LISTS PEANO_DIMENSIONS)
		message(
			STATUS
				"Adding a Swift2 target with dimension=${dimension} and build_mode=${mode}"
		)
		
		add_library(SWIFT2Core${dimension}d${SUFFIX_${mode}} STATIC)
		target_sources(SWIFT2Core${dimension}d${SUFFIX_${mode}} PUBLIC ${SWIFT2_SOURCES})
		target_include_directories(SWIFT2Core${dimension}d${SUFFIX_${mode}} PUBLIC ${CMAKE_SOURCE_DIR}/src)
		target_link_libraries(SWIFT2Core${dimension}d${SUFFIX_${mode}} PRIVATE Tarch${SUFFIX_${mode}})
		target_compile_definitions(SWIFT2Core${dimension}d${SUFFIX_${mode}} PRIVATE Dimensions=${dimension})
		target_compile_options(SWIFT2Core${dimension}d${SUFFIX_${mode}} PRIVATE ${${mode_upper}_FLAGS})
		target_compile_definitions(SWIFT2Core${dimension}d${SUFFIX_${mode}} PRIVATE ${${mode_upper}_DEFINITIONS})
		target_link_options(SWIFT2Core${dimension}d${SUFFIX_${mode}} PRIVATE ${${mode_upper}_LDFLAGS})

		install(
			TARGETS SWIFT2Core${dimension}d${SUFFIX_${mode}}
			RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
							COMPONENT ${PROJECT_NAME}_Runtime
			LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
							COMPONENT ${PROJECT_NAME}_Runtime
							NAMELINK_COMPONENT ${PROJECT_NAME}_Devel
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
							COMPONENT ${PROJECT_NAME}_Devel)

	endforeach()
endforeach()
