from paraview.simple import *

import peano4.visualisation
import peano4.visualisation.filters

import sys
import os
import argparse


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Peano 4 - pvserver render script')
  parser.add_argument("--filter-fine-grid",          dest="filter_fine_grid",             action="store_true",  default = False, help="Display only fine grid" )
  parser.add_argument("--average",                   dest="filter_average",               action="store_true",  default = False, help="Average over cell data (reduces resolution/memory)" )
  parser.add_argument("--norm-calculator",           dest="norm_calculator",              action="store_true",  default = False, help="Compute norm over each point" )
  parser.add_argument("--eliminate-relative-paths",  dest="eliminate_relative_paths",     action="store_true",  default = False, help="If you invoke the script on a different directory than your current working directory, ensure that all meta files written do not hold relative paths" )
  parser.add_argument("--type",                      dest="type",                         choices=["display", "vtu", "patch-file" ],  default="vtu", help="Output format" )
  parser.add_argument("--dir",                       dest="dir",                          default=".", help="Output directory" )
  parser.add_argument("-v", "--verbose",             dest="verbose",                      action="store_true",  default = False, help="Run in a chatty mode" )
    
  parser.add_argument(dest="filename", help="Input file name" )
  args = parser.parse_args()

  if not os.path.exists(args.filename):
    print("Error, specified input file '{}' does not exist, exiting...". format(args.filename))
    sys.exit(1)

  visualiser = None
  if args.type=="display":
    visualiser = peano4.visualisation.output.Interactive( args.filename, args.verbose )
  if args.type=="vtu":
    visualiser = peano4.visualisation.output.VTU( file_name=args.filename, output_directory=args.dir, verbose=args.verbose )
  if args.type=="patch-file":
    visualiser = peano4.visualisation.output.PatchFile( file_name=args.filename, output_directory=args.dir, verbose=args.verbose )

  filter = []
  #
  # Ensure the filter order is cheap-to-expensive
  #
  if args.filter_average:
    print( "add averaging filter" )
    visualiser.append_filter( peano4.visualisation.filters.AverageOverCell(args.verbose), False )
  if args.filter_fine_grid:
    print( "add fine grid filter" )
    visualiser.append_filter( peano4.visualisation.filters.ExtractFineGrid(False, args.verbose), False )
  if args.norm_calculator:
    print( "add norm calculator filter" )
    visualiser.append_filter( peano4.visualisation.filters.Calculator(verbose=args.verbose), False )
    
  
    

  visualiser.display()
