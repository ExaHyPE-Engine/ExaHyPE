# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import IntEnum


class DoFAssociation(IntEnum):
    """
    Superclass has to be IntEnum, as we use this one within Jinja2 templates.

    Jinja2 templates cannot compare against enum variants, but they can always
    compare against integers.
    """

    Undef = 0
    Vertex = 1
    Face = 2
    Cell = 3
    # Generic covers standard MPI messages or generated messages used within the
    # grid management and traversal, i.e. those with no tie to a particular grid
    # entity.
    Generic = 4
    Global = 5


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
        """
        Both the association and the namespace are not to be set directly.

        Use the operation configure() instead.

        ## Additional store/load/send/receive arguments

        - The first entry of this triple is the expression that you want the
          observer to pass in.
        - The second one is the C++ type of this expression.
        - The third one is the name that you want to use in the dof signatures
          for this argument.

          The DoF subclass initialises the store and load routines from the
          second entry. Most subclasses define an additional setter for the
          property.

        ## Attributes

        association: DoFAssociation

        name:        String
          Has to be a fit to the C++ naming conventions

        namespace:   [String]
          Sequence of namespaces.
        """
        self.association = DoFAssociation.Undef
        self.name = name
        self.namespace = []

    def configure(self, namespace, association):
        """Typically called by model as soon as you add an object to it."""
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
        """
        What should the data type be called within the data repository or action sets.

        We add a prefix name here.
        """
        return get_logical_type_name_for_association(self.association, self.name)

    def get_enumeration_type(self):
        """What should the data type be called within the data repository."""
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
