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
# OpenMP Options
# We need those separately from the Peano build as we use OpenMP for 
# postprocessing scripts, e.g.
#
AC_LANG(C++)
AC_OPENMP()


CXXFLAGS_PEANO_2D_RELEASE=" -DDimensions=2 -DPeanoDebug=0"
CXXFLAGS_PEANO_2D_STATS="   -DDimensions=2 -DPeanoDebug=0 -DTrackStatistics"
CXXFLAGS_PEANO_2D_ASSERTS=" -DDimensions=2 -DPeanoDebug=2 -g3 -O0"
CXXFLAGS_PEANO_2D_TRACE="   -DDimensions=2 -DPeanoDebug=1 -g3"
CXXFLAGS_PEANO_2D_DEBUG="   -DDimensions=2 -DPeanoDebug=4 -g3 -O0"

CXXFLAGS_PEANO_3D_RELEASE=" -DDimensions=3 -DPeanoDebug=0"
CXXFLAGS_PEANO_3D_STATS="   -DDimensions=3 -DPeanoDebug=0 -DTrackStatistics"
CXXFLAGS_PEANO_3D_ASSERTS=" -DDimensions=3 -DPeanoDebug=2 -g3 -O0"
CXXFLAGS_PEANO_3D_TRACE="   -DDimensions=3 -DPeanoDebug=1 -g3"
CXXFLAGS_PEANO_3D_DEBUG="   -DDimensions=3 -DPeanoDebug=4 -g3 -O0"


LDFLAGS_PEANO_RELEASE=" "
LDFLAGS_PEANO_STATS="   "
LDFLAGS_PEANO_ASSERTS=" -g3"
LDFLAGS_PEANO_TRACE="   -g3"
LDFLAGS_PEANO_DEBUG="   -g3"


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
# TBB Options
#
# The result is stored in a variable multithreading
#
AC_ARG_WITH(
  multithreading, 
  AC_HELP_STRING([--with-multithreading], [switch on multithreading [default=no,cpp,tbb,omp,omp-layered]]),
  [
    AC_MSG_CHECKING(for multithreading)
    case "${withval}" in
      no)     AC_MSG_RESULT([no multithreading]);                    multithreading=false ;;
      cpp)    AC_MSG_RESULT([build with C++11 threading support]);                          multithreading=cpp ;;
      tbb)    AC_MSG_RESULT([build with TBB (Intel Threading Building Blocks) support]); multithreading=tbb ;;
      omp)    AC_MSG_RESULT([build with OpenMP]); multithreading=omp ;;
      *)      AC_MSG_FAILURE([default=no (no multithreading). Use omp, cpp or tbb to define target]);            multithreading=false ;;
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
  AC_CHECK_HEADERS([omp.h],[],[AC_MSG_WARN([header <omp.h> not found])])
  AC_DEFINE([SharedOMP], [1], [SharedOMP])
fi


#
# MPI  Options
#
# Logic: user provides mpi compiler command
#        check if that command exists
#        overwrite CXX to be the user supplied value
#
AC_ARG_WITH(mpi,
  AC_HELP_STRING([--with-mpi], [switch on distributed memory parallelisation [default=no, mpiCC command]]),
  [ AC_MSG_CHECKING(for mpi)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);                              mpi=false ;;
      *)     AC_MSG_RESULT(use mpi c++ command ${withval});  mpi=${withval}  ;;
    esac ],
  [ AC_MSG_CHECKING(for mpi); AC_MSG_RESULT(no); mpi=false ])
 
AC_DEFUN([AC_PROG_USERMPI], [AC_CHECK_PROG(USERMPI,"${mpi}",yes)])


if test "$mpi" != false; then
  AC_PROG_USERMPI
  if test x"${USERMPI}" != x"yes" ; then
      AC_MSG_ERROR([The specified MPI compiler (${mpi}) was not found.])
  fi
  CXX=$mpi
  AC_DEFINE([Parallel], [1], [Parallel])
else
  MPICXX=false
fi


#
# Compiler options. Allow icpc and gnu. Plus minimum version check.
#

AX_COMPILER_VENDOR

