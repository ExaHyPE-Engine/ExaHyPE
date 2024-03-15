# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import sys

from .Makefile import Makefile

class Readme(object):
  """
  Creates one README for the whole setup, so people can look up what 
  they have actually built.

  The README has some default content. Then it hosts some info about the
  actual build. This information is provided from the makefile just before
  the readme is actually dumped. Finally, there is information about the
  individual tools used. This information is generated from
  peano4.datamodel.Model where each individual model that we use adds
  entries to the readme. Particular toolboxes or extensions can add their
  information through their models, too
  """

  __Header = """
# Readme of a Peano 4 project

This experiment did rely on Peano 4 or a toolbox created on top of Peano.   
If you publish results obtained with this source code, we would appreciate  
if you could point to the project homepage                                  
                                                                            
http://www.peano-framework.org/                                             
                                                                            
and cite the appropriate papers from below.


## Peano 4 ##

Peano's core ideas, algorithms and (usage) vision are best discussed in the 
TOMS paper below. The TOMS paper describes the third generation of Peano. 
While it is thus not a perfect description, the core concepts by means of 
data structures and user model is the same as in Peano 4. Please cite this
paper when you use the software:

        @article{Weinzierl:2019:Peano,                    
          author = {Weinzierl, Tobias},                   
          title = {The Peano Software-Parallel, Automaton-Based, Dynamically Adaptive Grid Traversals},                    
          year = {2019},                    
          issue_date = {June 2019},                    
          publisher = {Association for Computing Machinery},                    
          address = {New York, NY, USA},                    
          volume = {45},                    
          number = {2},                    
          issn = {0098-3500},                    
          url = {https://doi.org/10.1145/3319797},                    
          doi = {10.1145/3319797},                    
          abstract = {We discuss the design decisions, design alternatives, and rationale behind the third generation of Peano, a framework for dynamically adaptive Cartesian meshes derived from spacetrees. Peano ties the mesh traversal to the mesh storage and supports only one element-wise traversal order resulting from space-filling curves. The user is not free to choose a traversal order herself. The traversal can exploit regular grid subregions and shared memory as well as distributed memory systems with almost no modifications to a serial application code. We formalize the software design by means of two interacting automata-one automaton for the multiscale grid traversal and one for the application-specific algorithmic steps. This yields a callback-based programming paradigm. We further sketch the supported application types and the two data storage schemes realized before we detail high-performance computing aspects and lessons learned. Special emphasis is put on observations regarding the used programming idioms and algorithmic concepts. This transforms our report from a "one way to implement things" code description into a generic discussion and summary of some alternatives, rationale, and design decisions to be made for any tree-based adaptive mesh refinement software.},
          journal = {ACM Trans. Math. Softw.},                    \
          month = apr,                    
          articleno = {14},                    
          numpages = {41},                    
          keywords = {Software, spacetree, adaptive mesh refinement, parallel multiscale grid traversal
        }


There is an earlier publication about Peano in SISC which discusses the core 
algorithms with the stacks in great detail. This paper can be seen as release 
paper for previous Peano code generations (generation 2).


        @article{10.1137/100799071,
          author = {Weinzierl, Tobias and Mehl, Miriam},
          title = {Peano-A Traversal and Storage Scheme for Octree-Like Adaptive Cartesian Multiscale Grids}, 
          year = {2011}, 
          issue_date = {September 2011}, 
          publisher = {Society for Industrial and Applied Mathematics}, 
          address = {USA}, 
          volume = {33}, 
          number = {5}, 
          issn = {1064-8275}, 
          url = {https://doi.org/10.1137/100799071}, 
          doi = {10.1137/100799071}, 
          abstract = {Almost all approaches to solving partial differential equations (PDEs) are based upon a spatial discretization of the computational domain - a grid. This paper presents an algorithm to generate, store, and traverse a hierarchy of $d$-dimensional Cartesian grids represented by a $(k=3)$-spacetree, a generalization of the well-known octree concept, and it also shows the correctness of the approach. These grids may change their adaptive structure throughout the traversal. The algorithm uses $2d+4$ stacks as data structures for both cells and vertices, and the storage requirements for the pure grid reduce to one bit per vertex for both the complete grid connectivity structure and the multilevel grid relations. Since the traversal algorithm uses only stacks, the algorithm's cache hit rate is continually higher than 99.9 percent, and the runtime per vertex remains almost constant; i.e., it does not depend on the overall number of vertices or the adaptivity pattern. We use the algorithmic approach as the fundamental concept for a mesh management for $d$-dimensional PDEs and for a matrix-free PDE solver represented by a compact discrete $3^d$-point operator. In the latter case, one can implement a Jacobi smoother, a Krylov solver, or a geometric multigrid scheme within the presented traversal scheme which inherits the low memory requirements and the good memory access characteristics directly.}, 
          journal = {SIAM J. Sci. Comput.}, 
          month = oct, 
          pages = {2732--2760}, 
          numpages = {29}, 
          keywords = {octree, cache efficiency, multiscale, space-filling curve, adaptive Cartesian grid, spacetree, partial differential equation} 
        }
"""

  def __init__(self):
    self.bibtex_entry = []
    self._executable_name = ""
    self._package_descriptions = set()
    self._entries = []
    self._constants = None

  def add_package_description( self, readme_package_descriptor ): 
    self._package_descriptions.add(readme_package_descriptor)
    pass

  def set_constants(self,description):
    self._constants = description

  def set_executable_name( self, executable_name ):
    self._executable_name = executable_name

  def add_entry( self, readme_entry ):
    self._entries.append(readme_entry)
    pass

  def generate(self,directory):
    filename = directory + "/README-" + self._executable_name + ".md";
    file = open(filename, "w")
    file.write( self.__Header )
    file.write("""

## Information provided by used packages

""")
    for i in self._package_descriptions:
      file.write( i )
    file.write("""

## Information provided by data model

""")
    for i in self._entries:
      file.write( i )
    file.write( """

## Summary

File has been generated by

python3 """)
    for i in sys.argv:
      file.write( " " + i )
    file.write( """

www.peano-framework.org
(C) 2021-  Tobias Weinzierl

""")
