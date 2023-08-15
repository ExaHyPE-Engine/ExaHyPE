SUBDIRS = src
TESTS = src/unittests/UnitTests2d src/unittests/UnitTests3d \
				src/unittests/UnitTests2d_debug src/unittests/UnitTests3d_debug \
				src/unittests/UnitTests2d_stats src/unittests/UnitTests3d_stats \
				src/unittests/UnitTests2d_asserts src/unittests/UnitTests3d_asserts \
				src/unittests/UnitTests2d_trace src/unittests/UnitTests3d_trace

doc: docs

docs:
	doxygen documentation/Doxyfile

ACLOCAL_AMFLAGS = -I m4
