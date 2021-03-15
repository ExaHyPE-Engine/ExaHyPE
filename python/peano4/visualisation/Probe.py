import argparse
import csv
import os
import sys

import natsort

from peano4.visualisation.OutputFileParser import OutputFileParser

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Peano 4 - probe a specific '
                                               'point in a set of Peano patch files')
  parser.add_argument('--input-file',     dest='filepath',       help='File path to a Peano patch file')
  parser.add_argument('--input-dir',      dest='directory_path', help='Directory file path that stores Peano patch files')
  parser.add_argument('-x',               dest='x_axis',         help='coordinate: x-axis', required=True, type=float)
  parser.add_argument('-y',               dest='y_axis',         help='coordinate: y-axis', required=True, type=float)
  parser.add_argument('-z',               dest='z_axis',         help='coordinate: z-axis', type=float)
  parser.add_argument('--set-identifier', dest='set_identifier', help='Name of the set of unknowns we want to extract', default='')
  args = parser.parse_args()

  if args.filepath:
    if not os.path.exists(args.filepath):
      print("Error, specified input file '{}' does not exist, exiting...".format(args.filepath))
      sys.exit(1)

    ofparser = OutputFileParser(args.filepath, args.set_identifier, 0)

    with open('probe_output.csv', 'w') as csv_file:
      writer = csv.writer(csv_file, delimiter=',')
      probe_result = ofparser.probe(args.x_axis, args.y_axis, args.z_axis)
      if probe_result:  
        writer.writerow(probe_result)

  if args.directory_path:
    if not os.path.exists(args.directory_path):
      print("Error, specified input directory '{}' does not exist, exiting...".format(args.directory_path))
      sys.exit(1)

    with open('probe_output.csv', 'w') as csv_file:
      writer = csv.writer(csv_file, delimiter=',')
      for file in natsort.natsorted(os.listdir(args.directory_path)):
        ofparser = OutputFileParser(os.path.join(args.directory_path, file), args.set_identifier, 0)
        probe_result = ofparser.probe(args.x_axis, args.y_axis, args.z_axis)
        if probe_result:
          writer.writerow(probe_result)


