#
# You might have to add . to the PYTHONPATH
#
import matplotlib.pyplot as plt
import argparse
import tarfile
import os

from noh import (
  Choices,
  ParallelisationVariants,
  Parallelisation_DomainDecomposition,
  StorageVariants,
  SortingVariants,
  AllKernelOptimisationVariants,
  Storage_Scattered,
  BasicKernelOptimisationVariants,
  KernelOptimisation_NoOptimisation,
)

Symbols = ["-o", "-s", "-v", "-^"]


def plot_for_subset_of_variants(choices,
                                tar,
                                plt,
                                ):
    data_files = tar.getnames()
    data_points = []
    for choice in choices:
        print("study choice/variant {}".format(choice))
        for file in data_files:
            if choice + ".out" in file:
                print("read file {}".format(file))
                time_stepping_cost = 0
                tar.extract(file)
                input_file = open(file, "r")
                for line in input_file:
                    if "time stepping:" in line:
                        time_stepping_cost = float(
                             line.split("time stepping:")[1].split("s")[0]
                        )
                print("cost is {}s".format(time_stepping_cost))
                data_points.append(time_stepping_cost)
                os.remove(file)
                
    thread_count = archive_file.split("-threads")[0].split("-")[-1]
    plt.plot(
            data_points,
            Symbols[args.file.index(archive_file)],
            label=thread_count + " threads",
    )


def construct_labels(sub_choices,
                     ):
    """!
    
    You can pass in the subchoices directly, or you can simply pass in some
    strings. The routine takes the points in the identifier and replaces them
    with linebreaks.
    
    """                
    return [
        x.replace(".", "\n") for x in sub_choices
    ]

def add_legend_and_labels(plt,
                          labels_on_x_axis,
                          ):
    """!
    
    Add the labels of the choices as x-ticks and rotate them by 90 degrees. 
    After that, we have to extend the whitespace underneath the plot, as the
    labels otherwise don't fit in.
    
    """                      
    plt.xticks(
        range(0, len(labels_on_x_axis)), labels_on_x_axis, rotation="vertical", fontsize=6
    )
    plt.ylabel("time [t]=s")
    plt.legend()
    plt.subplots_adjust(bottom=0.3)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="SPH comparison of different implementation variants"
    )
    parser.add_argument(
        "file",
        nargs="+",
        help="Filename of the file to parse. You can specify multiple files.",
    )
    parser.add_argument(
        "-o", "--output", dest="output", required=True, help="Output file"
    )
    args = parser.parse_args()


    # ===========================
    # All variants
    # ===========================
    sub_choices = Choices
    plt.clf()
    for archive_file in args.file:
        print("Study file {}".format(archive_file))
        plot_for_subset_of_variants(sub_choices,
                                    tarfile.open(archive_file, "r:gz"),
                                    plt)
    add_legend_and_labels(plt,
                          construct_labels(sub_choices),
                          )
    print("Write to file {}.pdf and {}.png".format(args.output, args.output))
    plt.savefig(args.output + ".pdf")
    plt.savefig(args.output + ".png")
    plt.yscale("log")
    plt.savefig(args.output + "-log.pdf")
    plt.savefig(args.output + "-log.png")


    # ===========================
    # Only domain decomposition
    # ===========================
    sub_choices = [
        parallelisation_variant + "." + storage + "." + sorting + "." + kernel_optimisation
        for parallelisation_variant in [Parallelisation_DomainDecomposition]
        for storage                 in [Storage_Scattered]
        for sorting                 in SortingVariants
        for kernel_optimisation     in BasicKernelOptimisationVariants
    ] + [
        parallelisation_variant + "." + storage + "." + sorting + "." + kernel_optimisation
        for parallelisation_variant in [Parallelisation_DomainDecomposition]
        for storage                 in StorageVariants
        for sorting                 in SortingVariants
        for kernel_optimisation     in AllKernelOptimisationVariants
    ]
    plt.clf()
    for archive_file in args.file:
        print("Study file {}".format(archive_file))
        plot_for_subset_of_variants(sub_choices,
                                    tarfile.open(archive_file, "r:gz"),
                                    plt)
    add_legend_and_labels(plt,
                          construct_labels( [x.replace(Parallelisation_DomainDecomposition + ".","") for x in sub_choices] ),
                          )
    print("Write to file {}.pdf and {}.png".format(args.output, args.output))
    plt.savefig(args.output + "-domain-decomposition.pdf")
    plt.savefig(args.output + "-domain-decomposition.png")
    plt.yscale("log")
    plt.savefig(args.output + "-domain-decomposition-log.pdf")
    plt.savefig(args.output + "-domain-decomposition-log.png")


    # ===========================
    # Without any optimisation
    # ===========================
    sub_choices = [
        parallelisation_variant + "." + storage + "." + sorting + "." + kernel_optimisation
        for parallelisation_variant in [Parallelisation_DomainDecomposition]
        for storage                 in [Storage_Scattered] + StorageVariants
        for sorting                 in SortingVariants
        for kernel_optimisation     in [KernelOptimisation_NoOptimisation]
    ]
    plt.clf()
    for archive_file in args.file:
        print("Study file {}".format(archive_file))
        plot_for_subset_of_variants(sub_choices,
                                    tarfile.open(archive_file, "r:gz"),
                                    plt)
    add_legend_and_labels(plt,
                          construct_labels( [x.replace(Parallelisation_DomainDecomposition + ".","") for x in sub_choices] ),
                          )
    print("Write to file {}.pdf and {}.png".format(args.output, args.output))
    plt.savefig(args.output + "-compare-only-storage.pdf")
    plt.savefig(args.output + "-compare-only-storage.png")
    plt.yscale("log")
    plt.savefig(args.output + "-compare-only-storage-log.pdf")
    plt.savefig(args.output + "-compare-only-storage-log.png")

    
    
    