AX_CXX_COMPILE_STDCXX_11


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
  #if test "$multithreading" == omp; then
  if test "$multithreading" == omp-layered; then
    AC_DEFINE([GPUOffloading], [1], [GPUOffloading])
  elif test "$multithreading" == omp; then
    AC_DEFINE([GPUOffloading], [1], [GPUOffloading])
  else
    AC_MSG_FAILURE([NVIDIA GPU support requires OpenMP; reconfigure with omp multithreading]);
  fi
fi
AM_CONDITIONAL([OpenMPGPUOffloading],[test "x$gpu" = xnvidia ])






#
# Vendor-specific toolchain (logging devices, memory allocation libraries, ...)
#
AC_ARG_WITH(toolchain,
  AC_HELP_STRING([--with-toolchain], [build with vendor-specific toolchain [default=no,intel,nvidia,scorep,chrome,gnu]]),
  [ AC_MSG_CHECKING(for toolchain)
    case "${withval}" in
      no)        AC_MSG_RESULT(no);                                toolchain=false ;;
      intel)     AC_MSG_RESULT(build with support for the Intel SDK - please ensure that CXXFLAGS and LDFLAGS are properly set);   toolchain=intel  ;;
      nvidia)    AC_MSG_RESULT(build with support for the NVIDIA SDK - please ensure that CXXFLAGS and LDFLAGS are properly set);  toolchain=nvidia ;;
      scorep)    AC_MSG_RESULT(build with support for the Score-P - please ensure that CXXFLAGS and LDFLAGS are properly set);  toolchain=scorep ;;
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

if test "$toolchain" == intel; then
  AC_DEFINE([UseIntel], [1], [UseIntel])
  AC_DEFINE([UseLogService], [ITACLogger], [ITACLogger])
  AC_CHECK_HEADERS([VT.h],[],[AC_MSG_ERROR([header <VT.h> not found])])
  LDFLAGS+=-ipo
  CXXFLAGS+=" -ip -ipo "
fi



if test "$toolchain" == scorep; then
  AC_DEFINE([UseLogService], [ScorePLogger], [ScorePLogger])
  AC_DEFINE([UseScoreP],     [UseScoreP], [UseScoreP])
  #LDFLAGS+=-ipo
  #CXXFLAGS+=" -ip -ipo "
fi

if test "$toolchain" == nvidia; then
  AC_DEFINE([UseNVIDIA], [1], [UseNVIDIA])
  AC_DEFINE([UseLogService], [NVTXLogger], [NVTXLogger])
  AC_CHECK_HEADERS([nvToolsExt.h],[],[AC_MSG_ERROR([header <nvToolsExt.h> not found])])
fi

if test "$toolchain" == chrome; then
  AC_DEFINE([UseLogService], [ChromeTraceFileLogger], [ChromeTraceFileLogger])
fi

if test "$toolchain" == gnu; then
  LDFLAGS+=-flto
fi





#
# FEM 
#
AC_ARG_ENABLE(fem,
  AC_HELP_STRING([--enable-fem], [enable FEM  [default=false]]),
  [ AC_MSG_CHECKING(for fem)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    fem=false ;;
      *)     AC_MSG_RESULT(yes);   fem=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for fem); AC_MSG_RESULT(no); fem=false ])
AM_CONDITIONAL([UseToolboxFiniteElements],[test "x$fem" = xtrue])
if test "$fem" != false; then
  LDADD_PEANO_2D_RELEASE+=" -lToolboxFiniteElements2d "
  LDADD_PEANO_2D_STATS+="   -lToolboxFiniteElements2d "
  LDADD_PEANO_2D_ASSERTS+=" -lToolboxFiniteElements2d " 
  LDADD_PEANO_2D_TRACE+="   -lToolboxFiniteElements2d "
  LDADD_PEANO_2D_DEBUG+="   -lToolboxFiniteElements2d "

  LDADD_PEANO_3D_RELEASE+=" -lToolboxFiniteElements3d "
  LDADD_PEANO_3D_STATS+="   -lToolboxFiniteElements3d "
  LDADD_PEANO_3D_ASSERTS+=" -lToolboxFiniteElements3d " 
  LDADD_PEANO_3D_TRACE+="   -lToolboxFiniteElements3d "
  LDADD_PEANO_3D_DEBUG+="   -lToolboxFiniteElements3d "

  LDFLAGS_PEANO_RELEASE+=" -L$(top_builddir)/src/toolbox/finiteelements"
  LDFLAGS_PEANO_STATS+="   -L$(top_builddir)/src/toolbox/finiteelements"
  LDFLAGS_PEANO_ASSERTS+=" -L$(top_builddir)/src/toolbox/finiteelements"
  LDFLAGS_PEANO_TRACE+="   -L$(top_builddir)/src/toolbox/finiteelements"
  LDFLAGS_PEANO_DEBUG+="   -L$(top_builddir)/src/toolbox/finiteelements"
