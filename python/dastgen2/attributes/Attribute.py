# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import abc
from enum import Enum
from dastgen2.Utils import construct_ifdef_string


class Attribute(object):
    """!

    Represents one attribute.

    This is the superclass of all attributes tied

    """

    class Qualifier(Enum):
        """!
        classifies attribute qualifiers.
        """

        NONE = 0
        STATIC = 1
        CONST = 2
        CONST_STATIC = 3
        CONSTEXPR = 4

    def __init__(
        self,
        name,
        ifdefs=[],
        qualifier=Qualifier.NONE,
        initval=None,
        expose_in_header_file=False,
    ):
        """

        name: String
          This is a plain string which has to follow the C++ naming conventions, i.e.
          it is case-sensitive and may not contain any special characters besides the
          underscore. If also has to be unique within the enclosing data model.
          However, this uniqueness is a C++ constraint. A DaStGen model can hold the
          same attribute multiple times, as long as their ifdefs are different and
          hence mask them out, i.e. ensure that only version is "active" at any
          compile run.

        ifdefs: [String]
          A set of strings which have to hold at compile time to determine if this
          attribute does exist or not.

        qualifier: self.Qualifier
          An additional qualifier.

        initval: str or None
          Initial value. The type depends on underlying type. But you can always pass
          in a string that evaluates to the correct type.

        expose_in_header_file: Boolean
          Flag that determines if an attribute's setters and getters should have an
          implementation within the header or if the generated C++ code should strictly
          separate declaration and definition and stick the latter into a separate
          implementation and hence object file.

        """
        self._name = name
        self.use_data_store = False
        self.ifdefs = ifdefs
        self.qualifier = qualifier
        self._initval = initval
        self.expose_in_header_file = expose_in_header_file

        if (
            self._is_const_static or self._is_constexpr or self._is_const
        ) and self._initval is None:
            raise ValueError(
                "Attribute",
                self._name,
                ": You need to provide an initialisation value for a const attribute",
            )

    @property
    def _is_static(self):
        return self.qualifier == self.Qualifier.STATIC

    @property
    def _is_const_static(self):
        return self.qualifier == self.Qualifier.CONST_STATIC

    @property
    def _is_const(self):
        return self.qualifier == self.Qualifier.CONST

    @property
    def _is_constexpr(self):
        return self.qualifier == self.Qualifier.CONSTEXPR

    def get_public_fields(self):
        """!

        Return string that is to be embedded into the public part of the class
        definition. Most attributes don't add anything here, but some (alike
        enumerations) need to.

        """
        return ""

    @abc.abstractmethod
    def get_methods(self, _full_qualified_class_name, for_declaration=True):
        """!

        Return sequence of methods that are defined for this attribute. Each
        entry is a tuple. Its first entry is the signature of the function
        (not including the semicolon), the second entry is the return type.

        for_declaration: Boolean
            if True, assume we want method names for function declaration, not
            definition. Some generators might - for example - add additional
            annotations for the declaration such as Clang attributes. The
            most frequently used use case for this flag is the static
            annotation. To make a function a class function, you have to
            declare it as static. But the definition does not allow you to
            repeat that static keyword again.

        """
        return

    def use_default_copy_constructor(self):
        """!

        If this routine returns False, the generator will create a copy
        constructor copying each attribute over via a setter/getter
        combination.

        """
        return True

    def get_constructor_arguments(self):
        """!

        Return list of tuple of arguments for the constructor. The first
        tuple entry is the name, the second one the type. By default,
        I take them from the plain C attributes, but you can alter
        this behaviour.

        """
        return self.get_plain_C_attributes(for_constructor=True)

    @abc.abstractmethod
    def get_plain_C_attributes(self, for_constructor=False):
        """!

        Return list of n-tuples. The tuple can either host two, three or
        four entries. The list itself may contain arbitrary many tuples, as
        one attribute logically can map onto multiple technical attributes.
        For example, when declaring an array, also declare an integer variable
        containing its length.

        for_constructor: bool
            whether the return value of this function is intended for use
            in the constructor method of the DataModel. If true, will omit
            (optionally provided) initialization values in the attribute
            string.


        ### Two entries

        The first triple entry always is the name, the second one the type.
        Type has to be plain C.

        ### Three entries

        The first triple entry always is the name, the second one the type.
        Type has to be plain C. The third entry is a C++ attribute, i.e.
        a string embedded into [[...]].

        ### Four entries

        The first triple entry always is the name, the second one the type.
        Type has to be plain C. The third entry is a C++ attribute, i.e.
        a string embedded into [[...]].
        The fourth attribute is a list of ifdef symbols that have to be
        defined to use this attribute. The list of
        symbols is concatenated with an and. You can return an empty list.
        All symbol definitions can also be expressions such sa DEBUG>0
        or defined(DEBUG).

        Please note that these latter ifdefs are something completely
        different than the base class ifdefs. The base class ifdefs are
        to be used if you want to switch attributes on and off. The
        attributes here are used to switch between realisation variants.


        ## Arrays

        Arrays can be modelled by adding a cardinality ("[15]" for example)
        to the first triple entry, i.e. the name.

        """
        assert False, "abstract function should not be called"
        return

    def get_first_plain_C_attribute(self):
        """!

        For MPI for example, I need to know the first attribute. If you map
        your attribute onto multiple data types, it is one type that represents the whole thing.

        """
        return self.get_plain_C_attributes()

    def get_attribute_declaration_string(self):
        """!
        Construct the string used for variable declaration using the
        output of get_plain_C_attributes(self). This is a list of tuples,
        each tuple containing two, three, or four entries.

        See get_plain_C_attributes(self) for more details.
        """

        decString = ""
        decString += construct_ifdef_string(self.ifdefs)

        # one attribute logically can map onto multiple technical attributes
        # for example, when declaring an array, also declare an integer variable
        # containing its length
        for subattribute in self.get_plain_C_attributes(for_constructor=False):
            if len(subattribute) == 4:
                decString += construct_ifdef_string(subattribute[3])
                decString += (
                    "    "
                    + subattribute[2]
                    + "  "
                    + subattribute[1]
                    + "   "
                    + subattribute[0]
                    + ";\n"
                )
                decString += "#endif\n"
            elif len(subattribute) == 3:
                decString += (
                    "    "
                    + subattribute[2]
                    + "  "
                    + subattribute[1]
                    + "   "
                    + subattribute[0]
                    + ";\n"
                )
            elif len(subattribute) == 2:
                decString += "    " + subattribute[1] + "   " + subattribute[0] + ";\n"
            else:
                raise IndexError("wrong number of attribute tuples")

        if self.ifdefs != []:
            decString += "#endif \n"

        return decString

    def get_accessor_name(self):
        """!
        Generate the accessor name used throughout dastgen2 to
        create variables, function names, etc.
        """
        accessor_name = self._name[0].capitalize() + self._name[1:]
        return accessor_name

    @abc.abstractmethod
    def get_method_body(self, signature):
        """!

        I hand in the method signature (see get_methods()) and wanna get
        the whole implementation.

        """
        assert False, "not implemented"
        return

    @property
    def name(self):
        """!

        I expect that there's at least one setter/getter pair

        """
        return self._name

    @abc.abstractmethod
    def get_native_MPI_type(self):
        """!

        Return native (built-in) MPI datatype. Return None if there's no
        direct mapping. The actual result is not a string only, but a list
        of tuples from native type to cardinality.

        """
        return

    @abc.abstractmethod
    def get_to_string(self):
        """!

        Return string representation of attribute. Should be something
        that can be streamed via << into a sstream. So the result has
        to be of type string.

        """
        return

    def get_includes(self):
        return ""
