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
  dim            = makefile.d["DIM"]
  mode           = makefile.d["LIBRARY_POSTFIX"]

  makefile.add_gpu_object( "$(PEANO_SOURCE_PATH)/exahype2/fv/libExaHyPE2Core{}d{}_a-Generic.o".format( dim, mode) )
  makefile.add_gpu_object( "$(PEANO_SOURCE_PATH)/exahype2/fv/libExaHyPE2Core{}d{}_a-Rusanov.o".format(dim, mode) )
  makefile.add_gpu_object( "$(PEANO_SOURCE_PATH)/exahype2/libExaHyPE2Core{}d{}_a-PatchUtils.o".format(dim, mode) )
  makefile.add_gpu_object( "$(PEANO_SOURCE_PATH)/tarch/multicore/omp/libTarch{}_a-multicore.o".format(mode) )

