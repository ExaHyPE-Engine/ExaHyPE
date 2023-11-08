import os, sys
import argparse
import peano4

import multigrid

print( """

Peano 4 multigrid solver using Discontinuous Galerkin

@author 2022 Sean Baccas, Dmitry Nikolaenko, Tobias Weinzierl

""")

parser = argparse.ArgumentParser(description='Multigrid - Poisson solver')
parser.add_argument("-j",   "--parallel-builds",    dest="j",              type=int, default=-1, help="Parallel builds" )
parser.add_argument("-pd",  "--peano-dir",          dest="peanodir",       default="../../../../", help="Peano4 directory" )
parser.add_argument("-v",   "--verbose",            dest="verbose",        action="store_true", default=False, help="Verbose")
parser.add_argument("-d",   "--dimensions",         dest="dimensions",     type=int, default=2, help="Dimensions")
parser.add_argument("-upv", "--unknowns_per_vertex",dest="upv",            type=int, default=1, help="Unknowns per vertex")
parser.add_argument("-meshsize",  "--meshsize",     dest="meshsize",       default=0.3, help="Mesh size")
parser.add_argument("-m",   "--mode",               dest="mode",           choices=["release","stats","asserts", "trace"], required=True, help="Pick build type" )
args = parser.parse_args()

project = multigrid.matrixfree.Project( project_name = "CollocatedFEM", 
                                        namespace = [ "benchmarks", "multigrid", "matrixfree", "poisson" ]
                                         )

