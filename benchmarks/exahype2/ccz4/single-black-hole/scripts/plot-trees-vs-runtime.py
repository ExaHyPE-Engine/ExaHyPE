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

    fig, ax1 = plt.subplots(figsize=(10, 6))    
    
    runtime = {}
    max_local_cells_per_tree = {}
    min_local_cells_per_tree = {}
    max_virtual_cells_per_tree = {}
    min_virtual_cells_per_tree = {}
    
    tar = tarfile.open(args.file, open_mode)
    data_files = tar.getnames()
    for data_file in data_files:
        tar.extract(data_file)
        data = OutputFile( data_file, args.verbose )
        data.parse()
        
        rank = 0
        trees = max( data.trees_per_snapshot(rank) )

        if data.is_valid():
          runtime[ trees ] = data.runtime
          max_local_cells_per_tree[ trees ] = data.max_local_cells_per_snapshot(rank)[-1]
          min_local_cells_per_tree[ trees ] = data.min_local_cells_per_snapshot(rank)[-1]
          max_virtual_cells_per_tree[ trees ] = data.max_virtual_cells_per_snapshot(rank)[-1]
          min_virtual_cells_per_tree[ trees ] = data.min_virtual_cells_per_snapshot(rank)[-1]
          os.remove(data_file)
 
    bar_width = 1.0/6.0   
    tree_keys = sorted( runtime.keys() )
    x_data = range(0,len(tree_keys))
    
    ax1.set_xlabel('Number of trees per rank')
   
    color_runtime = "tab:blue"
    ax1.set_ylabel('Runtime', color=color_runtime)
    ax1.bar( [x-2.5*bar_width for x in x_data],
             [runtime[x] for x in tree_keys],
             width=bar_width,
             label="Runtime",
             color=color_runtime,
             )

    ax1.tick_params(axis='y', labelcolor=color_runtime)
    ax1.set_xticks(x_data)
    ax1.set_xticklabels( tree_keys )
    
    ax2 = ax1.twinx()
    
    color_cells = 'tab:red'
    ax2.set_ylabel('Number of cells', color=color_cells)
    ax2.bar( [x-1.5*bar_width for x in x_data],
             [max_local_cells_per_tree[x] for x in tree_keys],
             width=bar_width,
             label="Max local cells",
             color=color_cells,
             )
             
    color_cells = 'tab:purple'
    ax2.bar( [x-0.5*bar_width for x in x_data],
             [min_local_cells_per_tree[x] for x in tree_keys],
             width=bar_width,
             label="Min local cells",
             color=color_cells,
             )
             
    color_cells = 'tab:green'
    ax2.set_ylabel('Number of cells', color=color_cells)
    ax2.bar( [x+0.5*bar_width for x in x_data],
             [max_virtual_cells_per_tree[x] for x in tree_keys],
             width=bar_width,
             label="Max virtual cells",
             color=color_cells,
             )
             
    color_cells = 'tab:olive'
    ax2.bar( [x+1.5*bar_width for x in x_data],
             [min_virtual_cells_per_tree[x] for x in tree_keys],
             width=bar_width,
             label="Min virtual cells",
             color=color_cells,
             )

    plt.legend()
    plt.savefig( args.output + ".pdf" )
    plt.savefig( args.output + ".png" )