fi



#
# Multiprecision 
#
AC_ARG_ENABLE(multiprecision,
  AC_HELP_STRING([--enable-multiprecision], [enable multiprecision  [default=false]]),
  [ AC_MSG_CHECKING(for multiprecision)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    multiprecision=false ;;
      *)     AC_MSG_RESULT(yes);   multiprecision=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for multiprecision); AC_MSG_RESULT(no); multiprecision=false ])
AM_CONDITIONAL([UseToolboxMultiprecision],[test "x$multiprecision" = xtrue])
if test "$multiprecision" != false; then
  LDADD_PEANO_2D_RELEASE+=" -lToolboxMultiprecision "
  LDADD_PEANO_2D_STATS+="   -lToolboxMultiprecision_stats "
  LDADD_PEANO_2D_ASSERTS+=" -lToolboxMultiprecision_asserts " 
  LDADD_PEANO_2D_TRACE+="   -lToolboxMultiprecision_trace "
  LDADD_PEANO_2D_DEBUG+="   -lToolboxMultiprecision_debug "

  LDADD_PEANO_3D_RELEASE+=" -lToolboxMultiprecision "
  LDADD_PEANO_3D_STATS+="   -lToolboxMultiprecision_stats "
  LDADD_PEANO_3D_ASSERTS+=" -lToolboxMultiprecision_asserts " 
  LDADD_PEANO_3D_TRACE+="   -lToolboxMultiprecision_trace "
  LDADD_PEANO_3D_DEBUG+="   -lToolboxMultiprecision_debug "

  LDFLAGS_PEANO_RELEASE+=" -L$(top_builddir)/src/toolbox/multiprecision"
  LDFLAGS_PEANO_STATS+="   -L$(top_builddir)/src/toolbox/multiprecision"
  LDFLAGS_PEANO_ASSERTS+=" -L$(top_builddir)/src/toolbox/multiprecision"
  LDFLAGS_PEANO_TRACE+="   -L$(top_builddir)/src/toolbox/multiprecision"
  LDFLAGS_PEANO_DEBUG+="   -L$(top_builddir)/src/toolbox/multiprecision"
fi


#
# Load balancing 
#
AC_ARG_ENABLE(loadbalancing,
  AC_HELP_STRING([--enable-loadbalancing], [enable loadbalancing  [default=false]]),
  [ AC_MSG_CHECKING(for loadbalancing)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    loadbalancing=false ;;
      *)     AC_MSG_RESULT(yes);   loadbalancing=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for loadbalancing); AC_MSG_RESULT(no); loadbalancing=false ])
AM_CONDITIONAL([UseToolboxLoadBalancing],[test "x$loadbalancing" = xtrue])
if test "$loadbalancing" != false; then
  LDADD_PEANO_2D_RELEASE+=" -lToolboxLoadBalancing2d "
  LDADD_PEANO_2D_STATS+="   -lToolboxLoadBalancing2d_stats "
  LDADD_PEANO_2D_ASSERTS+=" -lToolboxLoadBalancing2d_asserts " 
  LDADD_PEANO_2D_TRACE+="   -lToolboxLoadBalancing2d_trace "
  LDADD_PEANO_2D_DEBUG+="   -lToolboxLoadBalancing2d_debug "

  LDADD_PEANO_3D_RELEASE+=" -lToolboxLoadBalancing3d "
  LDADD_PEANO_3D_STATS+="   -lToolboxLoadBalancing3d_stats "
  LDADD_PEANO_3D_ASSERTS+=" -lToolboxLoadBalancing3d_asserts " 
  LDADD_PEANO_3D_TRACE+="   -lToolboxLoadBalancing3d_trace "
  LDADD_PEANO_3D_DEBUG+="   -lToolboxLoadBalancing3d_debug "

  LDFLAGS_PEANO_RELEASE+=" -L$(top_builddir)/src/toolbox/loadbalancing"
  LDFLAGS_PEANO_STATS+="   -L$(top_builddir)/src/toolbox/loadbalancing"
  LDFLAGS_PEANO_ASSERTS+=" -L$(top_builddir)/src/toolbox/loadbalancing"
  LDFLAGS_PEANO_TRACE+="   -L$(top_builddir)/src/toolbox/loadbalancing"
  LDFLAGS_PEANO_DEBUG+="   -L$(top_builddir)/src/toolbox/loadbalancing"
