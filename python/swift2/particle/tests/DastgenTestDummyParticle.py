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
    and constructors.

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

        # Generate the data attributes.
        self._generate_boolean_attributes(ifdefs=False)
        self._generate_boolean_attributes(ifdefs=True)

        self._generate_double_attributes(compress=False, ifdefs=False)
        self._generate_double_attributes(compress=False, ifdefs=True)
        self._generate_double_attributes(compress=True, ifdefs=False)
        self._generate_double_attributes(compress=True, ifdefs=True)

        self._generate_enum_attributes(ifdefs=False)
        self._generate_enum_attributes(ifdefs=True)

        self._generate_integer_attributes(compress=False, ifdefs=False)
        self._generate_integer_attributes(compress=False, ifdefs=True)
        self._generate_integer_attributes(compress=True, ifdefs=False)
        self._generate_integer_attributes(compress=True, ifdefs=True)

        self._generate_string_attributes(ifdefs=False)
        self._generate_string_attributes(ifdefs=True)

        self._generate_boolean_array_attributes(compress=False, ifdefs=False)
        self._generate_boolean_array_attributes(compress=True, ifdefs=False)
        self._generate_boolean_array_attributes(compress=False, ifdefs=True)
        self._generate_boolean_array_attributes(compress=True, ifdefs=True)

        self._generate_double_array_attributes(compress=False, ifdefs=False)
        self._generate_double_array_attributes(compress=True, ifdefs=False)
        self._generate_double_array_attributes(compress=False, ifdefs=True)
        self._generate_double_array_attributes(compress=True, ifdefs=True)

        self._generate_integer_array_attributes(compress=False, ifdefs=False)
        self._generate_integer_array_attributes(compress=True, ifdefs=False)
        self._generate_integer_array_attributes(compress=False, ifdefs=True)
        self._generate_integer_array_attributes(compress=True, ifdefs=True)

        self._generate_peano_double_array_attributes(compress=False, ifdefs=False)
        self._generate_peano_double_array_attributes(compress=True, ifdefs=False)
        self._generate_peano_double_array_attributes(compress=False, ifdefs=True)
        self._generate_peano_double_array_attributes(compress=True, ifdefs=True)

        self._generate_peano_integer_array_attributes(compress=False, ifdefs=False)
        self._generate_peano_integer_array_attributes(compress=True, ifdefs=False)
        self._generate_peano_integer_array_attributes(compress=False, ifdefs=True)
        self._generate_peano_integer_array_attributes(compress=True, ifdefs=True)

        self._generate_user_defined_attributes(ifdefs=False)
        self._generate_user_defined_attributes(ifdefs=True)

        return

    def _generate_user_defined_attributes(self, ifdefs: bool):
        if ifdefs:
            var_basename = "DebugUserDefinedAttribute"
            defs = ["PeanoDebug > 0"]
        else:
            var_basename = "UserDefinedAttribute"
            defs = []
        self.data.add_attribute(
            dastgen2.attributes.UserDefinedType(
                name="myStatic" + var_basename,
                type="std::string",
                include="""
#include <string>
""",
                ifdefs=defs,
                qualifier=dastgen2.attributes.Attribute.Qualifier.STATIC,
                user_type_has_toString_method=False,
            )
        )

    def _generate_boolean_attributes(self, ifdefs: bool):
        """!
        Generate boolean attributes.

        ifdefs: Boolean
            if True, add debug ifdefs to the attributes.
        """

        # Set the variable basename, so we get different
        # variable names for each possible case.
        var_basename = "Boolean"
        if ifdefs:
            var_basename = "DebugBoolean"

        # Set ifdefs
        defs = []
        if ifdefs:
            defs = ["PeanoDebug > 0"]

        # NOTE: initial values, where provided, are hardcoded to be checked
        # for equality in the test. If you modify them, the test will fail.
        self.data.add_attribute(
            dastgen2.attributes.Boolean(
                "my" + var_basename,
                qualifier=dastgen2.attributes.Attribute.Qualifier.NONE,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Boolean(
                "myInitval" + var_basename,
                initval="true",
                qualifier=dastgen2.attributes.Attribute.Qualifier.NONE,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Boolean(
                "myStatic" + var_basename,
                qualifier=dastgen2.attributes.Attribute.Qualifier.STATIC,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Boolean(
                "myStaticInitval" + var_basename,
                initval="true",
                qualifier=dastgen2.attributes.Attribute.Qualifier.STATIC,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Boolean(
                "myConst" + var_basename,
                initval="true",
                qualifier=dastgen2.attributes.Attribute.Qualifier.CONST,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Boolean(
                "myConstStatic" + var_basename,
                initval="true",
                qualifier=dastgen2.attributes.Attribute.Qualifier.CONST_STATIC,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Boolean(
                "myConstExpr" + var_basename,
                initval="true",
                qualifier=dastgen2.attributes.Attribute.Qualifier.CONSTEXPR,
                ifdefs=defs,
            )
        )

        return

    def _generate_double_attributes(self, compress: bool, ifdefs: bool):
        """!
        Generate double attributes.

        compress: Boolean
            if True, enable compression.

        ifdefs: Boolean
            if True, add debug ifdefs to the attributes.
        """

        # Set the variable basename, so we get different
        # variable names for each possible case.
        var_basename = "Double"
        if compress and not ifdefs:
            var_basename = "CompressedDouble"
        if ifdefs and not compress:
            var_basename = "DebugDouble"
        if ifdefs and compress:
            var_basename = "DebugCompressedDouble"

        # Set compression values
        valid_mantissa_bits = None
        if compress:
            valid_mantissa_bits = 23

        # Set ifdefs
        defs = []
        if ifdefs:
            defs = ["PeanoDebug > 0"]

        # NOTE: initial values, where provided, are hardcoded to be checked
        # for equality in the test. If you modify them, the test will fail.
        self.data.add_attribute(
            dastgen2.attributes.Double(
                "my" + var_basename,
                qualifier=dastgen2.attributes.Attribute.Qualifier.NONE,
                valid_mantissa_bits=valid_mantissa_bits,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Double(
                "myInitval" + var_basename,
                initval=4.0,
                qualifier=dastgen2.attributes.Attribute.Qualifier.NONE,
                valid_mantissa_bits=valid_mantissa_bits,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Double(
                "myStatic" + var_basename,
                qualifier=dastgen2.attributes.Attribute.Qualifier.STATIC,
                valid_mantissa_bits=valid_mantissa_bits,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Double(
                "myStaticInitval" + var_basename,
                initval=8.0,
                qualifier=dastgen2.attributes.Attribute.Qualifier.STATIC,
                valid_mantissa_bits=valid_mantissa_bits,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Double(
                "myConst" + var_basename,
                initval=16.0,
                qualifier=dastgen2.attributes.Attribute.Qualifier.CONST,
                valid_mantissa_bits=valid_mantissa_bits,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Double(
                "myConstStatic" + var_basename,
                initval=32.0,
                qualifier=dastgen2.attributes.Attribute.Qualifier.CONST_STATIC,
                valid_mantissa_bits=valid_mantissa_bits,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Double(
                "myConstExpr" + var_basename,
                initval=64.0,
                qualifier=dastgen2.attributes.Attribute.Qualifier.CONSTEXPR,
                valid_mantissa_bits=valid_mantissa_bits,
                ifdefs=defs,
            )
        )

        return

    def _generate_enum_attributes(self, ifdefs: bool):
        """!
        Generate enum attributes.

        ifdefs: Boolean
            if True, add debug ifdefs to the attributes.
        """

        # Set the variable basename, so we get different
        # variable names for each possible case.
        var_basename = "Enum"

        # Set ifdefs
        defs = []
        if ifdefs:
            var_basename = "DebugEnum"
            defs = ["PeanoDebug > 0"]

        self.data.add_attribute(
            dastgen2.attributes.Enumeration(
                "my" + var_basename,
                variants=["Foo", "Bar", "Baz", "Last"],
                qualifier=dastgen2.attributes.Attribute.Qualifier.NONE,
                ifdefs=defs,
            )
        )

        return

    def _generate_integer_attributes(self, compress: bool, ifdefs: bool):
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
                qualifier=dastgen2.attributes.Attribute.Qualifier.STATIC,
                min_value=min_value,
                max_value=max_value,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.Integer(
                "myStaticInitval" + var_basename,
                initval=1234,
                qualifier=dastgen2.attributes.Attribute.Qualifier.STATIC,
                min_value=min_value,
                max_value=max_value,
                ifdefs=defs,
            )
        )
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

    def _generate_string_attributes(self, ifdefs: bool):
        """!
        Generate string attributes.

        ifdefs: Boolean
            if True, add debug ifdefs to the attributes.
        """

        # Set the variable basename, so we get different
        # variable names for each possible case.
        var_basename = "String"

        # Set ifdefs
        defs = []
        if ifdefs:
            var_basename = "DebugString"
            defs = ["PeanoDebug > 0"]

        # NOTE: initial values, where provided, are hardcoded to be checked
        # for equality in the test. If you modify them, the test will fail.
        self.data.add_attribute(
            dastgen2.attributes.String(
                "my" + var_basename,
                qualifier=dastgen2.attributes.Attribute.Qualifier.NONE,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.String(
                "myInitval" + var_basename,
                initval="initval",
                qualifier=dastgen2.attributes.Attribute.Qualifier.NONE,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.String(
                "myStatic" + var_basename,
                qualifier=dastgen2.attributes.Attribute.Qualifier.STATIC,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.String(
                "myStaticInitval" + var_basename,
                initval="static_initval",
                qualifier=dastgen2.attributes.Attribute.Qualifier.STATIC,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.String(
                "myConst" + var_basename,
                initval="const_initval",
                qualifier=dastgen2.attributes.Attribute.Qualifier.CONST,
                ifdefs=defs,
            )
        )
        self.data.add_attribute(
            dastgen2.attributes.String(
                "myConstStatic" + var_basename,
                initval="const_static_initval",
                qualifier=dastgen2.attributes.Attribute.Qualifier.CONST_STATIC,
                ifdefs=defs,
            )
        )
        # Not implemented yet
        #  self.data.add_attribute(
        #      dastgen2.attributes.String(
        #          "myConstExpr" + var_basename,
        #          initval="constexpr_initval",
        #          qualifier=dastgen2.attributes.Attribute.Qualifier.CONSTEXPR,
        #          ifdefs=defs,
        #      )
        #  )

        return

    def _generate_boolean_array_attributes(self, compress: bool, ifdefs: bool):
        """!
        Generate boolean-array attributes.

        compress: Boolean
            if True, enable compression.

        ifdefs: Boolean
            if True, add debug ifdefs to the attributes.
        """

        # Set the variable basename, so we get different
        # variable names for each possible case.
        var_basename = "BooleanArray"
        if compress and not ifdefs:
            var_basename = "CompressedBooleanArray"
        if ifdefs and not compress:
            var_basename = "DebugBooleanArray"
        if ifdefs and compress:
            var_basename = "CompressedDebugBooleanArray"

        # Set ifdefs
        defs = []
        if ifdefs:
            defs = ["PeanoDebug > 0"]

        self.data.add_attribute(
            dastgen2.attributes.BooleanArray(
                "my" + var_basename,
                cardinality=8,
                qualifier=dastgen2.attributes.Attribute.Qualifier.NONE,
                compress=compress,
                ifdefs=defs,
            )
        )

        return

    def _generate_integer_array_attributes(self, compress: bool, ifdefs: bool):
        """!
        Generate integer-array attributes.

        compress: Boolean
            if True, enable compression.

        ifdefs: Boolean
            if True, add debug ifdefs to the attributes.
        """

        # Set the variable basename, so we get different
        # variable names for each possible case.
        var_basename = "IntegerArray"
        if compress and not ifdefs:
            var_basename = "CompressedIntegerArray"
        if ifdefs and not compress:
            var_basename = "DebugIntegerArray"
        if ifdefs and compress:
            var_basename = "CompressedDebugIntegerArray"

        # Set ifdefs
        defs = []
        if ifdefs:
            defs = ["PeanoDebug > 0"]

        # Set compression values
        min_value = None
        max_value = None
        if compress:
            min_value = 0
            max_value = 2097152

        self.data.add_attribute(
            dastgen2.attributes.IntegerArray(
                "my" + var_basename,
                cardinality=8,
                qualifier=dastgen2.attributes.Attribute.Qualifier.NONE,
                min_value=None,
                max_value=None,
                ifdefs=defs,
            )
        )

        return

    def _generate_double_array_attributes(self, compress: bool, ifdefs: bool):
        """!
        Generate double-array attributes.

        compress: Boolean
            if True, enable compression.

        ifdefs: Boolean
            if True, add debug ifdefs to the attributes.
        """

        # Set the variable basename, so we get different
        # variable names for each possible case.
        var_basename = "DoubleArray"
        if compress and not ifdefs:
            var_basename = "CompressedDoubleArray"
        if ifdefs and not compress:
            var_basename = "DebugDoubleArray"
        if ifdefs and compress:
            var_basename = "CompressedDebugDoubleArray"

        # Set ifdefs
        defs = []
        if ifdefs:
            defs = ["PeanoDebug > 0"]

        # Set compression values
        valid_mantissa_bits = None
        if compress:
            valid_mantissa_bits = 23

        self.data.add_attribute(
            dastgen2.attributes.DoubleArray(
                "my" + var_basename,
                cardinality=8,
                qualifier=dastgen2.attributes.Attribute.Qualifier.NONE,
                valid_mantissa_bits=valid_mantissa_bits,
                ifdefs=defs,
            )
        )

        return

    def _generate_peano_integer_array_attributes(self, compress: bool, ifdefs: bool):
        """!
        Generate peano4 integer-array attributes.

        compress: Boolean
            if True, enable compression.

        ifdefs: Boolean
            if True, add debug ifdefs to the attributes.
        """

        # Set the variable basename, so we get different
        # variable names for each possible case.
        var_basename = "PeanoIntegerArray"
        if compress and not ifdefs:
            var_basename = "CompressedPeanoIntegerArray"
        if ifdefs and not compress:
            var_basename = "DebugPeanoIntegerArray"
        if ifdefs and compress:
            var_basename = "CompressedDebugPeanoIntegerArray"

        # Set ifdefs
        defs = []
        if ifdefs:
            defs = ["PeanoDebug > 0"]

        # Set compression values
        min_value = None
        max_value = None
        if compress:
            min_value = 0
            max_value = 2097152

        self.data.add_attribute(
            peano4.dastgen2.Peano4IntegerArray(
                "my" + var_basename,
                cardinality=8,
                min_value=None,
                max_value=None,
                ifdefs=defs,
            )
        )

        return

    def _generate_peano_double_array_attributes(self, compress: bool, ifdefs: bool):
        """!
        Generate peano4 double-array attributes.

        compress: Boolean
            if True, enable compression.

        ifdefs: Boolean
            if True, add debug ifdefs to the attributes.
        """

        # Set the variable basename, so we get different
        # variable names for each possible case.
        var_basename = "PeanoDoubleArray"
        if compress and not ifdefs:
            var_basename = "CompressedPeanoDoubleArray"
        if ifdefs and not compress:
            var_basename = "DebugPeanoDoubleArray"
        if ifdefs and compress:
            var_basename = "CompressedDebugPeanoDoubleArray"

        # Set ifdefs
        defs = []
        if ifdefs:
            defs = ["PeanoDebug > 0"]

        # Set compression values
        valid_mantissa_bits = None
        if compress:
            valid_mantissa_bits = 23

        self.data.add_attribute(
            peano4.dastgen2.Peano4DoubleArray(
                "my" + var_basename,
                cardinality=8,
                valid_mantissa_bits=valid_mantissa_bits,
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
            touch_vertex_first_time_kernel="""::swift2::dastgenTest::checkDastgen(assignedParticles);""",
            prepare_traversal_kernel="""
                ::swift2::dastgenTest::dummyMoveForwardInTime<globaldata::{}>();""".format(
                self.name
            ),
            includes="""
                           #include "DastgenTest.h"
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
            touch_vertex_last_time_kernel="""::swift2::dastgenTest::checkDastgen(assignedParticles);""",
            unprepare_traversal_kernel="""
                                       globaldata::{PARTICLE}::getSpecies().setTimeStamp( globaldata::{PARTICLE}::getSpecies().getMinTimeStamp() + globaldata::{PARTICLE}::getSpecies().getMinTimeStepSize(), false );
                                       ::swift2::dastgenTest::reportStep<globaldata::{PARTICLE}>( "{PARTICLE}" );
                                       """.replace(
                "{PARTICLE}", self.name
            ),
            includes="""
                           #include "DastgenTest.h"
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
