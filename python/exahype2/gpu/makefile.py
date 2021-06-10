# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org



def add_exahype_objects(makefile):
  """

   OpenMP offloading currently does not work with static libraries. That is,
   we have to manually add object files which contain offload code even though
   these routines are there already in linked static libraries. This is 
   realised through this routine.

   A lot of these additional object files are parameterised over mode and
   dimensions, which all is information that is already available from the 
   project.

  """
  mode = makefile.d["MODE"].lower() # BUG this thing is always debug but I cannot find where this is set!

  makefile.add_gpu_object( "{PEANODIR}/src/exahype2/fv/libExaHyPE2Core{DIM}d_{}_a-Generic.o".format(mode, **makefile.d))
  makefile.add_gpu_object( "{PEANODIR}/src/exahype2/fv/libExaHyPE2Core{DIM}d_{}_a-Rusanov.o".format(mode, **makefile.d))
  makefile.add_gpu_object( "{PEANODIR}/src/exahype2/libExaHyPE2Core{DIM}d_{}_a-PatchUtils.o".format(mode, **makefile.d))
  makefile.add_gpu_object( "{PEANODIR}/src/tarch/multicore/omp/libTarch_{}_a-multicore.o".format(mode, **makefile.d) )
  #makefile.add_gpu_object( "$(top_builddir)/src/exahype2/fv/libExaHyPE2Core{}d{}_a-Rusanov.o".format(dim, makefile.d["MODE"].lower()) )
  #makefile.add_gpu_object( "$(top_builddir)/src/exahype2/libExaHyPE2Core{}d{}_a-PatchUtils.o".format(dim, makefile.d["MODE"].lower()) )
  #makefile.add_gpu_object( "$(top_builddir)/src/tarch/multicore/omp/libTarch{}_a-multicore.o".format(makefile.d["MODE"].lower()) )
  #makefile.add_gpu_object( "$(top_builddir)/src/exahype2/fv/libExaHyPE2Core{}d{}_a-Generic.o".format(dim, makefile.d["MODE"].lower()))
  #makefile.add_gpu_object( "$(top_builddir)/src/exahype2/fv/libExaHyPE2Core{}d{}_a-Rusanov.o".format(dim, makefile.d["MODE"].lower()) )
  #makefile.add_gpu_object( "$(top_builddir)/src/exahype2/libExaHyPE2Core{}d{}_a-PatchUtils.o".format(dim, makefile.d["MODE"].lower()) )
  #makefile.add_gpu_object( "$(top_builddir)/src/tarch/multicore/omp/libTarch{}_a-multicore.o".format(makefile.d["MODE"].lower()) )

