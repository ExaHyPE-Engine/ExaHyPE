#
# You might have to add . to the PYTHONPATH
#
import matplotlib.pyplot as plt
import argparse
import tarfile
import os

from noh import Choices


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="SPH comparison of different implementation variants")
    parser.add_argument("file", help="Filename of the file to parse. You can specify multiple files.")
    args = parser.parse_args()

    tar = tarfile.open( args.file, "r:gz" )
    data_files = tar.getnames()

    SubChoices  = Choices[0:12]
    data_points = []
    for choice in SubChoices:
        print( "study choice/variant {}".format(choice) )
        for file in data_files:
            if choice + ".out" in file:
                print( "read file {}".format(file) )
                time_stepping_cost = 0
                tar.extract( file )
                input_file = open(file,"r")
                for line in input_file:
                    if "time stepping:" in line: 
                        time_stepping_cost = float(line.split( "time stepping:" )[1].split("s")[0])
                print( "cost is {}s".format(time_stepping_cost) )
                data_points.append( time_stepping_cost )
                os.remove( file )
                        
    labels_on_x_axis = [x.replace(".","\n").replace( "domain-decomposition", "" ) for x in SubChoices]
                        
    plt.clf()
    plt.plot( data_points )
    plt.xticks( range(0,len(SubChoices)), labels_on_x_axis, rotation='vertical', fontsize=6)
    plt.ylabel( "time [t]=s")
    plt.title( "Domain Decomposition" )
    plt.subplots_adjust(bottom=0.3)
    plt.savefig( args.file + ".pdf" )
    plt.savefig( args.file + ".png" )
