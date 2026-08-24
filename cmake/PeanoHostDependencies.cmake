# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

macro(peano_find_host_dependencies)
  if(TARGET PeanoOptions)
    find_package(MPI REQUIRED)
    # Link the MPI imported target for every enabled core language (C and C++). The C
    # variant is inert until a .c source is added, but keeps the two languages consistent.
    target_link_libraries(PeanoOptions INTERFACE
      MPI::MPI_CXX
      $<TARGET_NAME_IF_EXISTS:MPI::MPI_C>
    )
    # Skip MPI's deprecated C++ bindings (mpicxx.h). We only use the C API,
    # and on some OpenMPI/MPICH installs <mpi.h> auto-includes them, which
    # triggers -Wcast-function-type warnings from inside the MPI headers.
    target_compile_definitions(PeanoOptions INTERFACE OMPI_SKIP_MPICXX MPICH_SKIP_MPICXX)
  else()
    find_dependency(MPI REQUIRED)
  endif()

  if(PEANO_WITH_MULTITHREADING)
    if(TARGET PeanoOptions)
      find_package(OpenMP)
      if(OpenMP_FOUND)
        # Link the OpenMP imported target for every enabled core language (C and C++). The C
        # variant is inert until a .c source is added, but keeps the two languages consistent.
        target_link_libraries(PeanoOptions INTERFACE
          OpenMP::OpenMP_CXX
          $<TARGET_NAME_IF_EXISTS:OpenMP::OpenMP_C>
        )
        target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_OPENMP PEANO_WITH_MULTITHREADING)
      else()
        message(WARNING "PEANO_WITH_MULTITHREADING=ON but OpenMP was not found; continuing without multithreading support.")
        set(PEANO_WITH_MULTITHREADING OFF CACHE BOOL "Build with multithreading (OpenMP) support" FORCE)
      endif()
    else()
      find_dependency(OpenMP)
    endif()
  endif()

  if(PEANO_WITH_HDF5)
    if(TARGET PeanoOptions)
      set(HDF5_PREFER_PARALLEL True)
      find_package(HDF5 COMPONENTS C)
      if(HDF5_FOUND)
        if(NOT HDF5_IS_PARALLEL)
          message(FATAL_ERROR "The found HDF5 installation does not support parallel I/O, which Peano requires.")
        endif()
        target_link_libraries(PeanoOptions INTERFACE HDF5::HDF5)
        target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_HDF5)
      else()
        message(WARNING "PEANO_WITH_HDF5=ON but no HDF5 installation was found; continuing without HDF5 support.")
        set(PEANO_WITH_HDF5 OFF CACHE BOOL "Build with HDF5 support" FORCE)
      endif()
    else()
      find_dependency(HDF5 COMPONENTS C)
    endif()
  endif()

  if(PEANO_WITH_NETCDF)
    if(TARGET PeanoOptions)
      find_package(NetCDF)
      if(NetCDF_FOUND)
        if(NOT NetCDF_HAS_HDF5)
          message(FATAL_ERROR "Found NetCDF installation was built without HDF5 support (required for NetCDF-4 support).")
        endif()
        if(NOT NetCDF_IS_PARALLEL)
          message(FATAL_ERROR "The found NetCDF installation does not support parallel I/O, which Peano requires.")
        endif()
        target_link_libraries(PeanoOptions INTERFACE ${NetCDF_LIBRARIES})
        target_include_directories(PeanoOptions INTERFACE ${NetCDF_INCLUDE_DIRS})
        target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_NETCDF)
      else()
        message(WARNING "PEANO_WITH_NETCDF=ON but no NetCDF installation was found; continuing without NetCDF support.")
        set(PEANO_WITH_NETCDF OFF CACHE BOOL "Build with NetCDF support" FORCE)
      endif()
    endif()
    # Application: NetCDF is baked into the exported target by absolute library
    # path and include directory (no imported target), so nothing is re-imported.
  endif()

  if(PEANO_WITH_ZLIB)
    if(TARGET PeanoOptions)
      find_package(ZLIB)
      if(ZLIB_FOUND)
        target_link_libraries(PeanoOptions INTERFACE ZLIB::ZLIB)
        target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_ZLIB)
      else()
        message(WARNING "PEANO_WITH_ZLIB=ON but ZLIB was not found; continuing without ZLIB compression support.")
        set(PEANO_WITH_ZLIB OFF CACHE BOOL "Build with ZLIB compression support" FORCE)
      endif()
    else()
      find_dependency(ZLIB)
    endif()
  endif()
endmacro()
