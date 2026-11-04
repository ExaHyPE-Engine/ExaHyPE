# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

macro(peano_find_host_dependencies)
  if(TARGET PeanoOptions)
    find_package(MPI REQUIRED)
    # MPI::MPI_C adds its settings to C sources only.
    target_link_libraries(PeanoOptions INTERFACE
      MPI::MPI_CXX
      $<TARGET_NAME_IF_EXISTS:MPI::MPI_C>
    )
    # Peano calls the MPI C API only. The <mpi.h> of some OpenMPI and MPICH installations includes
    # the deprecated C++ bindings. These headers raise -Wcast-function-type warnings.
    target_compile_definitions(PeanoOptions INTERFACE OMPI_SKIP_MPICXX MPICH_SKIP_MPICXX)
  else()
    find_dependency(MPI REQUIRED)
  endif()

  if(TARGET PeanoOptions)
    find_package(OpenMP REQUIRED)
    # OpenMP::OpenMP_C adds its compile flag to C sources only.
    target_link_libraries(PeanoOptions INTERFACE
      OpenMP::OpenMP_CXX
      $<TARGET_NAME_IF_EXISTS:OpenMP::OpenMP_C>
    )
    # OpenMP::OpenMP_CXX adds its compile flag to C++ sources only. A CUDA or HIP source gets the
    # host OpenMP flag here. The host loops of such a source then run on every thread of the team.
    separate_arguments(_peano_openmp_flags NATIVE_COMMAND "${OpenMP_CXX_FLAGS}")
    foreach(_peano_openmp_flag ${_peano_openmp_flags})
      target_compile_options(PeanoOptions INTERFACE
        "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=${_peano_openmp_flag}>"
        "$<$<COMPILE_LANGUAGE:HIP>:${_peano_openmp_flag}>"
      )
    endforeach()
  else()
    find_dependency(OpenMP REQUIRED)
  endif()

  if(PEANO_WITH_HDF5)
    if(TARGET PeanoOptions)
      set(HDF5_PREFER_PARALLEL True)
      find_package(HDF5 COMPONENTS C)
      if(HDF5_FOUND)
        if(NOT HDF5_IS_PARALLEL)
          message(FATAL_ERROR "Peano requires an HDF5 installation with parallel I/O.")
        endif()
        target_link_libraries(PeanoOptions INTERFACE HDF5::HDF5)
        target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_HDF5)
      else()
        message(WARNING "CMake finds no HDF5 installation for PEANO_WITH_HDF5=ON. Peano builds without HDF5 support.")
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
          message(FATAL_ERROR "Peano requires a NetCDF installation with HDF5 support for NetCDF-4 files.")
        endif()
        if(NOT NetCDF_IS_PARALLEL)
          message(FATAL_ERROR "Peano requires a NetCDF installation with parallel I/O.")
        endif()
        target_link_libraries(PeanoOptions INTERFACE ${NetCDF_LIBRARIES})
        target_include_directories(PeanoOptions INTERFACE ${NetCDF_INCLUDE_DIRS})
        target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_NETCDF)
      else()
        message(WARNING "CMake finds no NetCDF installation for PEANO_WITH_NETCDF=ON. Peano builds without NetCDF support.")
        set(PEANO_WITH_NETCDF OFF CACHE BOOL "Build with NetCDF support" FORCE)
      endif()
    endif()
    # The exported target stores the absolute NetCDF library path and include directory. A consumer
    # of the export needs no NetCDF target.
  endif()

  if(PEANO_WITH_ZLIB)
    if(TARGET PeanoOptions)
      find_package(ZLIB)
      if(ZLIB_FOUND)
        target_link_libraries(PeanoOptions INTERFACE ZLIB::ZLIB)
        target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_ZLIB)
      else()
        message(WARNING "CMake finds no ZLIB installation for PEANO_WITH_ZLIB=ON. Peano builds without ZLIB compression.")
        set(PEANO_WITH_ZLIB OFF CACHE BOOL "Build with ZLIB compression support" FORCE)
      endif()
    else()
      find_dependency(ZLIB)
    endif()
  endif()
endmacro()
