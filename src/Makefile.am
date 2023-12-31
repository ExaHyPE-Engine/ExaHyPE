# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

AUTOMAKE_OPTIONS = subdir-objects

#
# By default, Automake does dfs. We need the libs to be built first however.
# So we add the . explicitly. Please note that it is not enough to add your
# directory here. You also have to add any new directory to the root's
# configure.ac script and rerun the whole Autotool pipeline (see comment
# within configure.ac as well as its very last line).
#
SUBDIRS = \
	tarch \
	peano4 \
	toolbox/blockstructured \
	toolbox/finiteelements \
	toolbox/loadbalancing \
	toolbox/multiprecision \
	toolbox/particles \
	exahype2 \
	swift2 \
	petsc \
	convert \
	unittests
