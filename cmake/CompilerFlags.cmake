function(set_project_compiler_flags project_name)
  set(MSVC_COMPILER_FLAGS
    $<$<COMPILE_LANGUAGE:CXX>:/MP> # Use as many cores as available for compilation. https://blog.kitware.com/cmake-building-with-all-your-cores/
    $<$<COMPILE_LANGUAGE:CXX>:/W4> #/Wall # Baseline reasonable warnings.
    $<$<COMPILE_LANGUAGE:CXX>:/w14242> # 'identifier': conversion from 'type1' to 'type1', possible loss of data.
    $<$<COMPILE_LANGUAGE:CXX>:/w14254> # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data.
    $<$<COMPILE_LANGUAGE:CXX>:/w14263> # 'function': member function does not override any base class virtual member function.
    $<$<COMPILE_LANGUAGE:CXX>:/w14265> # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not be destructed correctly.
    $<$<COMPILE_LANGUAGE:CXX>:/w14287> # 'operator': unsigned/negative constant mismatch.
    $<$<COMPILE_LANGUAGE:CXX>:/we4289> # Nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope.
    $<$<COMPILE_LANGUAGE:CXX>:/w14296> # 'operator': expression is always 'boolean_value'.
    $<$<COMPILE_LANGUAGE:CXX>:/w14311> # 'variable': pointer truncation from 'type1' to 'type2'.
    $<$<COMPILE_LANGUAGE:CXX>:/w14545> # Expression before comma evaluates to a function which is missing an argument list.
    $<$<COMPILE_LANGUAGE:CXX>:/w14546> # Function call before comma missing argument list.
    $<$<COMPILE_LANGUAGE:CXX>:/w14547> # 'operator': operator before comma has no effect; expected operator with side-effect.
    $<$<COMPILE_LANGUAGE:CXX>:/w14549> # 'operator': operator before comma has no effect; did you intend 'operator'?
    $<$<COMPILE_LANGUAGE:CXX>:/w14555> # Expression has no effect; expected expression with side- effect.
    $<$<COMPILE_LANGUAGE:CXX>:/w14619> # Pragma warning: there is no warning number 'number'.
    $<$<COMPILE_LANGUAGE:CXX>:/w14640> # Enable warning on thread un-safe static member initialization.
    $<$<COMPILE_LANGUAGE:CXX>:/w14826> # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
    $<$<COMPILE_LANGUAGE:CXX>:/w14905> # Wide string literal cast to 'LPSTR'.
    $<$<COMPILE_LANGUAGE:CXX>:/w14906> # String literal cast to 'LPWSTR'.
    $<$<COMPILE_LANGUAGE:CXX>:/w14928> # Illegal copy-initialization; more than one user-defined conversion has been implicitly applied.
    $<$<COMPILE_LANGUAGE:CXX>:/permissive-> # Standards conformance mode for MSVC compiler. https://docs.microsoft.com/de-de/cpp/build/reference/permissive-standards-conformance?view=vs-2019
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:Release>:/Oi>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:Release>:/O2>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:Release>:/Ot>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:Release>:/GL>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:Release>:/GF>>
  )

  set(LLVM_COMPILER_FLAGS
    -W
    -Wall
    -Wextra # Reasonable and standard.
    -Wshadow # Warn the user if a variable declaration shadows one from a parent context.
    -Wnon-virtual-dtor # Warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors.
    -Wold-style-cast # Warn for c-style casts.
    -Wcast-align # Warn for potential performance problem casts.
    -Wunused # Warn on anything being unused.
    -Wunused-function
    -Wunused-variable
    -Wunused-parameter
    -Woverloaded-virtual # Warn if you overload (not override) a virtual function.
    -pedantic
    -Wpedantic # Warn if non-standard C++ is used.
    -Wconversion # Warn on type conversions that may lose data.
    -Wparentheses
    -Wmultichar
    -Wtrigraphs
    -Wpointer-arith
    -Wreturn-type
    -Wno-system-headers
    -Wno-deprecated
    -Wwrite-strings
    -fexceptions
    -fnon-call-exceptions
    -Wsign-conversion # Warn on sign conversions.
    -Wnull-dereference # Warn if a null dereference is detected.
    -Wdouble-promotion # Warn if float is implicit promoted to double.
    -Wformat=2 # Warn on security issues around functions that format output (i.e., printf).
    #-flto # https://stackoverflow.com/questions/31688069/requirements-to-use-flto
    #-ffat-lto-objects # https://stackoverflow.com/questions/13799452/what-is-the-difference-in-gcc-between-lto-and-fat-lto-objects
    #-fno-elide-constructors # Disables RVO. For testing purposes only, should be disabled in production code. https://shaharmike.com/cpp/rvo/
    $<$<CONFIG:Debug>:-O0;-ggdb;-fno-elide-constructors>
  )

  set(GNU_COMPILER_FLAGS
    ${LLVM_COMPILER_FLAGS}
    -Wmisleading-indentation # Warn if indentation implies blocks where blocks do not exist.
    -Wduplicated-cond # Warn if if/else chain has duplicated conditions.
    -Wduplicated-branches # Warn if if/else branches have duplicated code.
    -Wlogical-op # Warn about logical operations being used where bitwise were probably wanted.
    -Wuseless-cast # Warn if you perform a cast to the same type.
    #-m64
    #-ipo
    #-fp-model precise
  )

  set(NVHPC_COMPILER_FLAGS)

  if(MSVC)
    set(COMPILER_FLAGS ${MSVC_COMPILER_FLAGS})
  elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(COMPILER_FLAGS ${LLVM_COMPILER_FLAGS})
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(COMPILER_FLAGS ${GNU_COMPILER_FLAGS})
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
    set(COMPILER_FLAGS ${LLVM_COMPILER_FLAGS})
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "NVHPC")
    set(COMPILER_FLAGS ${NVHPC_COMPILER_FLAGS})
  else()
    message(AUTHOR_WARNING "No compiler flags set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
  endif()

  target_compile_options(${project_name} INTERFACE ${COMPILER_FLAGS})
endfunction()
