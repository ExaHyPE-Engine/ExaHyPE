# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import os

import matplotlib.pyplot as plt


Colours = [ "#ff0000",
            "#00ff00",
            "#0000ff",
            "#ff00ff",
            "#ffff00",
            "#00ffff",
          ]

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description="""
Peano 4 statistics inspection

Find out which columns in a statistics file have which meaning. The resulting
index can then be used with plot-statistics.py.

""")
  parser.add_argument("file", help="filename of examine")
  args = parser.parse_args()

  print( "Column \t| Counter" )
  print( "--------|----------------" )
  input_file = open( args.file, "r" )
  for line in input_file:
      if line.startswith( "#" ):
          pass
      else:  
          column_index = 0
          for token in line.split(","):
              if token=="t":
                  print( "{}\t|  time (always used to plot)".format( column_index ) )
              else:
                  print( "{}\t| {}".format( column_index, token ) )
              column_index += 1
          exit(0)
  exit(1)