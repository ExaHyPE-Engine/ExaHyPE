# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Makefile  import Makefile
from .Readme    import Readme


_PARALLELISATION_README_DESCRIPTION = r"""
### MPI parallelisation

Peano's distributed memory parallelisation is based upon plain space-filling
curves which are used first to split the cells among the ranks, and, after
that, to split the cells into chunks and deploy them onto cores (if shared
memory is used). There is no particularly novel contribution/paper on this
aspect of the code. The paper that is (algorithmically) most interesting is
an old EuroPar publication:

       @InProceedings{Bungartz:2006:Parallelisation,
         author={Bungartz, Hans-Joachim and Mehl, Miriam and Weinzierl, Tobias},
         editor={Nagel, Wolfgang E. and Walter, Wolfgang V. and Lehner, Wolfgang},
         title={A Parallel Adaptive Cartesian PDE Solver Using Space--Filling Curves},
         booktitle={Euro-Par 2006 Parallel Processing},
         year={2006},
         publisher={Springer Berlin Heidelberg},
         address={Berlin, Heidelberg},
         pages={1064--1074},
         isbn={978-3-540-37784-9}
       }

### OpenMP parallelisation

Peano 4 run uses a wrapper around OpenMP to obtain a high task efficiency.
The wrapper can be read as user-level threading implemented on top of OpenMP's
tasking mechanism. It is described in

       @article{Schulz:2021:Tasking,
         title = {Task inefficiency patterns for a wave equation solver},
         journal = {IWOMP},
         year = {2021},
         author = {Holger Schulz and Gonzalo Brito Gadeschi and Oleksandr Rudyy and Tobias Weinzierl},
      }

Peano relies on a mixture of classic domain decomposition and task-based
parallelism. The domain decomposition provides the baseline performance,
and the tasking adds flexibility and scalability on top.
"""


class Output(object):
  """
  Represents the total output generated from the Peano4 data model plus all
  the operations on it. This is basically a big collection.
  """
  def __init__(self):
    self.artefacts  = []
    self.readme     = Readme()
    self.makefile   = Makefile()


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


  def generate(self,directory):
    """
    Iterates over all stored artefacts and, hence, finally generates all the
    C files, makefiles, and so forth.

    Files the user registered as non-generated source files (via the makefile's
    add_h_file/add_cpp_file/add_Fortran_file) are protected: the code generator
    never overwrites them. Everything else is always (re)generated.
    """
    protected_files = self.makefile.protected_files()
    self.makefile.generate(directory)
    for artefact in self.artefacts:
      artefact.generate(directory, protected_files)

    self.readme.set_executable_name( self.makefile.executable_name() )
    self.readme.add_package_description( _PARALLELISATION_README_DESCRIPTION )

    self.readme.generate(directory)
