AUTOMAKE_OPTIONS = subdir-objects

if UseExaHyPE
  lib_LIBRARIES = libExaHyPE2Core2d.a  libExaHyPE2Core2d_debug.a  libExaHyPE2Core2d_asserts.a  libExaHyPE2Core2d_trace.a \
                  libExaHyPE2Core3d.a  libExaHyPE2Core3d_debug.a  libExaHyPE2Core3d_asserts.a  libExaHyPE2Core3d_trace.a
endif



ExaHyPE2Sources = \
  EnclaveBookkeeping.cpp \
  EnclaveTask.cpp \
  NonCriticalAssertions.cpp \
  PatchUtils.cpp \
  PlotFilter.cpp \
  RefinementControl.cpp \
  SmartEnclaveTask.cpp \
  UserInterface.cpp \
  UnitTests.cpp \
  aderdg/CorrectorAoS.cpp \
  aderdg/Generic.cpp \
  aderdg/KernelUtils.cpp \
  aderdg/PredictorAoS.cpp \
  aderdg/RiemannAoS.cpp \
  aderdg/RusanovNonlinearAoS.cpp \
  aderdg/tests/ADERDGTest.cpp \
  fv/BoundaryConditions.cpp \
  fv/Generic.cpp \
  fv/Rusanov.cpp \
  fv/tests/CopyPatchTest.cpp \
  fv/tests/applySplit1DRiemannToPatch_Overlap1AoSTest.cpp


 
libExaHyPE2Core2d_a_CXXFLAGS          = -DDimensions=2 -DPeanoDebug=0  
libExaHyPE2Core2d_trace_a_CXXFLAGS    = -DDimensions=2 -DPeanoDebug=1  
libExaHyPE2Core2d_asserts_a_CXXFLAGS  = -DDimensions=2 -DPeanoDebug=2  
libExaHyPE2Core2d_debug_a_CXXFLAGS    = -DDimensions=2 -DPeanoDebug=4  

 
libExaHyPE2Core3d_a_CXXFLAGS          = -DDimensions=3 -DPeanoDebug=0  
libExaHyPE2Core3d_trace_a_CXXFLAGS    = -DDimensions=3 -DPeanoDebug=1  
libExaHyPE2Core3d_asserts_a_CXXFLAGS  = -DDimensions=3 -DPeanoDebug=2  
libExaHyPE2Core3d_debug_a_CXXFLAGS    = -DDimensions=3 -DPeanoDebug=4  



libExaHyPE2Core2d_a_SOURCES         = $(ExaHyPE2Sources)
libExaHyPE2Core2d_trace_a_SOURCES   = $(ExaHyPE2Sources)
libExaHyPE2Core2d_asserts_a_SOURCES = $(ExaHyPE2Sources)
libExaHyPE2Core2d_debug_a_SOURCES   = $(ExaHyPE2Sources)


libExaHyPE2Core3d_a_SOURCES         = $(ExaHyPE2Sources)
libExaHyPE2Core3d_trace_a_SOURCES   = $(ExaHyPE2Sources)
libExaHyPE2Core3d_asserts_a_SOURCES = $(ExaHyPE2Sources)
libExaHyPE2Core3d_debug_a_SOURCES   = $(ExaHyPE2Sources)

