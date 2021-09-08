from paraview.simple import *
from peano4.visualisation.Visualiser import render_single_file 
from peano4.visualisation.Visualiser import render_dataset

import peano4.visualisation

import sys
import os
import argparse


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Peano 4 - pvserver render script')
  parser.add_argument("--no-convert",                dest="convert",                  action="store_false", default = True,  help="Convert into native vtk files" )
  parser.add_argument("--render",                    dest="render",                   action="store_true",  default = False, help="Skip interactive rendering" )
  parser.add_argument("--filter-fine-grid",          dest="filter_fine_grid",         action="store_true",  default = False, help="Display only fine grid" )
  parser.add_argument("--average",                   dest="filter_average",           action="store_true",  default = False, help="Average over cell data (reduces resolution/memory)" )
  parser.add_argument("--eliminate-relative-paths",  dest="eliminate_relative_paths", action="store_true",  default = False, help="If you invoke the script on a different directory than your current working directory, ensure that all meta files written do not hold relative paths" )
  parser.add_argument("-v", "--verbose",             dest="verbose",                  action="store_true",  default = False, help="Run in a chatty mode" )
    
  parser.add_argument(dest="filename", help="Input file name" )
  args = parser.parse_args()

  if not os.path.exists(args.filename):
    print("Error, specified input file '{}' does not exist, exiting...". format(args.filename))
    sys.exit(1)

  visualiser = peano4.visualisation.Visualiser( args.filename, args.verbose )

  filter = []
  #
  # Ensure the filter order is cheap-to-expensive
  #
  if args.filter_average:
    print( "add averaging filter" )
    visualiser.append_filter( peano4.visualisation.AverageOverCellFilter(args.verbose), False )
  if args.filter_fine_grid:
    print( "add fine grid filter" )
    visualiser.append_filter( peano4.visualisation.ExtractFineGridFilter(False, args.verbose), False )

  if args.render:
    visualiser.display()
    Show(tp)
    Interact() #Needed if running from command line
    
  if args.convert:
    visualiser.write_vtu_time_series(args.eliminate_relative_paths)
