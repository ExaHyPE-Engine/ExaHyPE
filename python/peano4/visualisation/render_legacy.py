import os
import sys
import argparse

from paraview.simple import *

sys.path.insert(0, os.path.abspath('../../../python'))

import peano4.visualisation
import peano4.visualisation.filters

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Peano 4 - pvserver render script')
  parser.add_argument("--filter-fine-grid",          dest="filter_fine_grid",             action="store_true",  default = False, help="Display only fine grid" )
  parser.add_argument("--average",                   dest="filter_average",               action="store_true",  default = False, help="Average over cell data (reduces resolution/memory)" )
  parser.add_argument("--norm-calculator",           dest="norm_calculator",              action="store_true",  default = False, help="Compute norm over each point" )
  parser.add_argument("--shrink-cells",              dest="shrink_cells",                 type=float,           default = 1.0,   help="Shrink or expand patches/cells. Off by default, i.e. set to 1.0" )
  parser.add_argument("--eliminate-relative-paths",  dest="eliminate_relative_paths",     action="store_true",  default = False, help="If you invoke the script on a different directory than your current working directory, ensure that all meta files written do not hold relative paths" )
  parser.add_argument("--type",                      dest="type",                         choices=["display", "vtu", "patch-file" ],  default="vtu", help="Output format" )
  parser.add_argument("--dir",                       dest="dir",                          default=".", help="Output directory" )
  parser.add_argument("-v", "--verbose",             dest="verbose",                      action="store_true",  default = False, help="Run in a chatty mode" )
  parser.add_argument(                               dest="filename", help="Input file name" )
  parser.add_argument("-start", "--start-snapshot-index", dest="start", type=int,
        help="set the snapshot number where the merge starts(all snapshot before it will be ignore), default is 0 (start from beginning)",
        default=0,)
  parser.add_argument("-end", "--end-snapshot-index", dest="end", type=int,
        help="set the snapshot number where the merge ends(all snapshot after it will be ignore), default is -1 (end at the last)",
        default=-1,)
  args = parser.parse_args()

  if not os.path.exists(args.filename):
    print("Error, specified input file '{}' does not exist, exiting...". format(args.filename))
    sys.exit(1)

  total_snapshot_number=0
  with open(args.filename, "r") as metafile:
      for line in metafile.readlines():
        if "timestamp" in line:
              total_snapshot_number+=1

  if ( (args.start>args.end and args.end!=-1) or args.start<0 or args.end<-1):
      print("Error, specified snapshot index is wrong, please check.")
      sys.exit(1)
  elif args.end==-1:
      start_snapshot=args.start
      end_snapshot=total_snapshot_number-1 #processing all snapshot if no end snapshot is specified.
  elif args.end>(total_snapshot_number-1):
      start_snapshot=args.start
      end_snapshot=total_snapshot_number-1
      print("Warning, specified end snapshot index exceed the totoal number, reset to the last snapshot.")
  else:
      start_snapshot=args.start
      end_snapshot=args.end

  print("==============================================================")
  print("The meta file include", total_snapshot_number, "snapshots, processing snapshot", start_snapshot, "to", end_snapshot)


  visualiser = None
  if args.type=="display":
    visualiser = peano4.visualisation.output.Interactive(file_name=args.filename, verbose=args.verbose )
  if args.type=="vtu":
    visualiser = peano4.visualisation.output.VTU_legacy( file_name=args.filename,
                                     output_directory=args.dir, verbose=args.verbose, start=start_snapshot, end=end_snapshot)
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
  if args.shrink_cells!=1.0:
    print( "add shrink patches filter" )
    visualiser.append_filter( peano4.visualisation.filters.ShrinkPatches(args.shrink_cells,verbose=args.verbose), False )

  visualiser.display()
