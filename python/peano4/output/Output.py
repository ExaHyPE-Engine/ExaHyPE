# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Makefile  import Makefile
from .Readme    import Readme
from .GPULaunch import GPULaunch


class Output(object):
  """
  Represents the total output generated from the Peano4 data model plus all
  the operations on it. This is basically a big collection.
  """
  def __init__(self):
    self.artefacts  = []
    self.readme     = Readme()
    self.makefile   = Makefile()
    self.gpu_launch = GPULaunch()


  def clear(self):
    """
    The clear eliminates all artefacts, but it does not erase the Makefile.
    This is important as I don't want to loose all the makefile info. If
    you want to clear the makefile, too, you have to invoke a clear on it.
    """
    self.artefacts  = []
    self.readme     = Readme()
    self.makefile.clear_files()


  def add(self,artefact,append=True):
    if append:
      self.artefacts.append(artefact)
    else:
      self.artefacts = [artefact] + self.artefacts

  def generate(self,overwrite,directory,subdirectory=""):
    """
    Iterates over all stored artefacts and, hence, finally generates all the
    C files, makefiles, and so forth.
    """
    self.makefile.generate(overwrite, directory, subdirectory)
    for artefact in self.artefacts:
      artefact.generate(overwrite,directory)

    self.readme.set_executable_name( self.makefile.executable_name() )
    self.readme.add_entry( self.makefile.readme_entry )
    self.readme.add_package_description( self.makefile.readme_package_descriptor )

    self.readme.generate(directory)

    self.gpu_launch.generate(directory)
