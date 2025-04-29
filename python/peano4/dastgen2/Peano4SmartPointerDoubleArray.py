# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import dastgen2.attributes.DoubleArray


class Peano4SmartPointerDoubleArray(dastgen2.attributes.DoubleArray):
    """!
        
    Specialisation of array using Peano's tarch with a smart pointer

    This routine maps a logical array of fixed size onto a smart pointer class.
    If you use it, the resulting element tied to a cell, face or vertex might 
    induce significantly lower memory movement overhead. However, this is not 
    guaranteed, and requires some careful testing. As you work with smart 
    pointers, the routine gives you direct access to the underlying vector, 
    i.e. provides specialised routines that expose the underlying data 
    structure: There are the standard getters and setters, but there are also
    variants which give you the start pointer data object, while get and set
    are compatible with Peano4DoubleArray's signature.
    
    Using this class is straightforward and doesn't seem to be any different to
    Peano4DoubleArray. However, under the hood you'll alter the behaviour of 
    peano4.datamodel.DaStGen2.
    
    """

    def __init__(self, 
                 name, 
                 cardinality, 
                 ifdefs=[], 
                 ):
        """
            
        See superclass' constructor.

        @param cardinality: String
          This is important: It is not (necessarily) an integer, but can be
          a string which is defined via a pragma or constexpr later.
          
        """
        super(Peano4SmartPointerDoubleArray, self).__init__(
            name, cardinality, valid_mantissa_bits=None, ifdefs=ifdefs
        )
        self._cardinality       = str(cardinality)

    def get_methods(self, _full_qualified_class_name, for_declaration=True):
        """!
            
        This routine defines which operations the generated data type offers
        
        It is very much the same as Peano4DoubleArray, i.e. it hides the 
        underlying smart pointers. If the user calls get, the data is converted
        into a normal tarch vector. If the user calls set, we take the argument
        and copy it into the underlying smart pointer.
        
        However, we add one more routine: getXXXXSmartPointer(). This way, 
        users can get access to the smart pointer data under the hood.
            
        """
        accessor_name = self._name[:1].title() + self._name[1:]
        return [
            (
                "get" + accessor_name + "() const",
                "tarch::la::Vector<" + self._cardinality + ",double>",
            ),
            (
                "set"
                + accessor_name
                + "(const tarch::la::Vector<"
                + self._cardinality
                + ",double>& value)",
                "void",
            ),
            ("get" + accessor_name + "(int index) const", "double"),
            ("set" + accessor_name + "(int index, double value)", "void"),
            (
                "get" + accessor_name + "SmartPointer()",
                "tarch::la::SmartPointerVector<" + self._cardinality + ",double>",
            ),
        ]

    def use_default_copy_constructor(self):
        return True

    def get_plain_C_attributes(self, for_constructor=False):
        return [
                (
                    "_" + self._name,
                    "tarch::la::SmartPointerVector<" + self._cardinality + ",double>",
                )
        ]

    def get_first_plain_C_attribute(self):
        return [("_" + self._name + ".data()[0]", "void")]

    def get_constructor_arguments(self):
        return [
            ("_" + self._name, "tarch::la::Vector<" + self._cardinality + ",double>")
        ]

    def get_method_body(self, signature):
        if self.use_data_store:
            name = "  _dataStore._" + self._name
        else:
            name = "  _" + self._name
            
        if (
            signature.startswith("get")
            and "SmartPointer" in signature
        ):
            return "  return " + name + ";\n"
        if (
            signature.startswith("get")
            and "index" in signature
        ):
            return "  return " + name + "(index);\n"
        elif (
            signature.startswith("set")
            and "index" in signature
        ):
            return name + "(index) = value;\n"
        elif signature.startswith("get"):
            return "  return " + name + ".toVector();\n"
        elif signature.startswith("set"):
            return name + " = value;\n"
        else:
            assert False
        return ""

    def get_native_MPI_type(self):
        return [("MPI_DOUBLE", self._cardinality)]

    def get_to_string(self):
        """

        Return string representation of attribute.

        """
        if self.use_data_store:
             return "_dataStore._" + self._name + ".toVector()"
        else:
            return "_" + self._name + ".toVector()"
