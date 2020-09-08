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
AC_INIT(Peano, 4.0)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE(foreign -Wall -Werror)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_HEADERS([src/config.h])

AC_CANONICAL_BUILD

AC_PROG_INSTALL
AC_PROG_MAKE_SET

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
# Ensure 
#


#
# OpenMP Options
# We need those separately from the Peano build as we use OpenMP for 
# postprocessing scripts, e.g.
#
AC_LANG(C++)
AC_OPENMP()


#
# TBB Options
#
# The result is stored in a variable multithreading
#
AC_ARG_WITH(
  multithreading, 
  AC_HELP_STRING([--with-multithreading], [switch on multithreading [default=no,cpp,tbb,omp]]),
  [
    AC_MSG_CHECKING(for multithreading)
    case "${withval}" in
      no)     AC_MSG_RESULT([no multithreading]);                    multithreading=false ;;
      cpp)    AC_MSG_RESULT([build with C++11 threading support]);                          multithreading=cpp ;;
      tbb)    AC_MSG_RESULT([build with TBB (Intel Threading Building Blocks) support - please ensure that CXXFLAGS and LDFLAGS are properly set]); multithreading=tbb ;;
      omp)    AC_MSG_RESULT([build with OpenMP]); multithreading=omp ;;
      *)      AC_MSG_FAILURE([default=no (no multithreading). Use cpp or tbb to define target]);            multithreading=false ;;
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
  AC_CHECK_HEADERS([tbb/tbb.h],[],[AC_MSG_ERROR([header <tbb.h> not found])])
  AC_DEFINE([SharedTBB], [1], [SharedTBB])
fi

if test "$multithreading" == omp; then
  AC_OPENMP()
  AC_CHECK_HEADERS([omp.h],[],[AC_MSG_ERROR([header <omp.h> not found])])
  AC_DEFINE([SharedOMP], [1], [SharedOMP])

  CXXFLAGS+=" $OPENMP_CXXFLAGS "
  LDFLAGS+=" $OPENMP_CXXFLAGS "
fi

#
# MPI  Options
#
AC_ARG_WITH(mpi,
  AC_HELP_STRING([--with-mpi], [switch on distributed memory parallelisation [default=no, mpiCC command]]),
  [ AC_MSG_CHECKING(for mpi)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);                              mpi=false ;;
      *)     AC_MSG_RESULT(use mpi c++ command ${withval});  mpi=${withval}  ;;
    esac ],
  [ AC_MSG_CHECKING(for mpi); AC_MSG_RESULT(no); mpi=false ])
  

if test "$mpi" != false; then
  CXX=$mpi
  AC_DEFINE([Parallel], [1], [Parallel])
fi

##
## VTK Options
##
AC_ARG_WITH(vtk,
  AC_HELP_STRING([--with-vtk], [enable VTK and thus build Peano's conversion tools [default=no, prefix]. prefix is where VTK include files are to be found, such as /usr/local/include/vtk-8.90]),
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
    AC_HELP_STRING([--with-vtk-version], [specify vtk version (major) [default=autodetect]]),
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
    AC_HELP_STRING([--with-vtk-suffix], [specify which suffix the vtk libraries carry [default=autodetect, use for no prefix]]),
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

  AC_CHECK_LIB(vtkIOCore$vtk_suffix,main)
  VTK_LIBRARY_SUFFIX=$vtk_suffix
  AC_SUBST([VTK_LIBRARY_SUFFIX])
fi


AM_CONDITIONAL([VTK_LIBRARY_VERSION_7],[test "$vtk_version" = "7"])
AM_CONDITIONAL([VTK_LIBRARY_VERSION_8],[test "$vtk_version" = "8"])
AM_CONDITIONAL([VTK_LIBRARY_VERSION_9],[test "$vtk_version" = "9"])


#
# HDF5
#
AC_ARG_WITH(hdf5,
  AC_HELP_STRING([--with-hdf5], [enable HDF5 support [default=no]]),
  [ AC_MSG_CHECKING(for hdf5)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);                                hdf5=false ;;
      *)     AC_MSG_RESULT(build with HDF5 support - please ensure that CXXFLAGS and LDFLAGS are properly set);  hdf5=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for hdf5); AC_MSG_RESULT(no); hdf5=false ])
 
if test "$with_hdf5" = "yes"; then
  if test "$mpi" != false; then
    AX_LIB_HDF5([parallel])
  else
    AX_LIB_HDF5([serial])
  fi
  if test "$with_hdf5" = "yes"; then
    AC_DEFINE([UseHDF5], [1], [UseHDF5])
  else
    AC_MSG_ERROR([Unable to find HDF5, we need HDF5.])
  fi
fi 



#
# GPU support
#
AC_ARG_ENABLE(gpu,
  AC_HELP_STRING([--enable-gpu], [build with the GPU support  [default=no,amd,nvidia]]),
  [ AC_MSG_CHECKING(for gpu)
    case "${enableval}" in
      no)        AC_MSG_RESULT(no);                                   gpu=false ;;
      nvidia)    AC_MSG_RESULT(build with NVIDIA GPU support - please ensure that CXXFLAGS and LDFLAGS are properly set and that you configure with OpenMP multithreading);  gpu=nvidia ;;
      amd)       AC_MSG_RESULT(build with AMD GPU support - please ensure that CXXFLAGS and LDFLAGS are properly set and that you configure with OpenMP multithreading);     gpu=amd ;;
      *)         AC_MSG_FAILURE([default=no (no gpu)]); gpu=false ;;
    esac 
  ],
  [ AC_MSG_CHECKING(for gpu); AC_MSG_RESULT(no); gpu=false ]
)

