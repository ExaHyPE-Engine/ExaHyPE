# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import argparse
import tarfile
import os
import exahype2.postprocessing

import matplotlib.pyplot as plt


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='ExaHyPE 2 scaling plotter')
  parser.add_argument("file",   help="filename of the file to parse (should be a tar.gz file)")
  parser.add_argument("-v",   "--verbose", dest="verbose", help="increase output verbosity",   action="store_true", default=False )
  parser.add_argument("-t",   "--type",    dest="type",    help="Pick implementation variant", choices=["time-step-size"], required=True  )
  parser.add_argument("-o",   "--output",  dest="output",  help="Name of output file",         default="output"  )
  args = parser.parse_args()
  
  performance_data = exahype2.postprocessing.PerformanceData( args.file, verbose=args.verbose )
  
  plt.clf()
  if args.type == "time-step-size":
    exahype2.postprocessing.plot_time_step_size_per_time_step( performance_data )
 
  #exahype2.postprocessing.plot_pie_chart_over_simulation_phases(performance_data)

  plt.savefig( args.output + ".pdf" )
  plt.savefig( args.output + ".png" )
