from paraview.simple import *
from peano4.visualisation.Visualiser import render_single_file 
from peano4.visualisation.Visualiser import render_dataset

import peano4.visualisation

import sys
import os
import argparse


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Peano 4 - pvserver render script')
  parser.add_argument("--no-filter-fine-grid", dest="filter_fine_grid", action="store_false", help="Display only fine grid" )
  parser.add_argument("--render",              dest="render",           action="store_true",  default = False, help="Skip interactive rendering" )
  parser.add_argument("--no-convert",          dest="convert",          action="store_false", default = True,  help="Convert into native vtk files" )
  parser.add_argument(dest="filename", help="Input file name" )
  args = parser.parse_args()

  if not os.path.exists(args.filename):
    print("Error, specified input file '{}' does not exist, exiting...". format(args.filename))
    sys.exit(1)

  visualiser = peano4.visualisation.Visualiser( args.filename )

  filter = []
  if args.filter_fine_grid:
    print( "add fine grid filter" )
    visualiser.append_filter( peano4.visualisation.ExtractFineGridFilter(), False )

  if args.render:
    visualiser.display()
    Show(tp)
    Interact() #Needed if running from command line
    
  if args.convert:
    visualiser.apply_filter_to_individual_snapshots_first = False    
    visualiser.write_vtu_time_series()
