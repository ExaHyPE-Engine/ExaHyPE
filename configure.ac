#
# Recreate build environment:
# ---------------------------
#   libtoolize
#   aclocal
#   autoconf
#   autoheader
#   automake --add-missing
#
#
# Summary:
#   libtoolize; aclocal; autoconf; autoheader; cp src/config.h.in .; automake --add-missing
#
#
# Cleanup
# -------
#   rm -rf aclocal.m4 compile src/config.* configure *sh Makefile Makefile.in missing stamp* ar-lib autom4te.cache *.log libtool depcomp *.scan  
#   find . -name Makefile -delete
#   find . -name Makefile.in -delete
#
#
AC_INIT([Peano],[4.0])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE(foreign -Wall -Werror)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_HEADERS([src/config.h])

AC_CANONICAL_BUILD

AC_PROG_INSTALL
AC_PROG_MAKE_SET

#
# Default installation is current path
#
AC_PREFIX_DEFAULT( "libs" )


#
# This is just a sanity check whether the right files/directories do exist
#
AC_CONFIG_SRCDIR([src/peano4/peano.cpp])

#
# Pipe system-specific findings into a file config.h
#
AC_CONFIG_HEADERS([config.h])
   
#
# I wanna build libraries
#
AM_PROG_AR
LT_INIT
AC_CONFIG_MACRO_DIRS([m4])


# 
# Checks for programs.
#
AC_PROG_CXX
AC_PROG_FC



#
# OpenMP Options
# We need those separately from the Peano build as we use OpenMP for 
# postprocessing scripts, e.g.
#
AC_LANG(C++)


#
# Compiler options. Allow icpc and gnu. Plus minimum version check.
#

#AX_COMPILER_VENDOR

#AX_CXX_COMPILE_STDCXX_11



srcdir_full_path=`cd $srcdir && pwd`


#
# ===============================================================
#   Define all the compiler and linker variables of Peano
# ===============================================================
#

CXXFLAGS_PEANO_2D_RELEASE=" -DDimensions=2 -DPeanoDebug=0"
CXXFLAGS_PEANO_2D_STATS="   -DDimensions=2 -DPeanoDebug=0 -DTrackStatistics"
CXXFLAGS_PEANO_2D_ASSERTS=" -DDimensions=2 -DPeanoDebug=2 -g -O0"
CXXFLAGS_PEANO_2D_TRACE="   -DDimensions=2 -DPeanoDebug=1 -g"
CXXFLAGS_PEANO_2D_DEBUG="   -DDimensions=2 -DPeanoDebug=4 -g -O0"

CXXFLAGS_PEANO_3D_RELEASE=" -DDimensions=3 -DPeanoDebug=0"
CXXFLAGS_PEANO_3D_STATS="   -DDimensions=3 -DPeanoDebug=0 -DTrackStatistics"
CXXFLAGS_PEANO_3D_ASSERTS=" -DDimensions=3 -DPeanoDebug=2 -g -O0"
CXXFLAGS_PEANO_3D_TRACE="   -DDimensions=3 -DPeanoDebug=1 -g"
CXXFLAGS_PEANO_3D_DEBUG="   -DDimensions=3 -DPeanoDebug=4 -g -O0"


LDFLAGS_PEANO_RELEASE=" -L$srcdir_full_path/src "
LDFLAGS_PEANO_STATS="   -L$srcdir_full_path/src "
LDFLAGS_PEANO_ASSERTS=" -L$srcdir_full_path/src -g"
LDFLAGS_PEANO_TRACE="   -L$srcdir_full_path/src -g"
LDFLAGS_PEANO_DEBUG="   -L$srcdir_full_path/src -g"


LDADD_PEANO_2D_RELEASE=" -lPeano4Core2d         -lTarch"
LDADD_PEANO_2D_STATS="   -lPeano4Core2d_stats   -lTarch_stats"
LDADD_PEANO_2D_ASSERTS=" -lPeano4Core2d_asserts -lTarch_asserts" 
LDADD_PEANO_2D_TRACE="   -lPeano4Core2d_trace   -lTarch_trace"
LDADD_PEANO_2D_DEBUG="   -lPeano4Core2d_debug   -lTarch_debug"

LDADD_PEANO_3D_RELEASE=" -lPeano4Core3d         -lTarch"
LDADD_PEANO_3D_STATS="   -lPeano4Core3d_stats   -lTarch_stats"
LDADD_PEANO_3D_ASSERTS=" -lPeano4Core3d_asserts -lTarch_asserts" 
LDADD_PEANO_3D_TRACE="   -lPeano4Core3d_trace   -lTarch_trace"
LDADD_PEANO_3D_DEBUG="   -lPeano4Core3d_debug   -lTarch_debug"


#
# ===============================================================
#   Parallelisation backends
#
# These configurations establish the multithreading, MPI and GPU
# environment.
# ===============================================================
#

#
# Multithreading
#
AC_ARG_WITH(
  multithreading, 
  AS_HELP_STRING([--with-multithreading],[switch on multithreading [default=no,cpp,omp,tbb,sycl]]),
  [
    AC_MSG_CHECKING(for multithreading)
    case "${withval}" in
      no)     AC_MSG_RESULT([no multithreading]);                    multithreading=false ;;
      cpp)    AC_MSG_RESULT([build with C++11 threading support]);   multithreading=cpp ;;
      tbb)    AC_MSG_RESULT([build with TBB]);                       multithreading=tbb ;;
      omp)    AC_MSG_RESULT([build with OpenMP - please ensure you pass -fopenmp/-qopenmp/-openmp or similar to your CXX flags]); multithreading=omp ;;
      sycl)   AC_MSG_RESULT([build with SYCL tasking on host]);      multithreading=sycl ;;
      *)      AC_MSG_FAILURE([default=no (no multithreading). Use omp, cpp or sycl to define target]);            multithreading=false ;;
    esac
  ], 
  [ 
    AC_MSG_CHECKING(for multithreading); 
    AC_MSG_RESULT(no); multithreading=false
  ]
)

if test "$multithreading" == cpp; then
  AC_DEFINE([SharedCPP], [1], [SharedCPP])
  AC_SEARCH_LIBS(pthread_create,pthread)
fi

if test "$multithreading" == tbb; then
  AC_DEFINE([SharedTBB], [1], [SharedTBB])
fi

if test "$multithreading" == omp; then
  AC_CHECK_HEADERS([omp.h],[],[AC_MSG_WARN([header <omp.h> not found])])
  AC_DEFINE([SharedOMP], [1], [SharedOMP])
fi

if test "$multithreading" == sycl; then
  AC_CHECK_HEADERS([CL/sycl.hpp],[],[AC_MSG_WARN([header <CL/sycl.hpp> not found])])
  AC_DEFINE([SharedSYCL], [1], [SharedSYCL])
fi


#
# MPI  Options
#
# Logic: user provides mpi compiler command
#        check if that command exists
#        overwrite CXX to be the user supplied value
#
AC_ARG_WITH(mpi,
  AS_HELP_STRING([--with-mpi],[switch on distributed memory parallelisation [default=no, mpiCC command]]),
  [ AC_MSG_CHECKING(for mpi)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);                              mpi=false ;;
      *)     AC_MSG_RESULT(use mpi c++ command ${withval});  mpi=${withval}  ;;
    esac ],
  [ AC_MSG_CHECKING(for mpi); AC_MSG_RESULT(no); mpi=false ])
 
#AC_DEFUN([AC_PROG_USERMPI], [AC_CHECK_PROG(USERMPI,"${mpi}",yes)])


