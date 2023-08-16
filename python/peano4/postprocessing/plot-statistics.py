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
Peano 4 statistics postprocessing

Plotter for statistics over time. The plotter works only for numerical
data dumped as four tuple over time.

""")
  parser.add_argument("file", help="filename of examine")
  parser.add_argument("-c", 
                      "--columns", 
                      dest="columns", 
                      help="comma-separated list of columns to read (0 equals time stamps and is read always)", 
                      required=True)
  parser.add_argument("-o", 
                      "--output",  
                      dest="output",    
                      help="Pick plot variant", 
                      choices=[ "pdf", "png", "csv" ], 
                      required=True)
  parser.add_argument("-minmax", 
                      "--min-max",  
                      dest="minmax",
                      action="store_true",
                      default=False,    
                      help="Write min-max bars" )
  args = parser.parse_args()
  
  columns = [ int(x) for x in args.columns.split(",") ]
  print( "Extract columns {}".format(columns) )
  
  if args.output=="pdf" or args.output=="png":    
    plt.clf()

  timestamps = []
  values     = []
  min_values = []
  max_values = []
  samples    = []
  for column in columns:
    input_file = open( args.file, "r" )
    metric = None
    for line in input_file:
      if metric==None:
        metric = line.split( "," )[column]
        print( "parse metric {}".format(metric) )
      elif line.strip()!="":
        time_stamp = float(line.split( "," )[0])
        token      = line.split( "," )[column].strip()
        if token!="":
          timestamps.append(time_stamp )
          values.append( float( token.split("(")[1].split("/")[0])  )
          min_values.append( float( token.split("/")[1])  )
          max_values.append( float( token.split("/")[2])  )
          samples.append( float( token.split("/#")[1].split(")")[0])  )
    if args.output=="csv":
      output_file = open( args.file + "-column-" + str(column) + ".csv", "w" )
      for data in zip(timestamps,values,min_values,max_values,samples):
        entries_per_row = 5
        for i in range(0,entries_per_row-1):
          output_file.write( str(data[i]) )
          output_file.write( "," )
        output_file.write( str(data[entries_per_row-1]) )
        output_file.write( "\n" )
    if args.output=="pdf" or args.output=="png":
      assert len(timestamps)==len(values)
      colour = columns.index(column) % len(Colours)
      if args.minmax:
        for i in zip(timestamps,min_values,max_values):
          plt.plot( [i[0],i[0]], [i[1],i[2]], color=Colours[colour] )
      plt.scatter( timestamps, values, color=Colours[colour], alpha=0.5, label=metric )

  if args.output=="pdf":    
    plt.legend()
    plt.savefig( args.file + ".pdf" )
  if args.output=="png":    
    plt.legend()
    plt.savefig( args.file + ".png" )
