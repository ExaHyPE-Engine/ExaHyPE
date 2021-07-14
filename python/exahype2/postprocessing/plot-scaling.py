# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import argparse
import tarfile
import os
import exahype2.postprocessing


import matplotlib.pyplot as plt


Colors = [ "#aa0000", "#00aa00", "#0000aa", "#797900", "#790079", "#007979", "#ababab" ]
#
# Number of symbols should differ from colours by one
#
Symbols = [ "o", "s", "<", ">" , "v", "^" ]



def visualise( data_points, symbol_counter, colour_counter):
    global single_node_time 
    global max_x        
    global max_time         
    
    (x_data, y_data) = exahype2.postprocessing.extract_times_per_step( data_points, args.max_cores_per_rank )    
     
    if len(x_data)>0:
      max_x = max(max_x,x_data[-1])
    
    if args.plot_efficiency:
      normalised_fasted_time = y_data[0] * x_data[0]
      for i in range(0,len(x_data)):
        y_data[i] = normalised_fasted_time / y_data[i] / x_data[i]
      if args.max_cores_per_rank>0:
        y_data = [y/float(args.max_cores_per_rank) for y in y_data]
      y_data = [ min(y,1.1) for y in y_data]
      
    symbol    = "-" + Symbols[ symbol_counter  % len(Symbols) ]
    my_color  = Colors[ colour_counter % len(Colors) ]
    # my_markevery = ( 0.1 + 0.8 * (colour_counter+1/len(Colors) )**3, 0.1 * 0.8 * symbol_counter / len(Symbols) )
    # First entry is first marker (start to to count with 0), second is spacing
    my_markevery = ( symbol_counter % len(Symbols) % (len(x_data)+1), int( len(x_data)/8 )+1 )
    
    if len(y_data)>0:
      max_time = max(y_data[0], max_time)
    
    if args.labels=="":
      plt.plot( x_data, y_data, symbol, color=my_color, markevery=my_markevery )
    else:
      try:
        my_label = args.labels.split(",")[ args.file.split(",").index(file) ]
      except:
        raise Exception( "Unable to extract " + str(args.file.split(",").index(file)) + "th entry from " + args.labels + ": " + str(args.labels.split(",")) )
      plt.plot( x_data, y_data, symbol, label=my_label, color=my_color, markevery=my_markevery )
    


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description="""
ExaHyPE 2 scaling plotter:

A generic script to create speedup plots.

""")
  parser.add_argument("file",   help="filename of the file to parse (should be a tar.gz file or a directory). If you have multiple files, seperate them with a comma")
  parser.add_argument("-v",                  "--verbose", help="increase output verbosity", action="store_true" )
  parser.add_argument("--max-cores-per-rank", dest="max_cores_per_rank", type=int, help="max number of cores per rank (pick 0 if you have only one core count per rank, pick -1 if you want to plot single-node data)", default=0)
  parser.add_argument("--group-data",         dest="group_data",      help="group k consecutive measurements as one piece of data, i.e. use similar styles (default=0, i.e. off)", default=0, type=int )
  parser.add_argument("--log-x",              dest="log_x",           help="plot with logarithmic axes", action="store_true" )
  parser.add_argument("--log-y",              dest="log_y",           help="plot with logarithmic axes", action="store_true" )
  parser.add_argument("--plot-efficiency",    dest="plot_efficiency", help="Don't plot raw times but efficiency", action="store_true" )
  parser.add_argument("--labels",             dest="labels",          help="Plot labels. You can use $...$ to use the math mode, but you will have to escape the dollars with a \\", default="" )
  parser.add_argument("--output",             dest="output",          help="output file prefix (file name extension is added automatically)", default="" )
  args = parser.parse_args()


  single_node_time = -1
  max_x        = -1
  max_time         = -1
  
  plt.clf()

  symbol_counter = 0
  colour_counter = 0
    
  for file in args.file.split(","):
    if os.path.isdir(file):
      print( "Parse file " + file )
      data_points = []
      for data_file in os.listdir(file):
        print( "========================================================================")
        print( file + "/" + data_file + " from " + file )
        print( "========================================================================")
        new_data = exahype2.postprocessing.PerformanceData(file + "/" + data_file, args.verbose)
        if new_data.valid:
          data_points.append( new_data ) 
      visualise( data_points, symbol_counter, colour_counter )
    elif file.endswith( "tar") or file.endswith("tar.gz"):
      print( "Parse archive " + file )
      open_mode = ""  
      if file.endswith( "tar.gz"):
        open_mode = "r:gz"
      elif file.endswith( "tar" ):
        open_mode = "r:"

      tar = None
      try:
        tar = tarfile.open( file, "r:gz" )
        data_files = tar.getnames()
    
        data_points = []
        for data_file in data_files:
          print( "========================================================================")
          print( data_file + " from " + file )
          print( "========================================================================")
          tar.extract( data_file )
          new_data = exahype2.postprocessing.PerformanceData(data_file, args.verbose)
          if new_data.valid:
            data_points.append( new_data ) 
          os.remove( data_file )
          
        visualise( data_points, symbol_counter, colour_counter )
      except Exception as e:
        print( "Error: " + str(e) )
        tar = None
      if tar!=None:   
        tar.close()
    else:
      print( file + " is neither a directory or an archive" )

    symbol_counter += 1
    if args.group_data>0:
      if symbol_counter>=args.group_data:
        symbol_counter=0
        colour_counter += 1
    else:
      colour_counter += 1

  if args.plot_efficiency:
    plt.ylabel( "Efficiency" )
  else:
    plt.ylabel( "Time per time step [t]=s" )
  if args.log_x:
    plt.xscale( "log", base=2 )
  if args.log_y:
    plt.yscale( "log", base=2 )

  
  if args.max_cores_per_rank<0:
    plt.xlabel( "Cores" )
  else:
    plt.xlabel( "Ranks" )

  xtics   = [ 1 ]
  xlabels = [ "1" ]
  while xtics[-1]<max_x:
    xtics.append( xtics[-1]*2 )
    xlabels.append( str(xtics[-1]) )
  plt.xticks( xtics, xlabels )
  
  plt.legend()
  filename = args.file.replace( ".tar.gz", "").replace( ".tar", "" )
  output_file_name = args.output
  if output_file_name=="":
      output_file_name = filename
  plt.savefig( output_file_name + ".pdf" )
  plt.savefig( output_file_name + ".png" )
