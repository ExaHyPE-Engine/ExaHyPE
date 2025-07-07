# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Attribute import Attribute


class DoublePointer(Attribute):
    """!
    
    A double pointer
    
    Such a pointer is not initialised by default, i.e. how to handle the 
    underlying data structure is totally up to the user. This data type
    also does not support any MPI data exchange.
    
    """
    def __init__(
        self,
        name,
        ifdefs=[],
        qualifier=Attribute.Qualifier.NONE,
    ):
        """

        """

        # Do we need to expose this attribute in the header file?
        # Only necessary for functions that will be inlined, i.e. constexpr
        expose = qualifier == Attribute.Qualifier.CONSTEXPR

        Attribute.__init__(
            self,
            name,
            ifdefs,
            qualifier=qualifier,
            initval="nullptr",
            expose_in_header_file=expose,
        )
        
        assert not self._is_constexpr, "double pointer cannot be constexpr"
        return

    def get_methods(self, _full_qualified_class_name, for_declaration=True):
        accessor_name = self.get_accessor_name()
        methods = []

        if self._is_static:
            if for_declaration:
                # No 'const' here: static member function cannot have 'const' qualifier
                getter = ("get" + accessor_name + "()", "static double*")
            else:
                # No 'static' here: 'static' can only be specified inside the class definition
                getter = ("get" + accessor_name + "()", "double*")

        elif self._is_const_static:
            if for_declaration:
                # No 'const' here: static member function cannot have 'const' qualifier
                getter = ("get" + accessor_name + "()", "static double*")
            else:
                # No 'static' here: 'static' can only be specified inside the class definition
                # No 'const' here either: function definition will then differ from that in declaration
                getter = ("get" + accessor_name + "()", "double*")

        elif self._is_const:
            getter = ("get" + accessor_name + "() const", "const double*")

        else:
            # default case.
            getter = ("get" + accessor_name + "() const", "double*")

        methods.append(getter)

        # Add a setter method, if it makes sense.
        if not (self._is_const_static or self._is_const or self._is_constexpr):
            methods.append(("set" + accessor_name + "(double* value)", "void"))

        return methods

    def get_plain_C_attributes(self, for_constructor=False):
        typestring = "double*"
        namestring = "_" + self._name

        # add const/static qualifiers and initialization values, if needed
        if self._is_static:
            typestring = "inline static double*"
            if self._initval is not None:
                namestring += " = " + str(self._initval)

        elif self._is_const_static:
            typestring = "inline const static double*"
            namestring += " = " + str(self._initval)

        elif self._is_const:
            typestring = "const double*"
            namestring += " = " + str(self._initval)

        else:
            # default case
            # If an initial value is provided, add it
            # However, only add it if we need it for a definition,
            # not as optional argument for constructor method
            if self._initval is not None and not for_constructor:
                namestring += " = " + str(self._initval)

        # Set up compression/packaging
        return [(namestring, typestring, "")]

    def get_method_body(self, signature):
        if signature.startswith("get") and self.use_data_store:
            return "  return _dataStore._" + self._name + ";\n"
        elif signature.startswith("get") and not self.use_data_store:
            return "  return _" + self._name + ";\n"
        elif signature.startswith("set") and self.use_data_store:
            return "  _dataStore._" + self._name + " = value;\n"
        elif signature.startswith("set") and not self.use_data_store:
            return "  _" + self._name + " = value;\n"
        else:
            assert False
        return ""

    def get_native_MPI_type(self):
        return [("<double pointer should not be used for MPI data exchange>", 1)]

    def get_to_string(self):
        """

        Return string representation of attribute.

        """
        if self.use_data_store:
            return "_dataStore._" + self._name
        else:
            return "_" + self._name
