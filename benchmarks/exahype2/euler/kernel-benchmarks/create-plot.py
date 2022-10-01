import os, sys
import argparse
import matplotlib.pyplot as plt

print("""

ExaHyPE 2 - Kernel Benchmarks Plotting

@author 2022 Tobias Weinzierl, Mario Wille

""")

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Kernel Benchmarks Plotting Script')
parser.add_argument("-d",   "--dim",               dest="dim",            type=int, default=2, help="Number of space dimensions")
parser.add_argument("-ps",  "--patch-size",        dest="patch_size",     type=int, default=17, help="Number of finite volumes per axis (dimension) per patch")
parser.add_argument("-f",   "--file",              dest="file",           default="", help="Specify to convert an output file into a plot.")
args = parser.parse_args()

algorithms = [
  "patch-wise",
  "batched",
]
data_structures = [
  "AoSoA",
  "AoS",
  "SoA",
  "in-situ"
]
implementations  = [
  #"heap,",
  "omp,",
]
machines = [
  #"host",
  "device 0",
]

for machine in machines:
  for implementation in implementations:
    plt.clf()
    added_data = False
    print( "Process machine {} and implementation {}".format(machine,implementation) )
    plotted_ticks = False
    for algorithm in algorithms:
      for data_structure in data_structures:
        input_file = open( args.file, "r" )
        x_labels = []
        y_values = []
        for line in input_file:
          if "Number of patches fused:" in line:
            x_labels.append( int(line.split(":")[-1]) )
          new_measurement = "completed measurements for stateless kernels" in line
          new_measurement = new_measurement and algorithm in line
          if data_structure=="SoA" or data_structure=="AoS":
            new_measurement = new_measurement and data_structure in line
            new_measurement = new_measurement and not "AoSoA" in line
          else:
            new_measurement = new_measurement and data_structure in line
          new_measurement = new_measurement and machine in line
          new_measurement = new_measurement and implementation in line
          if new_measurement:
            y_values.append( float(line.split("/")[-1]) ) 
        if len(y_values) > 0: # and len(x_values)==len(y_values)
          bar_width = 1.0/(len(algorithms)*len(data_structures)+1)
          linearised_index = algorithms.index(algorithm) * len(data_structures) + data_structures.index(data_structure)
          x_values = [i+linearised_index*bar_width for i in range(0,len(y_values))]
          if len(x_labels)>len(y_values):
            print( "Shorten x range for metric {} x {}".format(algorithm,data_structure) )
            x_values = x_values[0:len(y_values)]
          if len(x_labels)<len(y_values):
            raise Exception( "Parser error for {}x{} as x-len is {} vs y-len {}".format(algorithm,data_structure,len(x_labels),len(y_values)) )
          plt.bar( x_values, y_values, width=bar_width, label=algorithm + ", " + data_structure )
          added_data = True 
        if len(y_values)>0 and not plotted_ticks:
          plotted_ticks = True
          plt.xticks(range(0,len(x_labels)), x_labels)
      plt.yscale( "log" )
      plt.legend()
      plt.xlabel( "Assembled patches")
      plt.ylabel( "Time per FV update [s]")
      plt.title( "Size per patch: $ {}^{} $".format( args.patch_size, args.dim) )
      if added_data:
        plt.savefig( args.file.replace(".","_") + "_" + machine.replace(" ","_").replace(",","") + "_" + implementation.replace(" ","_").replace(",","") + ".pdf" )
      else:
        print( "No data found for {} x {}".format(machine,implementation) ) 

print("Done")
