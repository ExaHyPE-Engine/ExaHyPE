# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import IntEnum


class DoFAssociation(IntEnum):
    Undef = 0
    Vertex = 1
    Face = 2
    Cell = 3
    Global = 4


def get_logical_type_name_for_association(association, name):
    result = ""
    if association == DoFAssociation.Vertex:
        result += "VertexData"
    elif association == DoFAssociation.Cell:
        result += "CellData"
    elif association == DoFAssociation.Face:
        result += "FaceData"
    elif association == DoFAssociation.Global:
        result += "GlobalData"
    else:
        assert False
    result += name
    return result


def get_subnamespace_for_association(association):
    if association == DoFAssociation.Vertex:
        return "vertexdata"
    elif association == DoFAssociation.Cell:
        return "celldata"
    elif association == DoFAssociation.Face:
        return "facedata"
    elif association == DoFAssociation.Global:
        return "globaldata"
    else:
        assert False


class DoF(object):
    def __init__(self, name):
        """configure() sets the association and the namespace."""
        self.association = DoFAssociation.Undef
        self.name = name
        self.namespace = []

    def configure(self, namespace, association):
        """The model calls it when it adds the object."""
        self.namespace = [x for x in namespace]
        self.association = association
        self.namespace += [get_subnamespace_for_association(association)]

    def get_full_qualified_type(self):
        result = ""
        for i in self.namespace:
            result += i
            result += "::"
        result += self.name
        return result

    def get_logical_type_name(self):
        """Returns the name of the data type within the data repository and the action sets."""
        return get_logical_type_name_for_association(self.association, self.name)

    def get_enumeration_type(self):
        result = ""
        if self.association == DoFAssociation.Vertex:
            result += "::peano4::dataview::VertexEnumerator<"
        elif self.association == DoFAssociation.Face:
            result += "::peano4::dataview::FaceEnumerator<"
        else:
            assert False, "Association was {}".format(self.association)
        result += self.get_full_qualified_type()
        result += ">"
        return result
