# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import sys

_HEADER = """
# Readme of a Peano 4 project

This experiment relies on Peano 4.
If you publish results obtained with this source code, we would appreciate
if you could point to the project homepage

http://www.peano-framework.org/

and cite the appropriate papers from below.


## Peano 4 ##

Peano's core ideas, algorithms and (usage) vision are best discussed in the
TOMS paper below. The TOMS paper describes the third generation of Peano.
While it is thus not a perfect description, the core concepts by means of
data structures and user model are the same as in Peano 4. Please cite this
paper when you use the software:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
          journal = {ACM Trans. Math. Softw.},
          month = apr,
          articleno = {14},
          numpages = {41},
          keywords = {Software, spacetree, adaptive mesh refinement, parallel multiscale grid traversal
        }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

There is an earlier publication about Peano in SISC which discusses the core
algorithms with the stacks in great detail. This paper can be seen as release
paper for previous Peano code generations (generation 2).

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
          journal = {SIAM J. Sci. Comput.},
          month = oct,
          pages = {2732--2760},
          numpages = {29},
          keywords = {octree, cache efficiency, multiscale, space-filling curve, adaptive Cartesian grid, spacetree, partial differential equation}
        }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


## ExaHyPE 2

ExaHyPE 2 is the second-generation ExaHyPE engine built on Peano 4. Please cite
the ExaHyPE release paper:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       @article{Reinarz:2020:ExaHyPE,
         title = {ExaHyPE: An engine for parallel dynamically adaptive simulations of wave problems},
         journal = {Computer Physics Communications},
         volume = {254},
         pages = {107251},
         year = {2020},
         issn = {0010-4655},
         doi = {https://doi.org/10.1016/j.cpc.2020.107251},
         url = {https://www.sciencedirect.com/science/article/pii/S001046552030076X},
         author = {Anne Reinarz and Dominic E. Charrier and Michael Bader and Luke Bovard and Michael Dumbser and Kenneth Duru and Francesco Fambri and Alice-Agnes Gabriel and Jean-Matthieu Gallard and Sven K\\"oppel and Lukas Krenz and Leonhard Rannabauer and Luciano Rezzolla and Philipp Samfass and Maurizio Tavelli and Tobias Weinzierl},
         keywords = {Hyperbolic, PDE, ADER-DG, Finite volumes, AMR, MPI, TBB, MPI+X}
       }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


## Particle handling

Peano associates particles with vertices of a dual spacetree and moves them
between mesh levels as their position or search radius changes. The algorithm is
described by T. Weinzierl et al., *Two particle-in-grid realisations on
spacetrees*, Parallel Computing 52 (2016), 42-64,
https://doi.org/10.1016/j.parco.2015.12.007.


## MPI parallelisation

Peano's distributed memory parallelisation is based upon plain space-filling
curves which are used first to split the cells among the ranks, and, after
that, to split the cells into chunks and deploy them onto the cores of a rank.
The paper that is (algorithmically) most interesting is an old EuroPar
publication:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


## OpenMP parallelisation

Peano 4 uses a wrapper around OpenMP to obtain a high task efficiency.
The wrapper can be read as user-level threading implemented on top of OpenMP's
tasking mechanism. It is described in

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       @article{Schulz:2021:Tasking,
         title = {Task inefficiency patterns for a wave equation solver},
         journal = {IWOMP},
         year = {2021},
         author = {Holger Schulz and Gonzalo Brito Gadeschi and Oleksandr Rudyy and Tobias Weinzierl},
       }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Peano relies on a mixture of classic domain decomposition and task-based
parallelism. The domain decomposition provides the baseline performance,
and the tasking adds flexibility and scalability on top.
"""

_FOOTER = """

## Summary

File has been generated by

python3 {}

www.peano-framework.org
(C) 2021- Tobias Weinzierl

"""


class Readme:
    """Writes the README of a generated project: The citations and the generating command."""

    def generate(self, directory):
        with open(directory + "/README.md", "w") as readme:
            readme.write(_HEADER)
            readme.write(_FOOTER.format(" ".join(sys.argv)))
