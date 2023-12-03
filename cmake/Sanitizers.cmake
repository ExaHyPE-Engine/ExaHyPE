option(WITH_SANITIZE_ADDRESS "Enables Address Sanitizer in GCC and Clang" OFF)
option(WITH_SANITIZE_UNDEFINED "Enables Undefined Behavior Sanitizer in GCC and Clang" OFF)
option(WITH_SANITIZE_THREAD "Enable Thread Sanitizer in GCC and Clang" OFF)

if (WITH_SANITIZE_ADDRESS)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

if (WITH_SANITIZE_UNDEFINED)
  add_compile_options(-fsanitize=undefined)
  add_link_options(-fsanitize=undefined)
endif()

if (WITH_SANITIZE_THREAD)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()
