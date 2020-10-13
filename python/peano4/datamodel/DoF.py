# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import IntEnum


class DoFAssociation(IntEnum):
  """
  
   Superclass has to be IntEnum, as I use this one within Jinja2 templates
   where I struggled to compare against enum variants. I however can always
   compare against integers.
  
  """
  Undef  = 0
  Vertex = 1
  Face   = 2
  Cell   = 3
  #
  # I use generic for standard MPI messages or DaStGen messages which are used
  # within the grid mangement and traversal, i.e. have nothing to do with particular
  # grid entities
  #
  Generic = 4
  Global = 5
  
  
  
class DoF(object):
  def __init__(self, name):
    self.association = DoFAssociation.Undef
    self.name      = name
    self.namespace = []

  
  
  def configure(self,namespace,association):
    """
    Typically called by model as soon as you add an object to it
    """
    self.namespace = [x for x in namespace]
    self.association = association
    
    if association==DoFAssociation.Vertex:
      self.namespace += [ "vertexdata" ]
    elif association==DoFAssociation.Cell:
      self.namespace += [ "celldata" ]
    elif association==DoFAssociation.Face:
      self.namespace += [ "facedata" ]
    elif association==DoFAssociation.Global:
      self.namespace += [ "globaldata" ]
    else:
      assert False
    #print( "added a new DoF: " + str(self.namespace) )
    

  def get_full_qualified_type(self):
    result = ""
    for i in self.namespace:
      result += i
      result += "::"
    result += self.name
    return result
    

  def get_logical_type_name(self):
    """
      What should the data type be called within the data repository,
      or within action sets. We add a prefix name here.
    """
    result = ""
    if self.association==DoFAssociation.Vertex:
      result += "VertexData"
    elif self.association==DoFAssociation.Cell:
      result += "CellData"
    elif self.association==DoFAssociation.Face:
      result += "FaceData"
    elif association==DoFAssociation.Global:
      result += "GlobalData"
    else:
      assert False
    result += self.name
    return result


  def get_enumeration_type(self):
    """
      What should the data type be called within the data repository.
    """
    result = ""
    if self.association==DoFAssociation.Vertex:
      result += "peano4::datamanagement::VertexEnumerator<"
    elif self.association==DoFAssociation.Face:
      result += "peano4::datamanagement::FaceEnumerator<"
    #elif self.association==DoFAssociation.Cell:
    #  result += "peano4::datamanagement::CellWrapper<"
    else:
      assert False
    result += self.get_full_qualified_type()
    result += ">"
    return result