if test "$mpi" != false; then
  CXX=$mpi
  AC_DEFINE([Parallel], [1], [Parallel])
  
  AC_CHECK_PROG( USERMPI, $mpi, "yes" )
  if test x"$USERMPI" != x"yes" ; then
    AC_MSG_WARN([The specified MPI compiler (${mpi}) was not found in the path. If it is a global path, please ignore this message.])
  fi
fi


#
# GPU support
#
AC_ARG_WITH(
  gpu, 
  AS_HELP_STRING([--with-gpu],[build with the GPU support. Not all targets are supported for all programming models, but you will need some threading model enabled [default=no,omp,hip,sycl,cpp]]),
  [
    AC_MSG_CHECKING(for gpu)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);                                    gpu=false ;;
      omp)   AC_MSG_RESULT(build with OpenMP offloading to GPUs);  gpu=omp ;;
      hip)   AC_MSG_RESULT(build with HIP GPU support);            gpu=hip ;;
      cpp)   AC_MSG_RESULT(build with C++ GPU support);            gpu=cpp ;;
      sycl)  AC_MSG_RESULT(build with SYCL GPU support);           gpu=sycl ;;
      cuda)  AC_MSG_RESULT(build with CUDA GPU suppport);          gpu=cuda ;;
      *)     AC_MSG_FAILURE([use omp, cpp or sycl to define target]);            gpu=false ;;
    esac
  ], 
  [ 
    AC_MSG_CHECKING(for gpu); 
    AC_MSG_RESULT(no); gpu=false
  ]
)


AM_CONDITIONAL([GPUOffloadingOMP],    [test "$gpu" == omp])
AM_CONDITIONAL([GPUOffloadingSYCL],   [test "$gpu" == sycl])
AM_CONDITIONAL([GPUOffloadingHIP],    [test "$gpu" == hip])
AM_CONDITIONAL([GPUOffloadingCPP],    [test "$gpu" == cpp])
AM_CONDITIONAL([GPUOffloadingCUDA],   [test "$gpu" == cuda])

if test "$gpu" == omp; then
  AC_CHECK_HEADERS([omp.h],[],[AC_MSG_WARN([header <omp.h> not found])])
  AC_DEFINE([GPUOffloadingOMP], [1], [GPUOffloadingOMP])
fi

if test "$gpu" == sycl; then
  #AC_CHECK_HEADERS([omp.h],[],[AC_MSG_WARN([header <omp.h> not found])])
  AC_DEFINE([GPUOffloadingSYCL], [1], [GPUOffloadingSYCL])
fi

if test "$gpu" == hip; then
  #AC_CHECK_HEADERS([omp.h],[],[AC_MSG_WARN([header <omp.h> not found])])
  AC_DEFINE([GPUOffloadingHIP], [1], [GPUOffloadingHIP])
fi

if test "$gpu" == cpp; then
  #AC_CHECK_HEADERS([omp.h],[],[AC_MSG_WARN([header <omp.h> not found])])
  AC_DEFINE([GPUOffloadingCPP], [1], [GPUOffloadingCPP])
fi

if test "$gpu" == cuda; then
  AC_LANG_DEFINE([CUDA], [cuda], [CUDA], [CUDACXX],
  [])

  AC_DEFINE([GPUOffloadingCUDA], [1], [GPUOffloadingCUDA])

  AC_ARG_WITH([cuda],
    [AS_HELP_STRING([--with-cuda=PATH], [specify CUDA installation path])],
    [with_cuda="$withval"],
    [with_cuda=""])

  # We need to apply further checks on CUDA
  if test -n "$with_cuda"
  then
    # Check the option has argument (empty argument results with a "yes")
    if test "$with_cuda" = yes
    then
      AC_MSG_ERROR([The argument --with-cuda requires the path to CUDA, --with-cuda=<CUDA_PATH>])
    fi

    # Extract the path from the "--with-cuda" argument and check if the path is valid
    cuda_path=$(echo "$with_cuda" | sed 's/--with-cuda=//')

    # Check if the path is valid
    if test ! -d "$cuda_path"; then
      AC_MSG_ERROR([The input for cuda-path=$cuda_path is not a valid directory])
    fi

    CUDA_INCLUDE_FLAGS="-I$with_cuda/include"
    CUDA_LDFLAGS="-L$with_cuda/lib64 -L$with_cuda/lib"
    CUDA_LIBS="-lcudart"
    NVCC="$with_cuda/bin/nvcc"
    CUDA_PATH="$with_cuda"

    AC_SUBST(CUDA_INCLUDE_FLAGS)
    AC_SUBST(CUDA_LDFLAGS)
    AC_SUBST(CUDA_LIBS)
    AC_SUBST(NVCC)
    AC_SUBST(CUDA_PATH)
  else
    AC_MSG_ERROR([If the argument --with-gpu=cuda is geven, --with-cuda=<CUDA_PATH> argument is required with the path to CUDA])
  fi

  CXXFLAGS_TARCH_RELEASE="$CUDA_INCLUDE_FLAGS"
  CXXFLAGS_TARCH_STATS="  $CUDA_INCLUDE_FLAGS"
  CXXFLAGS_TARCH_ASSERTS="$CUDA_INCLUDE_FLAGS"
  CXXFLAGS_TARCH_TRACE="  $CUDA_INCLUDE_FLAGS"
  CXXFLAGS_TARCH_DEBUG="  $CUDA_INCLUDE_FLAGS"

  LDADD_TARCH_RELEASE=" $CUDA_LIBS"
  LDADD_TARCH_STATS="   $CUDA_LIBS"
  LDADD_TARCH_ASSERTS=" $CUDA_LIBS" 
  LDADD_TARCH_TRACE="   $CUDA_LIBS"
  LDADD_TARCH_DEBUG="   $CUDA_LIBS"

  LDFLAGS_TARCH_RELEASE+=" $CUDA_LDFLAGS"
  LDFLAGS_TARCH_STATS+="   $CUDA_LDFLAGS"
  LDFLAGS_TARCH_ASSERTS+=" $CUDA_LDFLAGS"
  LDFLAGS_TARCH_TRACE+="   $CUDA_LDFLAGS"
  LDFLAGS_TARCH_DEBUG+="   $CUDA_LDFLAGS"

  CXXFLAGS_PEANO_2D_RELEASE=" $CXXFLAGS_PEANO_2D_RELEASE $CXXFLAGS_TARCH_RELEASE"
  CXXFLAGS_PEANO_2D_STATS="   $CXXFLAGS_PEANO_2D_STATS   $CXXFLAGS_TARCH_STATS"
  CXXFLAGS_PEANO_2D_ASSERTS=" $CXXFLAGS_PEANO_2D_ASSERTS $CXXFLAGS_TARCH_ASSERTS"
  CXXFLAGS_PEANO_2D_TRACE="   $CXXFLAGS_PEANO_2D_TRACE   $CXXFLAGS_TARCH_TRACE"
  CXXFLAGS_PEANO_2D_DEBUG="   $CXXFLAGS_PEANO_2D_DEBUG   $CXXFLAGS_TARCH_DEBUG"

  CXXFLAGS_PEANO_3D_RELEASE=" $CXXFLAGS_PEANO_3D_RELEASE $CXXFLAGS_TARCH_RELEASE"
  CXXFLAGS_PEANO_3D_STATS="   $CXXFLAGS_PEANO_3D_STATS   $CXXFLAGS_TARCH_STATS"
  CXXFLAGS_PEANO_3D_ASSERTS=" $CXXFLAGS_PEANO_3D_ASSERTS $CXXFLAGS_TARCH_ASSERTS"
  CXXFLAGS_PEANO_3D_TRACE="   $CXXFLAGS_PEANO_3D_TRACE   $CXXFLAGS_TARCH_TRACE"
  CXXFLAGS_PEANO_3D_DEBUG="   $CXXFLAGS_PEANO_3D_DEBUG   $CXXFLAGS_TARCH_DEBUG"

  LDFLAGS_PEANO_RELEASE=" $LDFLAGS_PEANO_RELEASE $LDFLAGS_TARCH_RELEASE"
  LDFLAGS_PEANO_STATS="   $LDFLAGS_PEANO_STATS   $LDFLAGS_TARCH_STATS"
  LDFLAGS_PEANO_ASSERTS=" $LDFLAGS_PEANO_ASSERTS $LDFLAGS_TARCH_ASSERTS"
  LDFLAGS_PEANO_TRACE="   $LDFLAGS_PEANO_TRACE   $LDFLAGS_TARCH_TRACE"
  LDFLAGS_PEANO_DEBUG="   $LDFLAGS_PEANO_DEBUG   $LDFLAGS_TARCH_DEBUG"

  LDADD_PEANO_2D_RELEASE="$LDADD_PEANO_2D_RELEASE $LDADD_TARCH_RELEASE"
  LDADD_PEANO_2D_STATS="  $LDADD_PEANO_2D_STATS   $LDADD_TARCH_STATS"
  LDADD_PEANO_2D_ASSERTS="$LDADD_PEANO_2D_ASSERTS $LDADD_TARCH_ASSERTS"
  LDADD_PEANO_2D_TRACE="  $LDADD_PEANO_2D_TRACE   $LDADD_TARCH_TRACE"
  LDADD_PEANO_2D_DEBUG="  $LDADD_PEANO_2D_DEBUG   $LDADD_TARCH_DEBUG"

  LDADD_PEANO_3D_RELEASE="$LDADD_PEANO_3D_RELEASE $LDADD_TARCH_RELEASE"
  LDADD_PEANO_3D_STATS="  $LDADD_PEANO_3D_STATS   $LDADD_TARCH_STATS"
  LDADD_PEANO_3D_ASSERTS="$LDADD_PEANO_3D_ASSERTS $LDADD_TARCH_ASSERTS"
  LDADD_PEANO_3D_TRACE="  $LDADD_PEANO_3D_TRACE   $LDADD_TARCH_TRACE"
  LDADD_PEANO_3D_DEBUG="  $LDADD_PEANO_3D_DEBUG   $LDADD_TARCH_DEBUG"
