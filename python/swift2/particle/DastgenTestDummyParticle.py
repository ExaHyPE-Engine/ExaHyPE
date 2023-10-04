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

        self.data.add_attribute(
            dastgen2.attributes.Integer(
                "myInteger0", qualifier=dastgen2.attributes.Attribute.Qualifier.NONE
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Integer(
                "myInteger1",
                initval=11,
                qualifier=dastgen2.attributes.Attribute.Qualifier.NONE,
            )
        )
        # Not implemented yet
        #  self.data.add_attribute(dastgen2.attributes.Integer("myConstInteger", initval=12, qualifier=dastgen2.attributes.Attribute.Qualifier.CONST))
        self.data.add_attribute(
            dastgen2.attributes.Integer(
                "myConstStaticInteger",
                initval=13,
                qualifier=dastgen2.attributes.Attribute.Qualifier.CONST_STATIC,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Integer(
                "myConstExprInteger",
                initval=14,
                qualifier=dastgen2.attributes.Attribute.Qualifier.CONSTEXPR,
            )
        )

        self.data.add_attribute(
            dastgen2.attributes.Boolean(
                "myBoolean0", qualifier=dastgen2.attributes.Attribute.Qualifier.NONE
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Boolean(
                "myBoolean1",
                initval="true",
                qualifier=dastgen2.attributes.Attribute.Qualifier.NONE,
            )
        )
        # Not implemented yet
        #  self.data.add_attribute(dastgen2.attributes.Boolean("myConstBoolean", initval=12, qualifier=dastgen2.attributes.Attribute.Qualifier.CONST))
        self.data.add_attribute(
            dastgen2.attributes.Boolean(
                "myConstStaticBoolean",
                initval="false",
                qualifier=dastgen2.attributes.Attribute.Qualifier.CONST_STATIC,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Boolean(
                "myConstExprBoolean",
                initval="true",
                qualifier=dastgen2.attributes.Attribute.Qualifier.CONSTEXPR,
            )
        )

        #  self.data.add_attribute(dastgen2.attributes.String("myString0", qualifier=dastgen2.attributes.Attribute.Qualifier.NONE))
        #  self.data.add_attribute(dastgen2.attributes.Integer("myInteger1", initval=11, qualifier=dastgen2.attributes.Attribute.Qualifier.NONE))
        # Not implemented yet
        #  self.data.add_attribute(dastgen2.attributes.Integer("myConstInteger", initval=12, qualifier=dastgen2.attributes.Attribute.Qualifier.CONST))
        #  self.data.add_attribute(dastgen2.attributes.Integer("myConstStaticInteger", initval=13, qualifier=dastgen2.attributes.Attribute.Qualifier.CONST_STATIC))

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