if test "$gpu" == amd; then
  #AC_DEFINE([UseAMD], [1], [UseAMD])
  #AC_CHECK_HEADERS([hip/hip_runtime.h],[],[AC_MSG_ERROR([header <hip/hip_runtime.h> not found])])
  if test "$multithreading" == omp; then
    AC_DEFINE([GPUOffloading], [1], [GPUOffloading])
  else
    AC_MSG_FAILURE([AMD GPU support requires OpenMP; reconfigure with omp multithreading]);
  fi
fi

if test "$gpu" == nvidia; then
  if test "$multithreading" == omp; then
    AC_DEFINE([GPUOffloading], [1], [GPUOffloading])
  else
    AC_MSG_FAILURE([NVIDIA GPU support requires OpenMP; reconfigure with omp multithreading]);
  fi
fi






#
# Vendor-specific toolchain (logging devices, memory allocation libraries, ...)
#
AC_ARG_WITH(toolchain,
  AC_HELP_STRING([--with-toolchain], [build with vendor-specific toolchain [default=no,intel,nvidia,chrome]]),
  [ AC_MSG_CHECKING(for toolchain)
    case "${withval}" in
      no)        AC_MSG_RESULT(no);                                toolchain=false ;;
      intel)     AC_MSG_RESULT(build with support for the Intel SDK - please ensure that CXXFLAGS and LDFLAGS are properly set);   toolchain=intel  ;;
      nvidia)    AC_MSG_RESULT(build with support for the NVIDIA SDK - please ensure that CXXFLAGS and LDFLAGS are properly set);  toolchain=nvidia ;;
      chrome)    AC_MSG_RESULT(build with logging support for Google Chrome); toolchain=chrome ;;
      *)         AC_MSG_FAILURE([default=no (no bespoke toolchain)]); toolchain=false ;;
    esac 
  ],
  [ 
    AC_MSG_CHECKING(for toolchain);
    AC_MSG_RESULT(no); toolchain=false
  ]
)

if test "$toolchain" == intel; then
  AC_DEFINE([UseIntel], [1], [UseIntel])
  AC_DEFINE([UseLogService], [ITACLogger], [ITACLogger])
  AC_CHECK_HEADERS([VT.h],[],[AC_MSG_ERROR([header <VT.h> not found])])
fi

if test "$toolchain" == nvidia; then
  AC_DEFINE([UseNVIDIA], [1], [UseNVIDIA])
  AC_DEFINE([UseLogService], [NVTXLogger], [NVTXLogger])
  AC_CHECK_HEADERS([nvToolsExt.h],[],[AC_MSG_ERROR([header <nvToolsExt.h> not found])])
fi

if test "$toolchain" == chrome; then
  AC_DEFINE([UseLogService], [ChromeTraceFileLogger], [ChromeTraceFileLogger])
fi




#
# ExaHypE2
#
AC_ARG_ENABLE(exahype,
  AC_HELP_STRING([--enable-exahype], [enable ExaHyPE2 [default=no]]),
  [ AC_MSG_CHECKING(for exahype)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    exahype=false ;;
      *)     AC_MSG_RESULT(yes);   exahype=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for exahype); AC_MSG_RESULT(no); exahype=false ])
AM_CONDITIONAL([UseExaHyPE],[test "$exahype" != false])
if test "$exahype" != false; then
  AC_DEFINE([UseExaHyPE], [1], [UseExaHyPE])
fi





#
# Delta
#
AC_ARG_WITH(delta,
  AC_HELP_STRING([--with-delta], [enable Delta [default=no]]),
  [ AC_MSG_CHECKING(for delta)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);                                 delta=false ;;
      *)     AC_MSG_RESULT(build against Delta installation - please ensure that CXXFLAGS and LDFLAGS are properly set);  delta=${withval}  ;;
    esac ],
  [ AC_MSG_CHECKING(for delta); AC_MSG_RESULT(no); delta=false ])
# This whole autotools thing is not very nice: If I wanna use an if ... endif (
# note the syntax which is different to the present file, then I have to declare
# the variable as I do in the line below. Otherwise, the if thing is not 
# "exported". Seems the autoconf and the makefile generator are completely 
# separate with inconsistent syntax.
AM_CONDITIONAL([UseDelta],[test "$delta" != false])
if test "$delta" != false; then
  AC_DEFINE([UseDelta], [1], [UseDelta])
fi



#
# Kick out makefiles. I'm not sure but it seems that you have to add the directories to 
# Makefile.am in the src subdirectories, too
#
AC_OUTPUT(
  Makefile 
  src/Makefile 
  src/toolbox/multiprecision/Makefile src/toolbox/finiteelements/Makefile src/toolbox/loadbalancing/Makefile
  src/unittests/Makefile 
  examples/grid/Makefile examples/integerdiffusionthroughfaces/Makefile examples/delta/Makefile examples/regulargridupscaling/Makefile 
  src/convert/Makefile
  src/exahype2/Makefile
)