fi


#
# PETSc
#
AC_ARG_WITH(petsc,
  AS_HELP_STRING([--with-petsc],[switch on PETSc support]),
  [ AC_MSG_CHECKING(for petsc)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);                              petsc=false ;;
      *)     AC_MSG_RESULT(yes);                             petsc=${withval}  ;;
    esac ],
  [ AC_MSG_CHECKING(for petsc); AC_MSG_RESULT(no); petsc=false ])

AM_CONDITIONAL([UsePETSc],    [test "$petsc" != petsc])


#
# ===============================================================
#   Postprocessing and I/O
# ===============================================================
#

#
# VTK
#
AC_ARG_WITH(vtk,
  AS_HELP_STRING([--with-vtk],[enable VTK and thus build Peano's conversion tools [default=no, prefix]. prefix is where VTK include files are to be found, such as /usr/local/include/vtk-8.90]),
  [ AC_MSG_CHECKING(for vtk)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);     vtk_include=false ;;
      yes)   AC_MSG_RESULT(build against VTK installation in default location (/usr/include));    vtk_include="/usr/include"  ;;
      *)     AC_MSG_RESULT(build against VTK installation);    vtk_include=${withval}  ;;
    esac ],
  [ AC_MSG_CHECKING(for vtk); AC_MSG_RESULT(no); vtk_include=false ])
  

AM_CONDITIONAL([UseVTK],[test "$vtk_include" != false])


