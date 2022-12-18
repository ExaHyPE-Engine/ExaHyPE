from paraview.simple import *

import peano4.visualisation
import peano4.visualisation.filters

import sys
import os
import argparse

def render(filename, filter_fine_grid=False, filter_average=False, norm_calculator=False, shrink_cells=1.0, type="vtu", dir=".", verbose=False):
  if not os.path.exists(filename):
    print("Error, specified input file '{}' does not exist, exiting...". format(filename))
    sys.exit(1)

  visualiser = None
  if type=="display":
    visualiser = peano4.visualisation.output.Interactive(file_name=filename, verbose=verbose )
  if type=="vtu":
    visualiser = peano4.visualisation.output.VTU( file_name=filename, output_directory=dir, verbose=verbose )
  if type=="patch-file":
    visualiser = peano4.visualisation.output.PatchFile( file_name=filename, output_directory=dir, verbose=verbose )

  filter = []
  #
  # Ensure the filter order is cheap-to-expensive
  #
  if filter_average:
    print( "add averaging filter" )
    visualiser.append_filter( peano4.visualisation.filters.AverageOverCell(verbose), False )
  if filter_fine_grid:
    print( "add fine grid filter" )
    visualiser.append_filter( peano4.visualisation.filters.ExtractFineGrid(False, verbose), False )
  if norm_calculator:
    print( "add norm calculator filter" )
    visualiser.append_filter( peano4.visualisation.filters.Calculator(verbose=verbose), False )
  if shrink_cells!=1.0:
    print( "add shrink patches filter" )
    visualiser.append_filter( peano4.visualisation.filters.ShrinkPatches(shrink_cells,verbose=verbose), False )

  visualiser.display()

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Peano 4 - pvserver render script')
  parser.add_argument("--filter-fine-grid",          dest="filter_fine_grid",             action="store_true",  default = False, help="Display only fine grid" )
  parser.add_argument("--average",                   dest="filter_average",               action="store_true",  default = False, help="Average over cell data (reduces resolution/memory)" )
  parser.add_argument("--norm-calculator",           dest="norm_calculator",              action="store_true",  default = False, help="Compute norm over each point" )
  parser.add_argument("--shrink-cells",              dest="shrink_cells",               type=float,           default = 1.0,   help="Shrink or expand patches/cells. Off by default, i.e. set to 1.0" )
  parser.add_argument("--eliminate-relative-paths",  dest="eliminate_relative_paths",     action="store_true",  default = False, help="If you invoke the script on a different directory than your current working directory, ensure that all meta files written do not hold relative paths" )
  parser.add_argument("--type",                      dest="type",                         choices=["display", "vtu", "patch-file" ],  default="vtu", help="Output format" )
  parser.add_argument("--dir",                       dest="dir",                          default=".", help="Output directory" )
  parser.add_argument("-v", "--verbose",             dest="verbose",                      action="store_true",  default = False, help="Run in a chatty mode" )
  parser.add_argument(                               dest="filename", help="Input file name" )
  args = parser.parse_args()

  render(args.filename, args.filter_fine_grid, args.filter_average, args.norm_calculator, args.shrink_cells, args.type, args.dir, args.verbose)