fi


#
# Particles 
#
AC_ARG_ENABLE(particles,
  AC_HELP_STRING([--enable-particles], [enable particles  [default=false]]),
  [ AC_MSG_CHECKING(for particles)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    particles=false ;;
      *)     AC_MSG_RESULT(yes);   particles=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for particles); AC_MSG_RESULT(no); particles=false ])
AM_CONDITIONAL([UseToolboxParticles],[test "x$particles" = xtrue])
if test "$particles" != false; then
  LDADD_PEANO_2D_RELEASE+=" -lToolboxParticles2d "
  LDADD_PEANO_2D_STATS+="   -lToolboxParticles2d_stats "
  LDADD_PEANO_2D_ASSERTS+=" -lToolboxParticles2d_asserts " 
  LDADD_PEANO_2D_TRACE+="   -lToolboxParticles2d_trace "
  LDADD_PEANO_2D_DEBUG+="   -lToolboxParticles2d_debug "

  LDADD_PEANO_3D_RELEASE+=" -lToolboxParticles3d "
  LDADD_PEANO_3D_STATS+="   -lToolboxParticles3d_stats "
  LDADD_PEANO_3D_ASSERTS+=" -lToolboxParticles3d_asserts " 
  LDADD_PEANO_3D_TRACE+="   -lToolboxParticles3d_trace "
  LDADD_PEANO_3D_DEBUG+="   -lToolboxParticles3d_debug "

  LDFLAGS_PEANO_RELEASE+=" -L$(top_builddir)/src/toolbox/particles"
  LDFLAGS_PEANO_STATS+="   -L$(top_builddir)/src/toolbox/particles"
  LDFLAGS_PEANO_ASSERTS+=" -L$(top_builddir)/src/toolbox/particles"
  LDFLAGS_PEANO_TRACE+="   -L$(top_builddir)/src/toolbox/particles"
  LDFLAGS_PEANO_DEBUG+="   -L$(top_builddir)/src/toolbox/particles"
fi


#
# ExaHyPE2
#
AC_ARG_ENABLE(exahype,
  AC_HELP_STRING([--enable-exahype], [enable ExaHyPE2 [default=false]]),
  [ AC_MSG_CHECKING(for exahype)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    exahype=false ;;
      *)     AC_MSG_RESULT(yes);   exahype=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for exahype); AC_MSG_RESULT(no); exahype=false ])
AM_CONDITIONAL([UseExaHyPE],[test "x$exahype" = xtrue])
if test "$exahype" != false; then
  AC_DEFINE([UseExaHyPE], [1], [UseExaHyPE])
  if test "$loadbalancing" == false; then
    AC_MSG_ERROR([ExaHyPE2 requires the load balancing ])
  fi
  LDADD_PEANO_2D_RELEASE+=" -lToolboxExaHyPE2d "
  LDADD_PEANO_2D_STATS+="   -lToolboxExaHyPE2d_stats "
  LDADD_PEANO_2D_ASSERTS+=" -lToolboxExaHyPE2d_asserts " 
  LDADD_PEANO_2D_TRACE+="   -lToolboxExaHyPE2d_trace "
  LDADD_PEANO_2D_DEBUG+="   -lToolboxExaHyPE2d_debug "

  LDADD_PEANO_3D_RELEASE+=" -lToolboxExaHyPE3d "
  LDADD_PEANO_3D_STATS+="   -lToolboxExaHyPE3d_stats "
  LDADD_PEANO_3D_ASSERTS+=" -lToolboxExaHyPE3d_asserts " 
  LDADD_PEANO_3D_TRACE+="   -lToolboxExaHyPE3d_trace "
  LDADD_PEANO_3D_DEBUG+="   -lToolboxExaHyPE3d_debug "

  LDFLAGS_PEANO_RELEASE+=" -L$(top_builddir)/src/exahype"
  LDFLAGS_PEANO_STATS+="   -L$(top_builddir)/src/exahype"
  LDFLAGS_PEANO_ASSERTS+=" -L$(top_builddir)/src/exahype"
  LDFLAGS_PEANO_TRACE+="   -L$(top_builddir)/src/exahype"
  LDFLAGS_PEANO_DEBUG+="   -L$(top_builddir)/src/exahype"
