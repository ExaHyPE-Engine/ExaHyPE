import matplotlib.pyplot as plt
import os
import sys
import argparse
import numpy as np

kernel_variants = {
	"patch-wise": "patch-wise",
	"batched": "batched",
	"task-graph": "task-graph"
}

data_structures = {
	"AoSoA": "AoSoA",
	"AoS": "AoS",
	"SoA": "SoA",
}

parser = argparse.ArgumentParser(description="ExaHyPE 2 - SYCL Kernel Benchmarks Plotting Script")
parser.add_argument("-f", "--files", dest="files", required=True, nargs='*', help="select files to extract data")
parser.add_argument("-nr", "--files-no-reduction", dest="files_no_reduction", nargs='*',
					help="Select files from experiment without reduction")
parser.add_argument("-kv", "--kernel-variant", dest="kernel_variant", choices=kernel_variants.keys(), default="batched",
					help="Chooses which kernel variant to plot, default is batched")
parser.add_argument("-ds", "--data-structure", dest="data_structure", choices=data_structures.keys(), default="AoS",
					help="Chooses which temporary data structure to plot for, default is AoS")
parser.add_argument("-ts", "--time-steps", dest="time_steps", default=100,
					help="Specify how many cycles gpu had, default is 100")
parser.add_argument("-ps", "--patch-size", dest="patch_size", default=[4, 8, 14], nargs='*',
					help="Specify how many volumes are in patch")
args = parser.parse_args()

markers = [
	'D',
	'*',
	'o',
	's',
	'p',
	'x',
	'H',
	'h'
]

if args.files:
	cmap = plt.get_cmap("tab10")
	fig, ax = plt.subplots()
	input_files = [open(i, "r") for i in args.files]

	kernel_time = {str(i): [] for i in args.patch_size}
	mean_kernel_time = {str(i): [] for i in args.patch_size}
	number_of_patches = {str(i): [] for i in args.patch_size}

	for file_num in range(len(input_files)):
		for line in input_files[file_num]:
			if "Number of patches" in line:
				patch_count = int(line.split(":")[-1])
				number_of_patches[str(args.patch_size[file_num])].append(patch_count)
			if args.kernel_variant == "batched":
				if "Batched kernel execution time" in line:
					kernel_time[str(args.patch_size[file_num])].append(float(line.split(":")[-1]))
			elif args.kernel_variant == "patch-wise":
				if "Patchwise kernel execution time" in line:
					kernel_time[str(args.patch_size[file_num])].append(float(line.split(":")[-1]))
			elif args.kernel_variant == "task-graph":
				if "Task graph kernel execution time" in line:
					kernel_time[str(args.patch_size[file_num])].append(float(line.split(":")[-1]))
		for i in range(len(number_of_patches[str(args.patch_size[file_num])])):
			if args.data_structure == "AoS":
				temp = np.mean(kernel_time[str(args.patch_size[file_num])][
							   i * 3 * int(args.time_steps) + 1: (3 * i + 1) * int(
								   args.time_steps)])  # start from 1 since the first run is a warm up
				mean_kernel_time[str(args.patch_size[file_num])].append(temp)

			elif args.data_structure == "AoSoA":
				temp = np.mean(kernel_time[str(args.patch_size[file_num])][
							   (3 * i + 1) * int(args.time_steps) + 1: (3 * i + 2) * int(args.time_steps)])
				mean_kernel_time[str(args.patch_size[file_num])].append(temp)
			elif args.data_structure == "SoA":
				temp = np.mean(kernel_time[str(args.patch_size[file_num])][
							   (3 * i + 2) * int(args.time_steps) + 1: (3 * i + 3) * int(args.time_steps)])
				mean_kernel_time[str(args.patch_size[file_num])].append(temp)

		total_unknowns = 4 * number_of_patches[str(args.patch_size[file_num])] * int(args.patch_size[file_num]) ** 2
		ax.plot(number_of_patches[str(args.patch_size[file_num])],
				[j / k for j, k in zip(mean_kernel_time[str(args.patch_size[file_num])], total_unknowns)],
				linewidth=1.5, marker=str(markers[file_num]), markersize=4, color=cmap(file_num),
				label=f"$p={args.patch_size[file_num]}$")

	if args.files_no_reduction:
		input_files = [open(i, "r") for i in args.files_no_reduction]

		kernel_time = {str(i): [] for i in args.patch_size}
		mean_kernel_time = {str(i): [] for i in args.patch_size}
		number_of_patches = {str(i): [] for i in args.patch_size}

		for file_num in range(len(input_files)):
			for line in input_files[file_num]:
				if "Number of patches" in line:
					patch_count = int(line.split(":")[-1])
					number_of_patches[str(args.patch_size[file_num])].append(patch_count)
				if args.kernel_variant == "batched":
					if "Batched kernel execution time" in line:
						kernel_time[str(args.patch_size[file_num])].append(float(line.split(":")[-1]))
				elif args.kernel_variant == "patch-wise":
					if "Patchwise kernel execution time" in line:
						kernel_time[str(args.patch_size[file_num])].append(float(line.split(":")[-1]))
				elif args.kernel_variant == "task-graph":
					if "Task graph kernel execution time" in line:
						kernel_time[str(args.patch_size[file_num])].append(float(line.split(":")[-1]))
			for i in range(len(number_of_patches[str(args.patch_size[file_num])])):
				if args.data_structure == "AoS":
					temp = np.mean(kernel_time[str(args.patch_size[file_num])][
								   i * 3 * int(args.time_steps) + 1: (3 * i + 1) * int(
									   args.time_steps)])  # start from 1 since the first run is a warm up
					mean_kernel_time[str(args.patch_size[file_num])].append(temp)

				elif args.data_structure == "AoSoA":
					temp = np.mean(kernel_time[str(args.patch_size[file_num])][
								   (3 * i + 1) * int(args.time_steps) + 1: (3 * i + 2) * int(args.time_steps)])
					mean_kernel_time[str(args.patch_size[file_num])].append(temp)
				elif args.data_structure == "SoA":
					temp = np.mean(kernel_time[str(args.patch_size[file_num])][
								   (3 * i + 2) * int(args.time_steps) + 1: (3 * i + 3) * int(args.time_steps)])
					mean_kernel_time[str(args.patch_size[file_num])].append(temp)

			total_unknowns = 4 * number_of_patches[str(args.patch_size[file_num])] * int(args.patch_size[file_num]) ** 2
			ax.plot(number_of_patches[str(args.patch_size[file_num])],
					[j / k for j, k in zip(mean_kernel_time[str(args.patch_size[file_num])], total_unknowns)],
					linewidth=1.5, linestyle='dashed', marker=str(markers[file_num]), markersize=4,
					color=cmap(file_num),
					label=f"$p={args.patch_size[file_num]}$, no reduction")

	plt.style.use("ggplot")
	ax.set_xscale('log', base=2)
	ax.set_yscale('log')
	plt.legend(fancybox=True)
	plt.xlabel(r"Number of patches, $T$")
	plt.ylabel("Kernel time per unknown / s")
	plt.title(f"{args.kernel_variant}, {args.data_structure}")
	plt.grid(color='grey', alpha=0.5, linestyle='dashed', linewidth=0.5)
	plt.savefig(args.kernel_variant + ".pdf", bbox_inches='tight')
