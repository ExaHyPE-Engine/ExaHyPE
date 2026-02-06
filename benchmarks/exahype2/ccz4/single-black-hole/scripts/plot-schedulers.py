# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import tarfile
import os


from OutputFile import OutputFile

import matplotlib.pyplot as plt


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="""
ExaHyPE 2 scaling plotter:

A generic script to create speedup plots.

"""
    )
    parser.add_argument(
        "file",
        help="filename of the file to parse. Should be a tar.gz file",
    )
    parser.add_argument(
        "--output",
        dest="output",
        help="output file",
        default="output",
    )
    parser.add_argument(
        "-v", "--verbose", help="increase output verbosity", action="store_true"
    )
    args = parser.parse_args()

    open_mode = ""
    if args.file.endswith("tar.gz"):
        open_mode = "r:gz"
    elif args.file.endswith("tar"):
        open_mode = "r:"

    open_mode = "r:gz"

    plt.clf()
    
    Variants = [ "regular, no-tasks",
                 "regular, native",
                 "regular, lazy-evaluation",
                 "regular, timed-execution",
                 "regular, eager-consumer-tasks",
                 "adaptive, no-tasks",
                 "adaptive, native",
                 "adaptive, lazy-evaluation",
                 "adaptive, timed-execution",
                 "adaptive, eager-consumer-tasks",
                ]

    runtimes = [0.0 for x in Variants]                
                 
    tar = tarfile.open(args.file, open_mode)
    data_files = tar.getnames()
    for data_file in data_files:
        tar.extract(data_file)
        
        for variant in Variants:
            # if we split up the keywords, they still have trailing spaces
            keywords = variant.split(",")
            keywords = [x.strip() for x in keywords]
            all_keywords_in_filename = True
            for keyword in keywords:
                found_phrase = keyword in data_file
                all_keywords_in_filename  = all_keywords_in_filename and found_phrase    
            if all_keywords_in_filename:
                index_of_variant = Variants.index(variant)
                if args.verbose:
                    print( "variant {} matches filename {} and will yield bar {}".format(variant,data_file,index_of_variant) )
        
        data = OutputFile( data_file, args.verbose )
        data.parse()

        if data.is_valid():
            runtimes[index_of_variant] = data.runtime
        else:
            print( "file {} hosts no valid data".format(data_file) )
                
        os.remove(data_file)
         
    bar_width = 0.9   
    
    plt.ylabel('Runtime')
    plt.bar( range(0,len(Variants)),
             runtimes,
             width=bar_width,
             label="Runtime",
             )

    for i in range(0,len(Variants)):
        plt.text( i-0.2, 
                  1, 
                  Variants[i].strip().replace(",","\n"), 
                  rotation=-90,
                  )
               
    plt.xticks([],[])
    
    plt.legend()
    plt.savefig( args.output + ".pdf" )
    plt.savefig( args.output + ".png" )
