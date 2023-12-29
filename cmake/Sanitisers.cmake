option(WITH_SANITISE_ADDRESS "Build with address sanitiser" OFF)
option(WITH_SANITISE_UNDEFINED "Build with undefined behaviour sanitiser" OFF)
option(WITH_SANITISE_THREAD "Build with thread sanitiser" OFF)

if(WITH_SANITISE_ADDRESS)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

if(WITH_SANITISE_UNDEFINED)
  add_compile_options(-fsanitize=undefined)
  add_link_options(-fsanitize=undefined)
endif()

if(WITH_SANITISE_THREAD)
  add_compile_options(-fsanitize=thread)
  add_link_options(-fsanitize=thread)
endif()
