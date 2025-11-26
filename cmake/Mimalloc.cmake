# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

option(USE_MIMALLOC "Use mimalloc" OFF)

if(USE_MIMALLOC)
  set(_MI_HINTS)
  if(DEFINED MIMALLOC_DIR)
    list(APPEND _MI_HINTS HINTS "${MIMALLOC_DIR}")
  endif()
  if(DEFINED ENV{MIMALLOC_DIR})
    list(APPEND _MI_HINTS HINTS "$ENV{MIMALLOC_DIR}")
  endif()
  if(DEFINED ENV{MIMALLOC_ROOT})
    list(APPEND _MI_HINTS HINTS "$ENV{MIMALLOC_ROOT}")
  endif()

  find_package(mimalloc CONFIG QUIET ${_MI_HINTS})

  if(mimalloc_FOUND)
    # Prefer static if exported, otherwise fall back to shared
    if(TARGET mimalloc-static)
      set(_mi_tgt mimalloc-static)
    elseif(TARGET mimalloc)
      set(_mi_tgt mimalloc)
    else()
      set(_mi_tgt "")
    endif()

    if(_mi_tgt)
      target_link_libraries(PeanoOptions INTERFACE ${_mi_tgt})
      target_compile_definitions(PeanoOptions INTERFACE USE_MIMALLOC)
      message(STATUS "Using ${_mi_tgt}")
    else()
      message(WARNING "mimalloc found but exports no 'mimalloc' or 'mimalloc-static' target; skipping.")
    endif()
  else()
    message(STATUS "mimalloc not found; building without it (set MIMALLOC_DIR to hint).")
  endif()
endif()
