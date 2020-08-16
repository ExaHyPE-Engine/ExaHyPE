# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import argparse
import tarfile
import os
import exahype2


import matplotlib.pyplot as plt


Colors = [ "#aa0000", "#00aa00", "#0000aa", "#797900", "#790079", "#007979", "#ababab" ]
#
# Number of symbols should differ from colours by one
#
Symbols = [ "o", "s", "<", ">" , "v", "^" ]


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='ExaHyPE 2 scaling plotter')
  parser.add_argument("file",   help="filename of the file to parse (should be a tar.gz file)")
  parser.add_argument("-v", "--verbose", help="increase output verbosity", action="store_true" )
  parser.add_argument("--grid-construction", dest="grid_construction", help="plot grid construction time, too", action="store_true" )
  parser.add_argument("--last-iteration", dest="last_iteration", help="measure only the last iteration", action="store_true" )
  parser.add_argument("--pattern", dest="file_pattern", help="define pattern that has to be in the filename. If you have multiple data sets, separate them via a |", default="")
  parser.add_argument("--max-cores-per-rank", dest="max_cores_per_rank", type=int, help="max. number of cores per rank", default=0)
  parser.add_argument("--log", dest="log", help="plot with logarithmic axes", action="store_true" )
  parser.add_argument("--plot-efficiency", dest="plot_efficiency", help="Don't plot raw times but efficiency", action="store_true" )
  parser.add_argument("--group-measurements", type=int, default=0, dest="group_measurements", help="Number of consecutive measurements that belong together" )
  args = parser.parse_args()
  
  open_mode = ""  
  if args.file.endswith( "tar.gz"):
    open_mode = "r:gz"
  elif args.file.endswith( "tar" ):
    open_mode = "r:"
  else:
    print( "Warning: passed file should end with tar.gz or tar" )

  tar = tarfile.open( args.file, "r:gz" )
  data_files = tar.getnames()

  plt.clf()
  
  different_file_patterns = [ "" ]
  if args.file_pattern!="":
    different_file_patterns = args.file_pattern.split( "|" )

  color_counter = 0
  
  single_node_time = -1
  max_nodes        = -1
  
  for current_file_pattern in different_file_patterns:  
    data_points = []
    for data_file in data_files:
      if current_file_pattern in data_file:
        tar.extract( data_file )
        new_data = exahype2.postprocessing.PerformanceDataPoint(data_file)
        if args.verbose:
          print( "parsed " + data_file + ": " + str(new_data) )
        if new_data.valid:
          data_points.append( new_data ) 
        os.remove( data_file )
      #elif args.verbose:
      #  print( "skip file " + data_file + " as it does not fit to pattern " + current_file_pattern )

    #if args.grid_construction:
    # -- file type
    #  (x_data, y_data) = exahype2.postprocessing.extract_grid_construction_times( data_points )    
    #  plt.plot( x_data, y_data, label="total grid construction (incl. initial lb)" )

    (x_data, y_data) = exahype2.postprocessing.extract_times_per_step( data_points, args.last_iteration, args.max_cores_per_rank )    
    
    if len(x_data)==1:
      print( "data set (pattern " + current_file_pattern + ") hosts only one entry: assume this is the baseline" )
      single_node_time = y_data[0]  
      if not args.plot_efficiency:
        plt.plot( x_data, y_data, ":", color="#000000", label="linear trend" )  
     
    if len(x_data)>0:
      max_nodes = max(max_nodes,x_data[-1])
    
    if args.plot_efficiency:
      normalised_fasted_time = y_data[0] * x_data[0]
      for i in range(0,len(x_data)):
        y_data[i] = normalised_fasted_time / y_data[i] / x_data[i]
      if args.max_cores_per_rank>0:
        y_data = [y/float(args.max_cores_per_rank) for y in y_data]
      y_data = [ min(y,1.1) for y in y_data]
      
    symbol = "-" + Symbols[ different_file_patterns.index(current_file_pattern) % len(Symbols) ]
    my_markevery = 0.9

    my_color = Colors[ color_counter % len(Colors) ]
    color_counter += 1

    add_entry_to_legend = True
    if args.group_measurements>0:
      add_entry_to_legend = different_file_patterns.index(current_file_pattern) < args.group_measurements \
                         or different_file_patterns.index(current_file_pattern) % args.group_measurements == 0
      symbol = "-" + Symbols[ different_file_patterns.index(current_file_pattern) % args.group_measurements ]
      my_color = Colors[ int(different_file_patterns.index(current_file_pattern) / args.group_measurements) ]
      
       
    if len(different_file_patterns)==0:
      plt.plot( x_data, y_data, symbol, label="time per time step", markevery=my_markevery )
    elif add_entry_to_legend:
      if different_file_patterns.index(current_file_pattern)==0:
        plt.plot( x_data, y_data, symbol, label="time per time step, " + str(current_file_pattern), color=my_color, markevery=my_markevery )
      else:
        plt.plot( x_data, y_data, symbol, label=str(current_file_pattern), color=my_color, markevery=my_markevery )
    else:
      plt.plot( x_data, y_data, symbol, color=my_color, markevery=my_markevery )

  #
  # Label alread plotted before
  #
  if single_node_time>0 and not args.plot_efficiency:
    x_data = range(1,max_nodes)
    y_data = [single_node_time/i for i in x_data]
    plt.plot( x_data, y_data, ":", color="#000000" )  

  
  plt.xlabel( "Ranks" )
  if args.plot_efficiency:
    plt.ylabel( "Efficiency" )
  else:
    plt.ylabel( "Time [t]=s" )
  if args.log:
    plt.xscale( "log", basex=2 )
    plt.yscale( "log" )
  xtics   = [ 1 ]
  xlabels = [ "1" ]
  while xtics[-1]<max_nodes:
    xtics.append( xtics[-1]*2 )
    xlabels.append( str(xtics[-1]) )
  plt.xticks( xtics, xlabels )
  plt.legend()
  filename = args.file.replace( ".tar.gz", "").replace( ".tar", "" )
  plt.savefig( filename + ".pdf" )
  plt.savefig( filename + ".png" )
  
  tar.close()
