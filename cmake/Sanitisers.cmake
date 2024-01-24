option(SANITISE_ADDRESS "Sanitise address" OFF)
option(SANITISE_UNDEFINED "Sanitise undefined behaviour" OFF)
option(SANITISE_THREAD "Sanitise thread" OFF)

if(SANITISE_ADDRESS)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

if(SANITISE_UNDEFINED)
  add_compile_options(-fsanitize=undefined)
  add_link_options(-fsanitize=undefined)
endif()

if(SANITISE_THREAD)
  add_compile_options(-fsanitize=thread)
  add_link_options(-fsanitize=thread)
endif()
