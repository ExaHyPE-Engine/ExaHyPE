import matplotlib.pyplot as plt
import os
import sys
import re
import argparse
import numpy as np

kernel_variants = ["batched", "patch-wise", "task-graph"]

data_structures = {
	"AoSoA": "AoSoA",
	"AoS": "AoS",
	"SoA": "SoA",
}

reduction = ["with reduction", "no reduction"]

memory_model = ["copy", "managed", "usm"]

parser = argparse.ArgumentParser(description="ExaHyPE 2 - SYCL Kernel Benchmarks Plotting Script")
parser.add_argument("-f", "--files", dest="files", required=True, nargs='*',
					help="*REQUIRED* Files to extract data from")
parser.add_argument("-ds", "--data-structure", dest="data_structure", choices=data_structures.keys(), default="AoS",
					help="Chooses which temporary data structure to plot for, default is AoS")
parser.add_argument("-ps", "--patch-size", dest="patch_size", default=[4, 6, 8], nargs='*',
					help="Specify how many volumes are in patch, default is [4, 6, 8]")
parser.add_argument("-dn", "--device-name", dest="device_name",
					help="Device name to put in filename")
parser.add_argument("-d", "--dimension", dest="dimension", default="2",
					help="Dimension used in experiment, default is 2")
args = parser.parse_args()

markers_filled = [
	'D',
	'^',
	'o',
	's',
	'p',
	'x',
	'H',
	'h'
]

markers_unfilled = ['+', 'x', '1', '2', '3', '4']

