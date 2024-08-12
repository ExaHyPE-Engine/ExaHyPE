bin_PROGRAMS = Interpolation3d Restriction3d
RestrictionSources = restrictionMain.cpp ../../../src/toolbox/blockstructured/Restriction.cpp ../../../src/toolbox/blockstructured/Enumeration.cpp
InterpolationSources = interpolationMain.cpp ../../../src/toolbox/blockstructured/Interpolation.cpp ../../../src/toolbox/blockstructured/Enumeration.cpp

Restriction3d_CXXFLAGS = -I$(top_builddir)/src $(CXX_FLAGS_PEANO_3D_RELEASE) -DDimensions=3 -DPeanoDebug=1
Restriction3d_LDADD = -lPeano4Core3d -lTarch
Restriction3d_LDFLAGS = -L$(top_builddir)/src
Restriction3d_SOURCES = $(RestrictionSources)

Interpolation3d_CXXFLAGS = -I$(top_builddir)/src $(CXX_FLAGS_PEANO_3D_RELEASE) -DDimensions=3 -DPeanoDebug=1
Interpolation3d_LDADD = -lPeano4Core3d -lTarch
Interpolation3d_LDFLAGS = -L$(top_builddir)/src
Interpolation3d_SOURCES = $(InterpolationSources)

