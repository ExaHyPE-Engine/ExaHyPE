import os
import sys
import argparse

from enum import Enum

import matplotlib.pyplot as plt
import numpy as np



Markers = [ "o", "s", "<", ">", "^", "v", "D" ]
Colours = [ "#ff0000",
            "#00ff00",
            "#0000ff",
            "#abab00",
            "#ab00ab",
            "#00abab",
            "#787878",
            "#ffab50",
            "#ff50ab",
            "#abff50",
            "#50ffab",
            "#50abff",
            "#ab50ff",
            ]


Devices         = ["host", "device 0"]
CallSemantics   = ["functors", "static"]
DataFlow        = ["patch-wise", "batched", "task-graph" ]
Parallelisation = ["no parallelism", "parallel for", "subtasks", "map", "managed", "usm", "copy"]


def parse(device, call_semantics, data_flow, parallelisation):
  """"
  Parse file and search for entries for particular argument combination
  
  We return two lists: The first one gives the total throughput per measurement
  while the second one gives the throughput for one kernel.
  
  
  """
  throughput_total   = []
  throughput_kernel  = []
  best_case_label    = ""
  best_case_position = -1
  best_case_label_distance_from_coordinate_system_origin_squared = 0
  current_number_of_threads = "xxx"
  current_number_of_patches = "xxx"

  file = open( args.file, "r" )
  for line in file:
    if "Number of threads launching compute kernels" in line:
      current_number_of_threads = line.split(":")[-1].strip()
    if "Number of patches per thread/compute kernel launch" in line:
      current_number_of_patches = line.split(":")[-1].strip()
    if device in line and call_semantics in line and data_flow in line and parallelisation in line:
      current_throughput_total  = 1.0/float(line.split("|")[4])
      current_throughput_kernel = 1.0/float(line.split("|")[2])
      distance = current_throughput_total*current_throughput_total + 10.0*current_throughput_kernel*current_throughput_kernel
      if len(throughput_kernel)>0 and distance > best_case_label_distance_from_coordinate_system_origin_squared:
        best_case_label_distance_from_coordinate_system_origin_squared = distance
        best_case_position = len(throughput_total)
        best_case_label    = current_number_of_threads + "/" + current_number_of_patches
      throughput_total.append(  current_throughput_total )
      throughput_kernel.append( current_throughput_kernel )
  return throughput_total, throughput_kernel, best_case_label, best_case_position


parser = argparse.ArgumentParser(description='ExaHyPE 2 - Kernel Benchmarks Scaling - Plotting Script')
parser.add_argument( dest="file", help="File to parse" )
args = parser.parse_args()

for device in Devices:
  print( "Create plot for {}".format(device) )
  print( "============================" )
  plt.clf()
  marker_counter = 0
  for call_semantics in CallSemantics:
    if device == "device 0": call_semantics = ""
    for data_flow in DataFlow:
      for parallelisation in Parallelisation:
        throughput_total, throughput_kernel, best_case_label, best_case_position = parse(device, call_semantics, data_flow, parallelisation)
        assert len(throughput_total)==len(throughput_kernel), "data cardinalities {} vs {} with {}".format( len(throughput_total), len(throughput_kernel), best_case_label )
        if len(throughput_total)>0:
          print( "data found for {}, {}, {}".format(call_semantics, data_flow, parallelisation) )
          plt.scatter( throughput_total,
                       throughput_kernel,
                       c      = Colours[marker_counter % len(Colours)],
                       marker = Markers[marker_counter % len(Markers)],
                       alpha  = 0.5
                       )
          plt.scatter( throughput_total[best_case_position],
                       throughput_kernel[best_case_position],
                       c      = Colours[marker_counter % len(Colours)],
                       marker = Markers[marker_counter % len(Markers)],
                       label  = call_semantics + ", " + data_flow + ", " + parallelisation,
                       )
          plt.text( throughput_total[best_case_position],
                    throughput_kernel[best_case_position],
                    best_case_label,
                    c      = Colours[marker_counter % len(Colours)],
                    )
          marker_counter += 1

  plt.title( device + " (launching threads/patches per launch)")
  plt.legend()
  plt.xlabel("Total throughput (dofs/s)")
  plt.ylabel("Throughput for one kernel (dofs/s)")

  output = args.file + "-" + device
  #plt.savefig( output + ".png" )
  plt.savefig( output + ".pdf" )

  plt.xscale('log')
  plt.yscale('log')
  #plt.savefig( output + "-log.png" )
  plt.savefig( output + "-log.pdf" )