if test "$vtk_include" != false; then
  AC_DEFINE([UseVTK], [1], [UseVTK])
  AC_CHECK_FILE([$vtk_include/vtkVersion.h], [vtkFound="OK"])
  VTK_INCLUDE=-I$vtk_include
  AC_MSG_RESULT(use VTK include argument $VTK_INCLUDE)
 
  AC_SUBST([VTK_INCLUDE])

  #
  # If you specify a version, then I use this one. Otherwise, I search $vtk_include  
  #
  AC_ARG_WITH(vtk_version,
    AS_HELP_STRING([--with-vtk-version],[specify vtk version (major) [default=autodetect]]),
    [ AC_MSG_CHECKING(for vtk-version)
      case "${withval}" in
      *)     AC_MSG_RESULT(build against VTK version ${withval});    vtk_version=${withval}  ;;
      esac ],
    [ AC_MSG_CHECKING(for vtk-version); AC_MSG_RESULT(not set - try to autodetect); vtk_version="autodetect" ])

  if test x$vtk_version == x"autodetect"; then
    vtk_version_line=$(grep "define VTK_MAJOR_VERSION " $vtk_include/*.h)
    vtk_version=$(echo $vtk_version_line  | rev | cut -d' ' -f 1 | rev)
 
    if test x$vtk_version != "x"; then
      AC_MSG_NOTICE([detected VTK version $vtk_version])
    else
      AC_MSG_ERROR([no VTK manually specified and no VTK version could be detected at VTK prefix '$vtk_include'!])
    fi
  fi


  AC_ARG_WITH(vtk-suffix,
    AS_HELP_STRING([--with-vtk-suffix],[specify which suffix the vtk libraries carry [default=autodetect, use for no prefix]]),
    [ AC_MSG_CHECKING(for vtk-suffix)
      case "${withval}" in
        yes)   AC_MSG_RESULT(assume VTK libraries have no suffix);    vtk_suffix=""  ;;
	no)    AC_MSG_RESULT(not set - try to autodetect); vtk_suffix="autodetect";;
        *)     AC_MSG_RESULT(use suffix ${withval});    vtk_suffix=${withval}  ;;
      esac 
    ],  
    [ AC_MSG_CHECKING(for vtk-suffix); AC_MSG_RESULT(not set - try to autodetect); vtk_suffix="autodetect" ])


  #
  # So far, every VTK version we had tested had vtkIOCore, so we test this one. On the 
  # long term, there might be a need for ifs here checking the version.  
  # 
  # You have to very careful with the spaces here. A missing space before/after the ==
  # destroys the semantics. The x that I add before rhs and lhs of the comparison is
  # a cheap trick. It ensures that the expression is valid even if vtk_suffix is 
  # empty
  # 

  if test x$vtk_suffix == x"autodetect"; then
    vtk_version_line=$(grep "define VTK_MAJOR_VERSION " $vtk_include/*.h)
    vtk_major_version=$(echo $vtk_version_line  | rev | cut -d' ' -f 1 | rev)
    vtk_version_line=$(grep "define VTK_MINOR_VERSION " $vtk_include/*.h)
    vtk_minor_version=$(echo $vtk_version_line  | rev | cut -d' ' -f 1 | rev)
    vtk_suffix=-$vtk_major_version.$vtk_minor_version
    AC_MSG_RESULT(assume library suffix is $vtk_suffix)
  fi

  AC_CHECK_LIB(vtkIOCore$vtk_suffix,main,,[AC_MSG_ERROR([vtk library not found])])
  VTK_LIBRARY_SUFFIX=$vtk_suffix
  AC_SUBST([VTK_LIBRARY_SUFFIX])
fi


AM_CONDITIONAL([VTK_LIBRARY_VERSION_7],[test "$vtk_version" = "7"])
AM_CONDITIONAL([VTK_LIBRARY_VERSION_8],[test "$vtk_version" = "8"])
AM_CONDITIONAL([VTK_LIBRARY_VERSION_9],[test "$vtk_version" = "9"])


#
# HDF5
#
# This macro defines the variable with_hdf5
#
#AX_LIB_HDF5


AC_ARG_WITH(hdf5,
  AS_HELP_STRING([--with-hdf5],[enable HDF5 support [default=no]]),
  [ AC_MSG_CHECKING(for hdf5)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);                       use_hdf5=false ;;
      *)     AC_MSG_RESULT(build with HDF5 support);  use_hdf5=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for hdf5); AC_MSG_RESULT(no); use_hdf5=false ])
 
if test "$use_hdf5" == true; then
  AC_DEFINE([UseHDF5], [1], [UseHDF5])
fi 


#
# Otter
#
AC_ARG_WITH(otter,
  AS_HELP_STRING([--with-otter],[enable Otter [default=false]]),
  [ AC_MSG_CHECKING(for otter)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    use_otter=false ;;
      *)     AC_MSG_RESULT(yes);   use_otter=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for otter); AC_MSG_RESULT(no); use_otter=false ])
AM_CONDITIONAL([UseOtter],[test "x$use_otter" = xtrue])

# internal toolboxes don't need ifdefs, but here we need them
if test "$use_otter" == true; then
  AC_DEFINE([UseOtter], [UseOtter], [UseOtter])
fi


#
# libxsmm
#
AC_ARG_WITH(libxsmm,
  AS_HELP_STRING([--with-libxsmm],[enable libxsmm [default=false]]),
  [ AC_MSG_CHECKING(for libxsmm)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    use_libxsmm=false ;;
      *)     AC_MSG_RESULT(yes);   use_libxsmm=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for libxsmm); AC_MSG_RESULT(no); use_libxsmm=false ])
AM_CONDITIONAL([UseLibXSMM],[test "x$use_libxsmm" = xtrue])

if test "$use_libxsmm" == true; then
  AC_DEFINE([UseLibXSMM], [UseLibXSMM], [UseLibXSMM])
fi



#
# ===============================================================
#   Floating-point exception handling
# ===============================================================
#

AC_ARG_WITH(fenv,
  AS_HELP_STRING([--with-fenv],[build with floating-point exception handling [default=false]]),
  [ AC_MSG_CHECKING(for fenv)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    use_fenv=false ;;
      *)     AC_MSG_RESULT(yes);   use_fenv=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for fenv); AC_MSG_RESULT(no); use_fenv=false ])
AM_CONDITIONAL([UseFenv],[test "x$use_fenv" = xtrue])

if test "$use_fenv" == true; then
  AC_DEFINE([UseFenv], [UseFenv], [UseFenv])
fi

#
# ===============================================================
#   Tailored toolchains
#
# A toolchain in Peano in particular, bespoke compile and build 
# settings that work well with particular (performance) analysis
# tools or a suite of dev tools by a particular vendor, or some 
# bespoke postprocessing tools.
# ===============================================================
#

#
# Vendor-specific toolchain (logging devices, memory allocation libraries, ...)
#
AC_ARG_WITH(toolchain,
  AS_HELP_STRING([--with-toolchain],[build with vendor-specific toolchain [default=no,itac,itt,nvptx,scorep,chrome,gnu]]),
  [ AC_MSG_CHECKING(for toolchain)
    case "${withval}" in
      no)        AC_MSG_RESULT(no);                                           toolchain=false ;;
      itac)      AC_MSG_RESULT(build with support for the Intel's ITAC);      toolchain=itac  ;;
      itt)       AC_MSG_RESULT(build with support for the Intel's Instrumentation and Tracing Technology (ITT));      toolchain=itt  ;;
      nvidia)    AC_MSG_RESULT(build with support for the NVIDIA SDK);        toolchain=nvidia ;;
      scorep)    AC_MSG_RESULT(build with support for the Score-P);           toolchain=scorep ;;
      chrome)    AC_MSG_RESULT(build with logging support for Google Chrome); toolchain=chrome ;;
      gnu)       AC_MSG_RESULT(build with specialised GNU flags); toolchain=gnu ;;
      *)         AC_MSG_FAILURE([default=no (no bespoke toolchain)]); toolchain=false ;;
    esac 
  ],
  [ 
    AC_MSG_CHECKING(for toolchain);
    AC_MSG_RESULT(no); toolchain=false
  ]
)

if test "$toolchain" == itac; then
  AC_DEFINE([UseITAC], [1], [UseITAC])
  AC_DEFINE([UseLogService], [ITACLogger], [ITACLogger])
  
  CXXFLAGS_PEANO_2D_RELEASE+=" -DTBB_USE_THREADING_TOOLS -parallel-source-info=2"
  CXXFLAGS_PEANO_2D_STATS+="   -DTBB_USE_THREADING_TOOLS -parallel-source-info=2"
  CXXFLAGS_PEANO_2D_ASSERTS+=" -DTBB_USE_THREADING_TOOLS -parallel-source-info=2 -DTBB_USE_ASSERT"
  CXXFLAGS_PEANO_2D_TRACE+="   -DTBB_USE_THREADING_TOOLS -parallel-source-info=2"
  CXXFLAGS_PEANO_2D_DEBUG+="   -DTBB_USE_THREADING_TOOLS -parallel-source-info=2 -DTBB_USE_ASSERT"
  
  CXXFLAGS_PEANO_3D_RELEASE+=" -DTBB_USE_THREADING_TOOLS -parallel-source-info=2"
  CXXFLAGS_PEANO_3D_STATS+="   -DTBB_USE_THREADING_TOOLS -parallel-source-info=2"
  CXXFLAGS_PEANO_3D_ASSERTS+=" -DTBB_USE_THREADING_TOOLS -parallel-source-info=2 -DTBB_USE_ASSERT"
  CXXFLAGS_PEANO_3D_TRACE+="   -DTBB_USE_THREADING_TOOLS -parallel-source-info=2"
  CXXFLAGS_PEANO_3D_DEBUG+="   -DTBB_USE_THREADING_TOOLS -parallel-source-info=2 -DTBB_USE_ASSERT"
fi


if test "$toolchain" == itt; then
  AC_DEFINE([UseITT], [1], [UseITT])
  AC_DEFINE([UseLogService], [ITTLogger], [ITTLogger])
  
  CXXFLAGS_PEANO_2D_RELEASE+=" -DTBB_USE_THREADING_TOOLS -parallel-source-info=2"
  CXXFLAGS_PEANO_2D_STATS+="   -DTBB_USE_THREADING_TOOLS -parallel-source-info=2"
  CXXFLAGS_PEANO_2D_ASSERTS+=" -DTBB_USE_THREADING_TOOLS -parallel-source-info=2 -DTBB_USE_ASSERT"
  CXXFLAGS_PEANO_2D_TRACE+="   -DTBB_USE_THREADING_TOOLS -parallel-source-info=2"
  CXXFLAGS_PEANO_2D_DEBUG+="   -DTBB_USE_THREADING_TOOLS -parallel-source-info=2 -DTBB_USE_ASSERT"
  
  CXXFLAGS_PEANO_3D_RELEASE+=" -DTBB_USE_THREADING_TOOLS -parallel-source-info=2"
  CXXFLAGS_PEANO_3D_STATS+="   -DTBB_USE_THREADING_TOOLS -parallel-source-info=2"
  CXXFLAGS_PEANO_3D_ASSERTS+=" -DTBB_USE_THREADING_TOOLS -parallel-source-info=2 -DTBB_USE_ASSERT"
  CXXFLAGS_PEANO_3D_TRACE+="   -DTBB_USE_THREADING_TOOLS -parallel-source-info=2"
  CXXFLAGS_PEANO_3D_DEBUG+="   -DTBB_USE_THREADING_TOOLS -parallel-source-info=2 -DTBB_USE_ASSERT"
fi


if test "$toolchain" == scorep; then
  AC_DEFINE([UseLogService], [ScorePLogger], [ScorePLogger])
  AC_DEFINE([UseScoreP],     [UseScoreP], [UseScoreP])
  #LDFLAGS+=-ipo
  #CXXFLAGS+=" -ip -ipo "
fi

if test "$toolchain" == nvptx; then
  AC_DEFINE([UseNVIDIA], [1], [UseNVIDIA])
  AC_DEFINE([UseLogService], [NVTXLogger], [NVTXLogger])
  AC_CHECK_HEADERS([nvtx3/nvToolsExt.h],[],[AC_MSG_ERROR([header <nvtx3/nvToolsExt.h> not found])])
fi

if test "$toolchain" == chrome; then
  AC_DEFINE([UseLogService], [ChromeTraceFileLogger], [ChromeTraceFileLogger])
fi

if test "$toolchain" == gnu; then
  LDFLAGS+=-flto
fi




#
# ===============================================================
#   Toolboxes (actives with enable)
#
# A toolbox is an optional extension of Peano (package). As there
# are dependencies between the toolboxes, this part only analyses
# whether a toolbox should be used or not. It does not react to 
# this choice.
# ===============================================================
#


#
# FEM 
#
AC_ARG_ENABLE(finiteelements,
  AS_HELP_STRING([--enable-finiteelements],[enable finite elements toolbox  [default=false]]),
  [ AC_MSG_CHECKING(for finiteelements)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    toolbox_finiteelements=false ;;
      *)     AC_MSG_RESULT(yes);   toolbox_finiteelements=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for finiteelements); AC_MSG_RESULT(no); toolbox_finiteelements=false ])
AM_CONDITIONAL([UseToolboxFiniteElements],[test "x$toolbox_finiteelements" = xtrue])


#
# Multiprecision 
#
AC_ARG_ENABLE(multiprecision,
  AS_HELP_STRING([--enable-multiprecision],[enable multiprecision  [default=false]]),
  [ AC_MSG_CHECKING(for multiprecision)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    toolbox_multiprecision=false ;;
      *)     AC_MSG_RESULT(yes);   toolbox_multiprecision=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for multiprecision); AC_MSG_RESULT(no); toolbox_multiprecision=false ])
AM_CONDITIONAL([UseToolboxMultiprecision],[test "x$toolbox_multiprecision" = xtrue])
 
 
#
# Load balancing 
#
AC_ARG_ENABLE(loadbalancing,
  AS_HELP_STRING([--enable-loadbalancing],[enable loadbalancing  [default=false]]),
  [ AC_MSG_CHECKING(for loadbalancing)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    toolbox_loadbalancing=false ;;
      *)     AC_MSG_RESULT(yes);   toolbox_loadbalancing=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for loadbalancing); AC_MSG_RESULT(no); toolbox_loadbalancing=false ])
AM_CONDITIONAL([UseToolboxLoadBalancing],[test "x$toolbox_loadbalancing" = xtrue])


#
# Particles 
#
AC_ARG_ENABLE(particles,
  AS_HELP_STRING([--enable-particles],[enable particles  [default=false]]),
  [ AC_MSG_CHECKING(for particles)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    toolbox_particles=false ;;
      *)     AC_MSG_RESULT(yes);   toolbox_particles=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for particles); AC_MSG_RESULT(no); toolbox_particles=false ])
AM_CONDITIONAL([UseToolboxParticles],[test "x$toolbox_particles" = xtrue])


#
# ExaHyPE2
#
AC_ARG_ENABLE(exahype,
  AS_HELP_STRING([--enable-exahype],[enable ExaHyPE2 [default=false]]),
  [ AC_MSG_CHECKING(for exahype)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    toolbox_exahype=false ;;
      *)     AC_MSG_RESULT(yes);   toolbox_exahype=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for exahype); AC_MSG_RESULT(no); toolbox_exahype=false ])
AM_CONDITIONAL([UseExaHyPE],[test "x$toolbox_exahype" = xtrue])


#
# SWIFT2
#
AC_ARG_ENABLE(swift,
  AS_HELP_STRING([--enable-swift],[enable SWIFT2 [default=false]]),
  [ AC_MSG_CHECKING(for swift)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    toolbox_swift=false ;;
      *)     AC_MSG_RESULT(yes);   toolbox_swift=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for swift); AC_MSG_RESULT(no); toolbox_swift=false ])
AM_CONDITIONAL([UseSWIFT],[test "x$toolbox_swift" = xtrue])


#
# Blockstructured
#
AC_ARG_ENABLE(blockstructured,
  AS_HELP_STRING([--enable-blockstructured],[enable blockstructured (SAMR) support [default=false]]),
  [ AC_MSG_CHECKING(for blockstructured)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    toolbox_blockstructured=false ;;
      *)     AC_MSG_RESULT(yes);   toolbox_blockstructured=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for blockstructured); AC_MSG_RESULT(no); toolbox_blockstructured=false ])
AM_CONDITIONAL([UseToolboxBlockstructured],[test "x$toolbox_blockstructured" = xtrue])


#
# Delta
#
AC_ARG_WITH(delta,
  AS_HELP_STRING([--with-delta],[enable Delta [default=no]]),
  [ AC_MSG_CHECKING(for delta)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);   toolbox_delta=false ;;
      *)     AC_MSG_RESULT(yes);  toolbox_delta=${withval}  ;;
    esac ],
  [ AC_MSG_CHECKING(for delta); AC_MSG_RESULT(no); toolbox_delta=false ])
# This whole autotools thing is not very nice: If I wanna use an if ... endif (
# note the syntax which is different to the present file, then I have to declare
# the variable as I do in the line below. Otherwise, the if thing is not 
# "exported". Seems the autoconf and the makefile generator are completely 
# separate with inconsistent syntax.
AM_CONDITIONAL([UseDelta],[test "$toolbox_delta" != false])




#
# ===============================================================
#   Setup toolboxes and extensions
#
# Run through the set of toolboxes and extensions that are enabled, 
# configure them properly, i.e. augment the Peano compile and 
# linker flags, and check for consistency, i.e. whether all 
# dependencies are ok.
# ===============================================================
#


#
# FEM 
#
if test "$toolbox_finiteelements" != false; then
  AC_DEFINE([UseToolboxFiniteElements], [1], [UseToolboxFiniteElements])
  
  LDADD_PEANO_2D_RELEASE=" -lToolboxFiniteElements2d         $LDADD_PEANO_2D_RELEASE"
  LDADD_PEANO_2D_STATS="   -lToolboxFiniteElements2d_stats   $LDADD_PEANO_2D_STATS "
  LDADD_PEANO_2D_ASSERTS=" -lToolboxFiniteElements2d_asserts $LDADD_PEANO_2D_ASSERTS " 
  LDADD_PEANO_2D_TRACE="   -lToolboxFiniteElements2d_trace   $LDADD_PEANO_2D_TRACE "
  LDADD_PEANO_2D_DEBUG="   -lToolboxFiniteElements2d_debug   $LDADD_PEANO_2D_DEBUG "

  LDADD_PEANO_3D_RELEASE=" -lToolboxFiniteElements3d         $LDADD_PEANO_3D_RELEASE"
  LDADD_PEANO_3D_STATS="   -lToolboxFiniteElements3d_stats   $LDADD_PEANO_3D_STATS "
  LDADD_PEANO_3D_ASSERTS=" -lToolboxFiniteElements3d_asserts $LDADD_PEANO_3D_ASSERTS " 
  LDADD_PEANO_3D_TRACE="   -lToolboxFiniteElements3d_trace   $LDADD_PEANO_3D_TRACE "
  LDADD_PEANO_3D_DEBUG="   -lToolboxFiniteElements3d_debug   $LDADD_PEANO_3D_DEBUG "

  LDFLAGS_PEANO_RELEASE+=" -L$srcdir_full_path/src/toolbox/finiteelements "
  LDFLAGS_PEANO_STATS+="   -L$srcdir_full_path/src/toolbox/finiteelements "
  LDFLAGS_PEANO_ASSERTS+=" -L$srcdir_full_path/src/toolbox/finiteelements "
  LDFLAGS_PEANO_TRACE+="   -L$srcdir_full_path/src/toolbox/finiteelements "
  LDFLAGS_PEANO_DEBUG+="   -L$srcdir_full_path/src/toolbox/finiteelements "
fi


#
# Multiprecision 
#
if test "$toolbox_multiprecision" != false; then
  AC_DEFINE([UseToolboxMultiprecision], [1], [UseToolboxMultiprecision])
  
  LDADD_PEANO_2D_RELEASE=" -lToolboxMultiprecision         $LDADD_PEANO_2D_RELEASE "
  LDADD_PEANO_2D_STATS="   -lToolboxMultiprecision_stats   $LDADD_PEANO_2D_STATS "
  LDADD_PEANO_2D_ASSERTS=" -lToolboxMultiprecision_asserts $LDADD_PEANO_2D_ASSERTS " 
  LDADD_PEANO_2D_TRACE="   -lToolboxMultiprecision_trace   $LDADD_PEANO_2D_TRACE "
  LDADD_PEANO_2D_DEBUG="   -lToolboxMultiprecision_debug   $LDADD_PEANO_2D_DEBUG "

  LDADD_PEANO_3D_RELEASE=" -lToolboxMultiprecision         $LDADD_PEANO_3D_RELEASE "
  LDADD_PEANO_3D_STATS="   -lToolboxMultiprecision_stats   $LDADD_PEANO_3D_STATS "
  LDADD_PEANO_3D_ASSERTS=" -lToolboxMultiprecision_asserts $LDADD_PEANO_3D_ASSERTS " 
  LDADD_PEANO_3D_TRACE="   -lToolboxMultiprecision_trace   $LDADD_PEANO_3D_TRACE "
  LDADD_PEANO_3D_DEBUG="   -lToolboxMultiprecision_debug   $LDADD_PEANO_3D_DEBUG "

  LDFLAGS_PEANO_RELEASE+=" -L$srcdir_full_path/src/toolbox/multiprecision "
  LDFLAGS_PEANO_STATS+="   -L$srcdir_full_path/src/toolbox/multiprecision "
  LDFLAGS_PEANO_ASSERTS+=" -L$srcdir_full_path/src/toolbox/multiprecision "
  LDFLAGS_PEANO_TRACE+="   -L$srcdir_full_path/src/toolbox/multiprecision "
  LDFLAGS_PEANO_DEBUG+="   -L$srcdir_full_path/src/toolbox/multiprecision "
fi


#
# Load balancing 
#
if test "$toolbox_loadbalancing" != false; then
  AC_DEFINE([UseToolboxLoadBalancing], [1], [UseToolboxLoadBalancing])
  
  LDADD_PEANO_2D_RELEASE=" -lToolboxLoadBalancing2d         $LDADD_PEANO_2D_RELEASE "
  LDADD_PEANO_2D_STATS="   -lToolboxLoadBalancing2d_stats   $LDADD_PEANO_2D_STATS "
  LDADD_PEANO_2D_ASSERTS=" -lToolboxLoadBalancing2d_asserts $LDADD_PEANO_2D_ASSERTS " 
  LDADD_PEANO_2D_TRACE="   -lToolboxLoadBalancing2d_trace   $LDADD_PEANO_2D_TRACE "
  LDADD_PEANO_2D_DEBUG="   -lToolboxLoadBalancing2d_debug   $LDADD_PEANO_2D_DEBUG "

  LDADD_PEANO_3D_RELEASE=" -lToolboxLoadBalancing3d         $LDADD_PEANO_3D_RELEASE "
  LDADD_PEANO_3D_STATS="   -lToolboxLoadBalancing3d_stats   $LDADD_PEANO_3D_STATS "
  LDADD_PEANO_3D_ASSERTS=" -lToolboxLoadBalancing3d_asserts $LDADD_PEANO_3D_ASSERTS " 
  LDADD_PEANO_3D_TRACE="   -lToolboxLoadBalancing3d_trace   $LDADD_PEANO_3D_TRACE "
  LDADD_PEANO_3D_DEBUG="   -lToolboxLoadBalancing3d_debug   $LDADD_PEANO_3D_DEBUG "

  LDFLAGS_PEANO_RELEASE+=" -L$srcdir_full_path/src/toolbox/loadbalancing "
  LDFLAGS_PEANO_STATS+="   -L$srcdir_full_path/src/toolbox/loadbalancing "
  LDFLAGS_PEANO_ASSERTS+=" -L$srcdir_full_path/src/toolbox/loadbalancing "
  LDFLAGS_PEANO_TRACE+="   -L$srcdir_full_path/src/toolbox/loadbalancing "
  LDFLAGS_PEANO_DEBUG+="   -L$srcdir_full_path/src/toolbox/loadbalancing "
fi


#
# Blockstructured
#
if test "$toolbox_blockstructured" != false; then
  AC_DEFINE([UseToolboxBlockstructured], [1], [UseToolboxBlockstructured])
  
  LDADD_PEANO_2D_RELEASE=" -lToolboxBlockstructured2d         $LDADD_PEANO_2D_RELEASE "
  LDADD_PEANO_2D_STATS="   -lToolboxBlockstructured2d_stats   $LDADD_PEANO_2D_STATS "
  LDADD_PEANO_2D_ASSERTS=" -lToolboxBlockstructured2d_asserts $LDADD_PEANO_2D_ASSERTS " 
  LDADD_PEANO_2D_TRACE="   -lToolboxBlockstructured2d_trace   $LDADD_PEANO_2D_TRACE "
  LDADD_PEANO_2D_DEBUG="   -lToolboxBlockstructured2d_debug   $LDADD_PEANO_2D_DEBUG "

  LDADD_PEANO_3D_RELEASE=" -lToolboxBlockstructured3d         $LDADD_PEANO_3D_RELEASE "
  LDADD_PEANO_3D_STATS="   -lToolboxBlockstructured3d_stats   $LDADD_PEANO_3D_STATS "
  LDADD_PEANO_3D_ASSERTS=" -lToolboxBlockstructured3d_asserts $LDADD_PEANO_3D_ASSERTS " 
  LDADD_PEANO_3D_TRACE="   -lToolboxBlockstructured3d_trace   $LDADD_PEANO_3D_TRACE "
  LDADD_PEANO_3D_DEBUG="   -lToolboxBlockstructured3d_debug   $LDADD_PEANO_3D_DEBUG "

  LDFLAGS_PEANO_RELEASE+=" -L$srcdir_full_path/src/toolbox/blockstructured "
  LDFLAGS_PEANO_STATS+="   -L$srcdir_full_path/src/toolbox/blockstructured "
  LDFLAGS_PEANO_ASSERTS+=" -L$srcdir_full_path/src/toolbox/blockstructured "
  LDFLAGS_PEANO_TRACE+="   -L$srcdir_full_path/src/toolbox/blockstructured "
  LDFLAGS_PEANO_DEBUG+="   -L$srcdir_full_path/src/toolbox/blockstructured "
fi


#
# Particles 
#
if test "$toolbox_particles" != false; then
  AC_DEFINE([UseToolboxParticles], [1], [UseToolboxParticles])
  
  LDADD_PEANO_2D_RELEASE+=" -lToolboxParticles2d         $LDADD_PEANO_2D_RELEASE " 
  LDADD_PEANO_2D_STATS+="   -lToolboxParticles2d_stats   $LDADD_PEANO_2D_STATS "
  LDADD_PEANO_2D_ASSERTS+=" -lToolboxParticles2d_asserts $LDADD_PEANO_2D_ASSERTS " 
  LDADD_PEANO_2D_TRACE+="   -lToolboxParticles2d_trace   $LDADD_PEANO_2D_TRACE "
  LDADD_PEANO_2D_DEBUG+="   -lToolboxParticles2d_debug   $LDADD_PEANO_2D_DEBUG "

  LDADD_PEANO_3D_RELEASE+=" -lToolboxParticles3d         $LDADD_PEANO_3D_RELEASE " 
  LDADD_PEANO_3D_STATS+="   -lToolboxParticles3d_stats   $LDADD_PEANO_3D_STATS "
  LDADD_PEANO_3D_ASSERTS+=" -lToolboxParticles3d_asserts $LDADD_PEANO_3D_ASSERTS " 
  LDADD_PEANO_3D_TRACE+="   -lToolboxParticles3d_trace   $LDADD_PEANO_3D_TRACE "
  LDADD_PEANO_3D_DEBUG+="   -lToolboxParticles3d_debug   $LDADD_PEANO_3D_DEBUG "

  LDFLAGS_PEANO_RELEASE+=" -L$srcdir_full_path/src/toolbox/particles "
  LDFLAGS_PEANO_STATS+="   -L$srcdir_full_path/src/toolbox/particles "
  LDFLAGS_PEANO_ASSERTS+=" -L$srcdir_full_path/src/toolbox/particles "
  LDFLAGS_PEANO_TRACE+="   -L$srcdir_full_path/src/toolbox/particles "
  LDFLAGS_PEANO_DEBUG+="   -L$srcdir_full_path/src/toolbox/particles "
fi


#
# PETSC
#
if test "$petsc" != false; then
  AC_DEFINE([UsePETSc], [1], [UsePETSc])
  if test "$toolbox_finiteelements" == false; then
    AC_MSG_ERROR([PETSc requires the finite elements toolbox (run with --help to see the correct enable statement)])
  fi
  LDADD_PEANO_2D_RELEASE=" -lPETSc         $LDADD_PEANO_2D_RELEASE "
  LDADD_PEANO_2D_STATS="   -lPETSc_stats   $LDADD_PEANO_2D_STATS "
  LDADD_PEANO_2D_ASSERTS=" -lPETSc_asserts $LDADD_PEANO_2D_ASSERTS " 
  LDADD_PEANO_2D_TRACE="   -lPETSc_trace   $LDADD_PEANO_2D_TRACE "
  LDADD_PEANO_2D_DEBUG="   -lPETSc_debug   $LDADD_PEANO_2D_DEBUG "

  LDADD_PEANO_3D_RELEASE=" -lPETSc         $LDADD_PEANO_3D_RELEASE "
  LDADD_PEANO_3D_STATS="   -lPETSc_stats   $LDADD_PEANO_3D_STATS "
  LDADD_PEANO_3D_ASSERTS=" -lPETSc_asserts $LDADD_PEANO_3D_ASSERTS " 
  LDADD_PEANO_3D_TRACE="   -lPETSc_trace   $LDADD_PEANO_3D_TRACE "
  LDADD_PEANO_3D_DEBUG="   -lPETSc_debug   $LDADD_PEANO_3D_DEBUG "

  LDFLAGS_PEANO_RELEASE+=" -L$srcdir_full_path/src/petsc "
  LDFLAGS_PEANO_STATS+="   -L$srcdir_full_path/src/petsc "
  LDFLAGS_PEANO_ASSERTS+=" -L$srcdir_full_path/src/petsc "
  LDFLAGS_PEANO_TRACE+="   -L$srcdir_full_path/src/petsc "
  LDFLAGS_PEANO_DEBUG+="   -L$srcdir_full_path/src/petsc "
fi


#
# ExaHyPE2
#
if test "$toolbox_exahype" != false; then
  AC_DEFINE([UseExaHyPE], [1], [UseExaHyPE])
  if test "$toolbox_loadbalancing" == false; then
    AC_MSG_ERROR([ExaHyPE2 requires the load balancing toolbox (run with --help to see the correct enable statement)])
  fi
  if test "$toolbox_blockstructured" == false; then
    AC_MSG_ERROR([ExaHyPE2 requires the blockstructured toolbox (run with --help to see the correct enable statement)])
  fi
  LDADD_PEANO_2D_RELEASE=" -lExaHyPE2Core2d         $LDADD_PEANO_2D_RELEASE "
  LDADD_PEANO_2D_STATS="   -lExaHyPE2Core2d_stats   $LDADD_PEANO_2D_STATS "
  LDADD_PEANO_2D_ASSERTS=" -lExaHyPE2Core2d_asserts $LDADD_PEANO_2D_ASSERTS " 
  LDADD_PEANO_2D_TRACE="   -lExaHyPE2Core2d_trace   $LDADD_PEANO_2D_TRACE "
  LDADD_PEANO_2D_DEBUG="   -lExaHyPE2Core2d_debug   $LDADD_PEANO_2D_DEBUG "

  LDADD_PEANO_3D_RELEASE=" -lExaHyPE2Core3d         $LDADD_PEANO_3D_RELEASE "
  LDADD_PEANO_3D_STATS="   -lExaHyPE2Core3d_stats   $LDADD_PEANO_3D_STATS "
  LDADD_PEANO_3D_ASSERTS=" -lExaHyPE2Core3d_asserts $LDADD_PEANO_3D_ASSERTS " 
  LDADD_PEANO_3D_TRACE="   -lExaHyPE2Core3d_trace   $LDADD_PEANO_3D_TRACE "
  LDADD_PEANO_3D_DEBUG="   -lExaHyPE2Core3d_debug   $LDADD_PEANO_3D_DEBUG "

  LDFLAGS_PEANO_RELEASE+=" -L$srcdir_full_path/src/exahype2 "
  LDFLAGS_PEANO_STATS+="   -L$srcdir_full_path/src/exahype2 "
  LDFLAGS_PEANO_ASSERTS+=" -L$srcdir_full_path/src/exahype2 "
  LDFLAGS_PEANO_TRACE+="   -L$srcdir_full_path/src/exahype2 "
  LDFLAGS_PEANO_DEBUG+="   -L$srcdir_full_path/src/exahype2 "
fi


if test "$use_libxsmm" != false; then
  #Libxsmm
  echo "Checking whether libxsmm needs to be installed or built"
  if @<:@ ! -d submodules @:>@; then
    mkdir submodules
    echo "Created libxsmm directory in submodules"
  fi
  if @<:@ ! -e submodules/libxsmm/.git @:>@; then
    echo "Initializing libxsmm in submodules/libxsmm, this may take a while"
    cd submodules
    git clone https://github.com/libxsmm/libxsmm.git
    cd libxsmm
    git checkout 9b7bbb95c
    #Clean documentation to save space
    rm -rf samples/ #over 100M of not needed stuff
    rm -rf documentation/ #around 10M of documentation
    cd ../..
  fi
  
  # build libxsmm
  if @<:@ ! -d submodules/libxsmm/bin @:>@ || @<:@ ! -e submodules/libxsmm/bin/libxsmm_gemm_generator @:>@; then
    REBUILD_LIBXSMM=true
  fi
  if @<:@ "$REBUILD_LIBXSMM" = true @:>@; then
    echo "Building libxsmm gemm generator"
    cd submodules/libxsmm
    git restore samples
    make realclean --silent
    make generator --silent
    rm -rf samples
    cd ../..
  fi
  echo "Finished building libxsmm generator"


fi


#
# SWIFT2
#
if test "$toolbox_swift" != false; then
  AC_DEFINE([UseSWIFT], [1], [UseSWIFT])
  if test "$toolbox_loadbalancing" == false; then
    AC_MSG_ERROR([SWIFT2 requires the load balancing toolbox (run with --help to see the correct enable statement)])
  fi
  if test "$toolbox_particles" == false; then
    AC_MSG_ERROR([SWIFT2 requires the particles toolbox (run with --help to see the correct enable statement)])
  fi
  LDADD_PEANO_2D_RELEASE=" -lSWIFT2Core2d         $LDADD_PEANO_2D_RELEASE "
  LDADD_PEANO_2D_STATS="   -lSWIFT2Core2d_stats   $LDADD_PEANO_2D_STATS "
  LDADD_PEANO_2D_ASSERTS=" -lSWIFT2Core2d_asserts $LDADD_PEANO_2D_ASSERTS " 
  LDADD_PEANO_2D_TRACE="   -lSWIFT2Core2d_trace   $LDADD_PEANO_2D_TRACE "
  LDADD_PEANO_2D_DEBUG="   -lSWIFT2Core2d_debug   $LDADD_PEANO_2D_DEBUG "

  LDADD_PEANO_3D_RELEASE=" -lSWIFT2Core3d         $LDADD_PEANO_3D_RELEASE "
  LDADD_PEANO_3D_STATS="   -lSWIFT2Core3d_stats   $LDADD_PEANO_3D_STATS "
  LDADD_PEANO_3D_ASSERTS=" -lSWIFT2Core3d_asserts $LDADD_PEANO_3D_ASSERTS " 
  LDADD_PEANO_3D_TRACE="   -lSWIFT2Core3d_trace   $LDADD_PEANO_3D_TRACE "
  LDADD_PEANO_3D_DEBUG="   -lSWIFT2Core3d_debug   $LDADD_PEANO_3D_DEBUG "

  LDFLAGS_PEANO_RELEASE+=" -L$srcdir_full_path/src/swift2 "
  LDFLAGS_PEANO_STATS+="   -L$srcdir_full_path/src/swift2 "
  LDFLAGS_PEANO_ASSERTS+=" -L$srcdir_full_path/src/swift2 "
  LDFLAGS_PEANO_TRACE+="   -L$srcdir_full_path/src/swift2 "
  LDFLAGS_PEANO_DEBUG+="   -L$srcdir_full_path/src/swift2 "
fi


#
# SmartMPI
#
AC_ARG_WITH(smartmpi,
  AS_HELP_STRING([--with-smartmpi],[build with SmartMPI [default=no,Merged,Alternating]]),
  [ AC_MSG_CHECKING(for smartmpi)
    case "${withval}" in
      no)               AC_MSG_RESULT(no);    smartmpi=false ;;
      Merged)           AC_MSG_RESULT(yes; use topology Merged);            smartmpi=Merged ;;
      Alternating)      AC_MSG_RESULT(yes; use topology Alternating);       smartmpi=Alternating ;;
      *)         AC_MSG_RESULT(failed);   AC_MSG_ERROR([invalid option for SmartMPI]) ;;
    esac ],
  [ AC_MSG_CHECKING(for smartmpi); AC_MSG_RESULT(no); smartmpi=false ])
AM_CONDITIONAL([UseSmartMPI],[test "x$smartmpi" = xtrue])
if test "$smartmpi" == Merged; then
  AC_DEFINE([UseSmartMPI], [Merged], [UseSmartMPI])
fi
if test "$smartmpi" == Alternating; then
  AC_DEFINE([UseSmartMPI], [Alternating], [UseSmartMPI])
fi


#
# ===============================================================
#   Export variables
# ===============================================================
#
AC_SUBST(CXXFLAGS_PEANO_2D_RELEASE)
AC_SUBST(CXXFLAGS_PEANO_2D_STATS)
AC_SUBST(CXXFLAGS_PEANO_2D_ASSERTS)
AC_SUBST(CXXFLAGS_PEANO_2D_TRACE)
AC_SUBST(CXXFLAGS_PEANO_2D_DEBUG)

AC_SUBST(CXXFLAGS_PEANO_3D_RELEASE)
AC_SUBST(CXXFLAGS_PEANO_3D_STATS)
AC_SUBST(CXXFLAGS_PEANO_3D_ASSERTS)
AC_SUBST(CXXFLAGS_PEANO_3D_TRACE)
AC_SUBST(CXXFLAGS_PEANO_3D_DEBUG)

AC_SUBST(CXXFLAGS_TARCH_RELEASE)
AC_SUBST(CXXFLAGS_TARCH_STATS)
AC_SUBST(CXXFLAGS_TARCH_ASSERTS)
AC_SUBST(CXXFLAGS_TARCH_TRACE)
AC_SUBST(CXXFLAGS_TARCH_DEBUG)

AC_SUBST(LDFLAGS_PEANO_RELEASE)
AC_SUBST(LDFLAGS_PEANO_STATS)
AC_SUBST(LDFLAGS_PEANO_ASSERTS)
AC_SUBST(LDFLAGS_PEANO_TRACE)
AC_SUBST(LDFLAGS_PEANO_DEBUG)

AC_SUBST(LDADD_PEANO_2D_RELEASE)
AC_SUBST(LDADD_PEANO_2D_STATS)
AC_SUBST(LDADD_PEANO_2D_ASSERTS)
AC_SUBST(LDADD_PEANO_2D_TRACE)
AC_SUBST(LDADD_PEANO_2D_DEBUG)

AC_SUBST(LDADD_PEANO_3D_RELEASE)
AC_SUBST(LDADD_PEANO_3D_STATS)
AC_SUBST(LDADD_PEANO_3D_ASSERTS)
AC_SUBST(LDADD_PEANO_3D_TRACE)
AC_SUBST(LDADD_PEANO_3D_DEBUG)

#
# Kick out makefiles. I'm not sure but it seems that you have to add the directories to
# Makefile.am in the src subdirectories, too
#
AC_CONFIG_FILES([Makefile
  src/Makefile
  src/toolbox/blockstructured/Makefile
  src/toolbox/multiprecision/Makefile
  src/toolbox/finiteelements/Makefile
  src/toolbox/loadbalancing/Makefile
  src/toolbox/particles/Makefile
  src/convert/Makefile
  src/exahype2/Makefile
  src/petsc/Makefile
  src/swift2/Makefile
  src/unittests/Makefile
])
AC_OUTPUT
