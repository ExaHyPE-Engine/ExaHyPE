# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .PatchToDoubleArray import PatchToDoubleArray
from .DoF                import DoF


class Patch(DoF):
  def __init__(self, dim, no_of_unknowns, name):
    """
      dim should be a two-tuple or a three-tuple of integers, so a construction 
      could look like
      
        cell_data = peano4.datamodel.Patch( (6,6,6), "Fluid" )

    """
    super(Patch, self).__init__(name)
    self.dim       = dim
    self.no_of_unknowns = no_of_unknowns
    self.generator = PatchToDoubleArray(self)

  def __str__(self):
    return """
Dim:       """ + str(self.dim) + """
Unknowns:  """ + str(self.no_of_unknowns) + """
Generator: """ + str(self.generator) + """
"""    