fi


#
# SmartMPI
#
AC_ARG_WITH(smartmpi,
  AC_HELP_STRING([--with-smartmpi], [build with SmartMPI [default=false]]),
  [ AC_MSG_CHECKING(for smartmpi)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);    smartmpi=false ;;
      *)     AC_MSG_RESULT(yes - please ensure that CXXFLAGS and LDFLAGS are properly set);   smartmpi=true  ;;
    esac ],
  [ AC_MSG_CHECKING(for smartmpi); AC_MSG_RESULT(no); smartmpi=false ])
AM_CONDITIONAL([UseSmartMPI],[test "x$smartmpi" = xtrue])
if test "$smartmpi" != false; then
  AC_DEFINE([UseSmartMPI], [1], [UseSmartMPI])
  if test "$mpi" == false; then
    AC_MSG_ERROR([SmartMPI requires an MPI build of Peano])
  fi
  LDADD_PEANO_2D_RELEASE+=" -lsmartmpi "
  LDADD_PEANO_2D_STATS+="   -lsmartmpi "
  LDADD_PEANO_2D_ASSERTS+=" -lsmartmpi_debug " 
  LDADD_PEANO_2D_TRACE+="   -lsmartmpi "
  LDADD_PEANO_2D_DEBUG+="   -lsmartmpi_debug "

  LDADD_PEANO_3D_RELEASE+=" -lsmartmpi "
  LDADD_PEANO_3D_STATS+="   -lsmartmpi "
  LDADD_PEANO_3D_ASSERTS+=" -lsmartmpi_debug " 
  LDADD_PEANO_3D_TRACE+="   -lsmartmpi "
  LDADD_PEANO_3D_DEBUG+="   -lsmartmpi_debug "
fi


#
# Delta
#
AC_ARG_WITH(delta,
  AC_HELP_STRING([--with-delta], [enable Delta [default=no]]),
  [ AC_MSG_CHECKING(for delta)
    case "${withval}" in
      no)    AC_MSG_RESULT(no);                                 delta=false ;;
      *)     AC_MSG_RESULT(yes - please ensure that CXXFLAGS and LDFLAGS are properly set);  delta=${withval}  ;;
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
  LDADD_PEANO_2D_RELEASE+=" -ldelta2d "
  LDADD_PEANO_2D_STATS+="   -ldelta2d "
  LDADD_PEANO_2D_ASSERTS+=" -ldelta2d " 
  LDADD_PEANO_2D_TRACE+="   -ldelta2d "
  LDADD_PEANO_2D_DEBUG+="   -ldelta2d "

  LDADD_PEANO_3D_RELEASE+=" -ldelta3d "
  LDADD_PEANO_3D_STATS+="   -ldelta3d "
  LDADD_PEANO_3D_ASSERTS+=" -ldelta3d " 
  LDADD_PEANO_3D_TRACE+="   -ldelta3d "
  LDADD_PEANO_3D_DEBUG+="   -ldelta3d "
fi


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
AC_OUTPUT(
  Makefile 
  src/Makefile 
  src/toolbox/multiprecision/Makefile src/toolbox/finiteelements/Makefile src/toolbox/loadbalancing/Makefile src/toolbox/particles/Makefile
  examples/grid/Makefile examples/integerdiffusionthroughfaces/Makefile examples/delta/Makefile examples/regulargridupscaling/Makefile 
  src/convert/Makefile
  src/exahype2/Makefile
  src/unittests/Makefile
)

