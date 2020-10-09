# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Makefile import Makefile


class Output(object):
  """
  Represents the total output generated from the Peano4 data model plus all
  the operations on it. This is basically a big collection.
  """
  
  def __init__(self):
    self.artefacts = []
    self.makefile = Makefile()


  def clear(self):
    """
    
      The clear eliminates all artefacts, but it does not erase the Makefile. 
      This is important as I don't want to loose all the makefile info. If 
      you want to clear the makefile, too, you have to invoke a clear on it.
    
    """
    self.artefacts = []
    self.makefile.clear_files()

          
  def add(self,artefact,append=True):
    if append:
      self.artefacts.append(artefact)
    else:
      self.artefacts = [artefact] + self.artefacts
        
    
  def generate(self,overwrite,directory):
    """
    Iterates over all stored artefacts and, hence, finally generates all the
    C files, makefiles, and so forth.
    """
    self.makefile.generate(overwrite, directory)
    for artefact in self.artefacts:
      artefact.generate(overwrite,directory)
      