if args.files:
	cmap = plt.get_cmap("tab10")
	input_files = [open(i, "r") for i in args.files]
	if args.device_name.upper() == "A100":
		memory_model.remove("usm")
	for kernel in kernel_variants:
		marker_counter = 0
		reduction_counter = 0
		memory_counter = 0
		plt.cla()
		for memory in memory_model:
			colour_counter = 0
			for file_num in range(len(args.files)):
				for reduce in reduction:
					input_files[file_num].seek(0)
					kernel_time = []
					number_of_patches = []
					for line in input_files[file_num]:
						if "Number of threads launching compute kernels: 2" in line:
							break
						if "Number of patches" in line:
							number_of_patches.append(int(line.split(":")[-1].strip()))
						if "device 0" in line and kernel in line and memory in line and line.split(",")[
							2].strip() == args.data_structure and reduce in line:
							kernel_time.append(float(line.split("|")[2]))
					if len(kernel_time) != 0:
						if reduce == "with reduction":
							plt.plot(number_of_patches[:len(kernel_time)], kernel_time, linewidth=1.5,
									 marker=str(markers_filled[marker_counter]),
									 markevery=(memory_counter, len(memory_model)),
									 markersize=6, color=cmap(colour_counter), fillstyle='none',
									 label=f"{memory}, {reduce}, ps={str(args.patch_size[file_num])}")
						else:
							if reduction_counter == 0:
								plt.plot(number_of_patches[:len(kernel_time)], kernel_time, linestyle='--',
										 linewidth=1.5,
										 marker=str(markers_filled[marker_counter]),
										 markevery=(memory_counter, len(memory_model)),
										 markersize=6, color=cmap(colour_counter),
										 label=f"{memory}, {reduce}, ps={str(args.patch_size[file_num])}")
								reduction_counter += 1
							else:
								plt.plot(number_of_patches[:len(kernel_time)], kernel_time, linestyle='--',
										 linewidth=1.5,
										 marker=str(markers_filled[marker_counter]),
										 markevery=(memory_counter, len(memory_model)),
										 markersize=6, color=cmap(colour_counter))

				colour_counter += 1
			marker_counter += 1
			memory_counter += 1
		plt.legend()
		plt.style.use("ggplot")
		plt.xscale('log', base=2)
		plt.yscale("log")
		if args.device_name.upper() == "A100":
			plt.ylim((5e-9, 1e-5))
		elif args.device_name.upper() == "PVC":
			plt.ylim((1e-8, 1e-4))
		plt.legend(fancybox=True)
		plt.xlabel(r"Number of patches, $T$")
		plt.ylabel("Kernel time per unknown / s")
		plt.grid(color='grey', alpha=0.5, linestyle='dashed', linewidth=0.5)
		#plt.show()
		if args.device_name:
			plt.title(f"{kernel}, {args.data_structure}, d={args.dimension}, {args.device_name}")
			plt.savefig(
				kernel + "-" + args.dimension + "d-" + args.data_structure + "-" + args.device_name + ".pdf",
				bbox_inches='tight')
		else:
			plt.title(f"{kernel}, {args.data_structure}, d={args.dimension}")
			plt.savefig(
				kernel + "-" + args.dimension + "d-" + args.data_structure + ".pdf",
				bbox_inches='tight')
	if args.device_name.upper() == "PVC":
		memory_model.remove("usm")
	for kernel in kernel_variants:
		plt.cla()
		marker_counter = 0
		counter = 0
		memory_counter = 0
		for memory in memory_model:
			if memory == 'usm':
				continue
			else:
				colour_counter = 0
				for file_num in range(len(args.files)):
					input_files[file_num].seek(0)
					number_of_patches = []
					kernel_time = []
					full_time = []
					previous = ""
					for line in input_files[file_num]:
						if "Number of threads launching compute kernels: 2" in line:
							break
						if "Number of patches" in line:
							number_of_patches.append(int(line.split(":")[-1].strip()))
						if "core compute kernel time" in line:
							previous = line
						if "device 0" in line and kernel in line and memory in line and line.split(",")[
							2].strip() == args.data_structure and "with reduction" in line:
							full_time.append(float(line.split("|")[1]))
							time = re.search('core compute kernel time: (.*) \(', previous)
							kernel_time.append(float(time.group(1)))
					if len(kernel_time) != 0 and len(full_time) != 0:
						plt.plot(number_of_patches[:len(full_time)], full_time, linewidth=1.5,
								 marker=str(markers_filled[marker_counter]),
								 markevery=(memory_counter, len(memory_model)),
								 markersize=6, color=cmap(colour_counter), fillstyle='none',
								 label=f"{memory}, full runtime, ps={str(args.patch_size[file_num])}")
						if counter == 0:
							plt.plot(number_of_patches[:len(kernel_time)], kernel_time, linewidth=1.5,
									 marker=str(markers_filled[marker_counter]), linestyle='dashed',
									 markersize=6, color=cmap(colour_counter),
									 markevery=(memory_counter, len(memory_model)),
									 label=f"{memory}, kernel time, ps={str(args.patch_size[file_num])}")
							counter += 1
						else:
							plt.plot(number_of_patches[:len(kernel_time)], kernel_time, linewidth=1.5,
									 marker=str(markers_filled[marker_counter]), linestyle='dashed',
									 markevery=(memory_counter, len(memory_model)),
									 markersize=6, color=cmap(colour_counter))
					colour_counter += 1
				marker_counter += 1
				memory_counter += 1
		plt.legend()
		plt.style.use("ggplot")
		plt.xscale('log', base=2)
		plt.yscale("log")
		if args.device_name.upper() == "A100":
			plt.ylim(3e-5, 1e-1)
		elif args.device_name.upper() == "PVC":
			plt.ylim()
		plt.legend(fancybox=True)
		plt.xlabel(r"Number of patches, $T$")
		plt.ylabel("Time / s")
		plt.grid(color='grey', alpha=0.5, linestyle='dashed', linewidth=0.5)
		if args.device_name:
			plt.title(f"{kernel}, {args.data_structure}, d={args.dimension}, {args.device_name}")
			plt.savefig("runtime-" +
						kernel + "-" + args.dimension + "d-" + args.data_structure + "-" + args.device_name + ".pdf",
						bbox_inches='tight')
		else:
			plt.title(f"{kernel}, {args.data_structure}, d={args.dimension}")
			plt.savefig("runtime-" +
						kernel + "-" + args.dimension + "d-" + args.data_structure + ".pdf",
						bbox_inches='tight')
#plt.show()
