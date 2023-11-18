#
# You might have to add . to the PYTHONPATH
#
import matplotlib.pyplot as plt
import argparse
import tarfile
import os

from noh import Choices

Symbols = ["-o", "-s", "-v", "-^"]


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

    plt.clf()

    for archive_file in args.file:
        print("Study file {}".format(archive_file))
        tar = tarfile.open(archive_file, "r:gz")
        data_files = tar.getnames()

        SubChoices = Choices[0 : 12 + 1]
        data_points = []
        for choice in SubChoices:
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

    labels_on_x_axis = [
        x.replace(".", "\n").replace("domain-decomposition", "") for x in SubChoices
    ]

    plt.xticks(
        range(0, len(SubChoices)), labels_on_x_axis, rotation="vertical", fontsize=6
    )
    plt.ylabel("time [t]=s")
    plt.title("Domain Decomposition")
    plt.legend()
    plt.yscale("log")
    plt.subplots_adjust(bottom=0.3)
    print("Write to file {}.pdf and {}.png".format(args.output, args.output))
    plt.savefig(args.output + ".pdf")
    plt.savefig(args.output + ".png")
