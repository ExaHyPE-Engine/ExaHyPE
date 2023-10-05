# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.Particle import Particle
from swift2.particle.AlgorithmStep import AlgorithmStep

import peano4
import dastgen2
import peano4.dastgen2

from abc import ABC


class DastgenTestDummyParticle(Particle):
    """!

    A dummy particle containing meaningless variables to test the functionality
    of dastgen2.

    All this particle does is define variables, and then test getters, setters,
    and constructors in a single sweep.

    """

    def __init__(
        self,
        name,
        particles_per_cell=0,  # makes no sense, and should likely be forbidden
        min_h=0.3,
        max_h=0.3,
    ):
        super(DastgenTestDummyParticle, self).__init__(
            name,
            particles_per_cell,
            min_h,
            max_h,
        )

        self.generate_integer_attributes(compress=False, ifdefs=False)
        self.generate_integer_attributes(compress=False, ifdefs=True)
        self.generate_integer_attributes(compress=True, ifdefs=False)
        self.generate_integer_attributes(compress=True, ifdefs=True)

        return

    def generate_integer_attributes(self, compress: bool, ifdefs: bool):
        """!
        Generate integer attributes.

        compress: Boolean
            if True, enable compression.

        ifdefs: Boolean
            if True, add debug ifdefs to the attributes.
        """

        # Set the variable basename, so we get different
        # variable names for each possible case.
        var_basename = "Integer"
        if compress and not ifdefs:
            var_basename = "CompressedInteger"
        if ifdefs and not compress:
            var_basename = "DebugInteger"
        if ifdefs and compress:
            var_basename = "DebugCompressedInteger"

        # Set compression values
        min_value = None
        max_value = None
        if compress:
            min_value = 0
            max_value = 2097152

        # Set ifdefs
        defs = []
        if ifdefs:
            defs = ["PeanoDebug > 0"]

        # NOTE: initial values, where provided, are hardcoded to be checked
        # for equality in the test. If you modify them, the test will fail.
        self.data.add_attribute(
            dastgen2.attributes.Integer(
                "my" + var_basename,
                qualifier=dastgen2.attributes.Attribute.Qualifier.NONE,
                min_value=min_value,
                max_value=max_value,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Integer(
                "myInitval" + var_basename,
                initval=123,
                qualifier=dastgen2.attributes.Attribute.Qualifier.NONE,
                min_value=min_value,
                max_value=max_value,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Integer(
                "myStatic" + var_basename,
                initval=1234,
                qualifier=dastgen2.attributes.Attribute.Qualifier.STATIC,
                min_value=min_value,
                max_value=max_value,
                ifdefs=defs,
            )
        )
        # Not implemented yet
        self.data.add_attribute(
            dastgen2.attributes.Integer(
                "myConst" + var_basename,
                initval=12345,
                qualifier=dastgen2.attributes.Attribute.Qualifier.CONST,
                min_value=min_value,
                max_value=max_value,
                ifdefs=defs,
            )
        )

        self.data.add_attribute(
            dastgen2.attributes.Integer(
                "myConstStatic" + var_basename,
                initval=123456,
                qualifier=dastgen2.attributes.Attribute.Qualifier.CONST_STATIC,
                min_value=min_value,
                max_value=max_value,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Integer(
                "myConstExpr" + var_basename,
                initval=1234567,
                qualifier=dastgen2.attributes.Attribute.Qualifier.CONSTEXPR,
                min_value=min_value,
                max_value=max_value,
                ifdefs=defs,
            )
        )

        return

    def algorithm_steps(self):
        """!

        Return algorithm steps: A list of AlgorithmStep objects to be executed in
        that order.

        """

        step = AlgorithmStep(
            name="dummyDastgenTest",
            dependencies=AlgorithmStep.Dependencies.SELF,
            effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
            touch_vertex_first_time_kernel="""::dastgenTest::checkDastgen(localParticles);""",
            prepare_traversal_kernel="""
                ::dastgenTest::dummyMoveForwardInTime<globaldata::{}>();""".format(
                self.name
            ),
            includes="""
                           #include "swift2/dastgenTest.h"
                           """,
        )

        # We need 2 algorithm steps because the main.cpp generation
        # assumes there will be more than 1 algorithm step. That's a
        # fair assumption, and not a good enough reason to go exception
        # handle that just for this test.
        step2 = AlgorithmStep(
            name="dummyDastgenTest2",
            dependencies=AlgorithmStep.Dependencies.SELF,
            effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
            touch_vertex_last_time_kernel="""::dastgenTest::checkDastgen(localParticles);""",
            unprepare_traversal_kernel="""
                                       globaldata::{PARTICLE}::getSpecies().setTimeStamp( globaldata::{PARTICLE}::getSpecies().getMinTimeStamp() + globaldata::{PARTICLE}::getSpecies().getMinTimeStepSize(), false );
                                       ::dastgenTest::reportStep<globaldata::{PARTICLE}>( "{PARTICLE}" );
                                       """.replace(
                "{PARTICLE}", self.name
            ),
            includes="""
                           #include "swift2/dastgenTest.h"
                           """,
        )

        return [step, step2]

    @property
    def mantissa_size(self):
        """

        Set the mantissa size of doubles and Peano double arrays if
        we want to use reduced precission via Clang annotations. As a reference,
        floats have mantissa size = 23.

        """
        return self._mantissa_size

    @mantissa_size.setter
    def mantissa_size(self, mantissa_size):
        if mantissa_size < 0:
            raise ValueError("Mantissa size has to be larger than 0.")
        self._mantissa_size = mantissa_size

    def get_cpp_namespace_from_project_namespace(self):
        """

        Transform namespace into cpp format. Could be used to append namespace to
        constants in kernels (not used currently).

        """
        namespace = "::".join(self._swift_project_namespace) + "::"

        print(namespace)

    def initialisation_steps(self):
        return []

    @property
    def readme_descriptor(self):
        return (
            super(DastgenTestDummyParticle, self).readme_descriptor
            + """

  Dummy particle containing meaningless variables to test the functionality
  of dastgen2.
    """
        )
