# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Attribute import Attribute


class String(Attribute):
    """!

    Wrapper around C++ string which is not a dataype supported by
    MPI natively.

    max_length  Maximum length of the strings that can be handled.
                The shorter you keep this value, the smaller the
                MPI message size, as we don't use dynamic data
                structures. We always map the C++ string onto an
                array with max_length entries.

    """

    def __init__(
        self,
        name,
        max_length=80,
        ifdefs=[],
        qualifier=Attribute.Qualifier.NONE,
        initval=None,
    ):
        if initval is not None:
            # sanitize initval
            # Make sure it starts with a `"`
            if not initval.startswith('"'):
                initval = '"' + initval
            # and that it ends with a `\0"`
            if not initval.endswith('\\0"'):
                if initval.endswith('"'):
                    # only cut off trailing '"'
                    # will be added back one line below, after the null character
                    initval = initval[:-1]
                initval += '\\0"'

        # Do we need to expose this attribute in the header file?
        # Only necessary for functions that will be inlined, i.e. constexpr
        expose = qualifier == Attribute.Qualifier.CONSTEXPR

        Attribute.__init__(
            self,
            name,
            ifdefs,
            qualifier=qualifier,
            initval=initval,
            expose_in_header_file=expose,
        )
        self._max_length = max_length

        return

    def get_methods(self, _full_qualified_class_name, for_declaration=True):
        accessor_name = self.get_accessor_name()
        methods = []

        if self._is_static:
            if for_declaration:
                # No 'const' here: static member function cannot have 'const' qualifier
                getter = ("get" + accessor_name + "()", "static std::string")
            else:
                # No 'static' here: 'static' can only be specified inside the class definition
                getter = ("get" + accessor_name + "()", "std::string")

        elif self._is_const_static:
            if for_declaration:
                # No 'const' here: static member function cannot have 'const' qualifier
                getter = ("get" + accessor_name + "()", "static std::string")
            else:
                # No 'static' here: 'static' can only be specified inside the class definition
                # No 'const' here either: function definition will then differ from that in declaration
                getter = ("get" + accessor_name + "()", "std::string")

        elif self._is_const:
            getter = ("get" + accessor_name + "() const", "const std::string")

        elif self._is_constexpr:
            raise NotImplementedError("Strings + constexpr not implemented yet.")
            #  getter = ("get" + accessor_name + "() const [80]", "constexpr char")

        else:
            # default case.
            getter = ("get" + accessor_name + "() const", "std::string")

        methods.append(getter)

        # Add a setter method, if it makes sense.
        if not (self._is_const_static or self._is_const or self._is_constexpr):
            methods.append(
                ("set" + accessor_name + "(const std::string& value)", "void")
            )

        return methods

    def get_plain_C_attributes(self, for_constructor=False):
        typestring = "char"
        namestring = "_" + self._name + "[" + str(self._max_length) + "]"
        lengthtype = (
            "int"  # type + qualifiers string for integer carrying string length
        )
        lengthval = None

        # add const/static qualifiers and initialization values, if needed
        if self._is_static:
            typestring = "inline static char"
            lengthtype = "inline static int"

            if self._initval is not None:
                namestring += " = " + self._initval
                # initval is sanitized at this point. Take away 4:
                # 2 aposprophes, and 2 extra '\' to escape the NULL char
                lengthval = len(self._initval) - 4
                lengthtype = "inline static int"

        elif self._is_const_static:
            typestring = "inline const static char"
            namestring += " = " + self._initval
            # initval is sanitized at this point. Take away 4:
            # 2 aposprophes, and 2 extra '\' to escape the NULL char
            lengthval = len(self._initval) - 4
            lengthtype = "inline const static int"

        elif self._is_const:
            typestring = "const char"
            namestring += " = " + self._initval
            # initval is sanitized at this point. Take away 4:
            # 2 aposprophes, and 2 extra '\' to escape the NULL char
            lengthval = len(self._initval) - 4
            lengthtype = "const int"

        elif self._is_constexpr:
            raise NotImplementedError("Strings + constexpr not implemented yet.")
            #  typestring = "constexpr static char"
            #  namestring += " = " + self._initval
            #  lengthval = len(self._initval) - 4
            #  lengthtype = "constexpr int"

        else:
            # default case
            # If an initial value is provided, add it
            # However, only add it if we need it for a definition,
            # not as optional argument for constructor method
            if self._initval is not None and not for_constructor:
                namestring += " = " + self._initval
                # initval is sanitized at this point. Take away 4:
                # 2 aposprophes, and 2 extra '\' to escape the NULL char
                lengthval = len(self._initval) - 4

        # construct name of integer which contains string's length
        lengthstring = "_" + self._name + "Length"
        if lengthval is not None:
            # if lengthval has a value, it means that we are using
            # some intial value. So add the correct value of the length.
            lengthstring += " = " + str(lengthval)

        return [
            (namestring, typestring, ""),
            (lengthstring, lengthtype),
        ]

    def get_setter_getter_name(self):
        return self._name

    def get_method_body(self, signature):
        if signature.startswith("get"):
            return (
                """
  std::ostringstream result;
  for (int i=0; i<_"""
                + self._name
                + """Length; i++) {
    result << static_cast<char>(_"""
                + self._name
                + """[i]);
  }
  return result.str();
"""
            )
        elif signature.startswith("set"):
            return (
                """
  _"""
                + self._name
                + """Length = value.length();
  for (int i=0; i<value.length(); i++) {
    _"""
                + self._name
                + """[i] = value.data()[i];
  }

"""
            )
        else:
            assert False
        return ""

    def get_native_MPI_type(self):
        return [("MPI_CHAR", self._max_length), ("MPI_INT", 1)]

    def get_to_string(self):
        """

        Return string representation of attribute.

        """
        return "get" + self.get_accessor_name() + "()"
