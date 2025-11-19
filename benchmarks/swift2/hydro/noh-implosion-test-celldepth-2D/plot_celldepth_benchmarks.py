#!/usr/bin/env python3

# --------------------------------------------------------------------------
# Plot results of cell depth benchmarks.
# Expects outputs with the file name format
#
#   output_N$N_PART-$CELL_MAX-$CELL_MIN.log
#
# as written by `run_celldepth_test.sh`.
# The content of that output shall be the peano swift2 stdout, with the
# addition of an external timer run as such (using bash):
#
#  timing() {
#
#      timeformat="----------------------------------------------------------------\n"
#      timeformat="$timeformat""stats for %C\n"
#      timeformat="$timeformat""AverageMem(kb),MaxMem(kb),WallTime(s),CPUTime(s),UserTime(s),CPUPercentage\n"
#      timeformat="$timeformat""%K,%M,%e,%S,%U,%P"
#
#      /usr/bin/time -f "$timeformat" "$@"
#  }
#
#  OUTFILE=output_N$N_PART-$CELL_MAX-$CELL_MIN.log
#  timing -o "$OUTFILE" -a ./noh2D-$SUFFIX 2>&1 > "$OUTFILE"
#
# --------------------------------------------------------------------------


from matplotlib import pyplot as plt
import os
import argparse


def get_all_logs():
    """
    Find all log files
    """

    ls = os.listdir()
    logfiles = []

    for f in ls:
        if f.startswith("output_N") and f.endswith(".log"):
            logfiles.append(f)

    if len(logfiles) == 0:
        raise ValueError("No log files found. Directory contents:", ls)

    return logfiles


class BenchmarkData:
    def __init__(self, file):
        """
        Pass a valid filename to a logfile.
        It will extract benchmark timing data.
        """

        self.file = file

        self.npart = 0
        self.cellmax = 0.0
        self.cellmin = 0.0

        self.walltime = 0.0
        self.maxmem = 0.0
        self.CPUPercentage = 0.0

        self.grid_construction = 0.0
        self.initial_conditions = 0.0
        self.initialisation = 0.0
        self.plotting = 0.0
        self.cleanup = 0.0
        self.timestepping = 0.0

        self._read_logfile()

    def print(self):
        print("----------------------------------------------------------")
        print(f"file=      {self.file}")
        print(f"npart=     {self.npart}")
        print(f"cell_min=  {self.cellmin}")
        print(f"cell_max=  {self.cellmax}")

        print(f"walltime=       {self.walltime} (s)")
        print(f"max memory=     {self.maxmem} (kB)")
        print(f"CPU Percentage= {self.CPUPercentage} %")

        print(f"grid construction=  {self.grid_construction} (s)")
        print(f"initial conditions= {self.initial_conditions} (s)")
        print(f"initialisation=     {self.initialisation} (s)")
        print(f"plotting=           {self.plotting} (s)")
        print(f"cleanup=            {self.cleanup} (s)")
        print(f"timestepping=       {self.timestepping} (s)")
        print("----------------------------------------------------------")

    def _read_logfile(self):
        """
        Extract data from a logfile.
        """

        # First meta-data from filename.

        rawfilename = self.file.strip(".log")
        rawfilename = rawfilename.strip("output_")

        n, cmax, cmin = rawfilename.split("-")
        self.npart = int(n.strip("N"))
        self.cellmax = float(cmax)
        self.cellmin = float(cmin)

        f = open(self.file, "r")
        lines = f.readlines()
        timing = lines[-1]
        timing = timing.strip()
        avmem, maxmem, walltime, cputime, usertime, cpupercent = timing.split(",")

        self.maxmem = float(maxmem)
        self.walltime = float(walltime)
        self.CPUPercentage = float(cpupercent.strip("%"))

        termline = lines[-11]
        tl = termline.strip()
        # remove all whitespace, then join together with a single whitespace
        tl = " ".join(tl.split())
        _, _, _, _, _, _, terminated, successfully = tl.split()
        if terminated != "terminated" or successfully != "successfully":
            raise ValueError("Reading wrong 'terminated successfully' line:", termline)

        gridline = lines[-10]
        gl = gridline.strip()
        gl = " ".join(gl.split())
        _, _, _, _, _, _, grid, construction, timing, _, _, _ = gl.split()
        if grid != "grid" or construction != "construction:":
            raise ValueError("Reading wrong 'grid construction' line", gridline)
        gridtime = float(timing.strip("s"))
        self.grid_construction = gridtime

        icline = lines[-9]
        icl = icline.strip()
        icl = " ".join(icl.split())
        _, _, _, _, _, _, initial, conditions, timing, _, _, _ = icl.split()
        if initial != "initial" or conditions != "conditions:":
            raise ValueError("Reading wrong 'initial conditions' line", icline)
        ictime = float(timing.strip("s"))
        self.initial_conditions = ictime

        initline = lines[-8]
        il = initline.strip()
        il = " ".join(il.split())
        _, _, _, _, _, _, initialisation, timing, _, _, _ = il.split()
        if initialisation != "initialisation:":
            raise ValueError("Reading wrong 'initialalisation' line", initline)
        inittime = float(timing.strip("s"))
        self.initialisation = inittime

        plotline = lines[-7]
        pl = plotline.strip()
        pl = " ".join(pl.split())
        _, _, _, _, _, _, plotting, timing, _, _, _ = pl.split()
        if plotting != "plotting:":
            raise ValueError("Reading wrong 'plotting' line", plotline)
        plottime = float(timing.strip("s"))
        self.plotting = plottime

        cleanup = lines[-6]
        cl = cleanup.strip()
        cl = " ".join(cl.split())
        _, _, _, _, _, _, cleanup, timing, _, _, _ = cl.split()
        if cleanup != "cleanup:":
            raise ValueError("Reading wrong 'cleanup' line", cleanup)
        cleantime = float(timing.strip("s"))
        self.cleanup = cleantime

        timestepping = lines[-5]
        tl = timestepping.strip()
        tl = " ".join(tl.split())
        _, _, _, _, _, _, time, stepping, timing, _, _, _ = tl.split()
        if time != "time" and stepping != "stepping:":
            raise ValueError("Reading wrong 'time stepping' line", timestepping)
        tstime = float(timing.strip("s"))
        self.timestepping = tstime

        f.close()

        return


parser = argparse.ArgumentParser(description="Plot benchmark outputs.")
parser.add_argument(
    "-l",
    "--log",
    dest="log",
    action="store_true",
    default=False,
    help="Plot log scale",
)
parser.add_argument(
    "-b",
    "--both",
    dest="both",
    action="store_true",
    default=False,
    help="Plot lin and log scale",
)
args = parser.parse_args()

log = args.log
both = args.both


logfiles = get_all_logs()
alldata = [BenchmarkData(l) for l in logfiles]

# find all N_PART available
npartlist = []
for d in alldata:
    if d.npart not in npartlist:
        npartlist.append(d.npart)


data_by_npart = []

for n in npartlist:
    templist = []
    for d in alldata:
        if d.npart == n:
            templist.append(d)

    # Sort the individual list by CELL_MAX/CELL_MIN
    sort_val_list = [t.cellmax * 1e3 + t.cellmin for t in templist]

    sort_val_list, templist = (
        list(t) for t in zip(*sorted(zip(sort_val_list, templist)))
    )
    templist = templist[::-1]  # invert sorting

    data_by_npart.append(templist)



if both:
    fig = plt.figure(figsize=(24, 6), dpi=300)
    ax1 = fig.add_subplot(1, 2, 1)
    ax2 = fig.add_subplot(1, 2, 2)
else:
    fig = plt.figure(figsize=(12, 6), dpi=300)
    ax = fig.add_subplot(1, 1, 1)

markers = ["s", "o", "^", "<", ">"]

for n, npart in enumerate(npartlist):
    data = data_by_npart[n]

    xvals = [str(d.cellmax) + "/" + str(d.cellmin) for d in data]
    walltimes = [d.walltime for d in data]
    timestepping = [d.timestepping for d in data]

    if both:
        ax1.plot(
            xvals,
            walltimes,
            marker=markers[n],
            label="walltime " + str(npart) + r"$^2$",
            linestyle="-",
            c="C" + str(n),
        )
        ax1.plot(
            xvals,
            timestepping,
            marker=markers[n],
            label="timestepping " + str(npart) + r"$^2$",
            linestyle="--",
            c="C" + str(n),
        )

        ax2.semilogy(
            xvals,
            walltimes,
            marker=markers[n],
            label="walltime " + str(npart) + r"$^2$",
            linestyle="-",
            c="C" + str(n),
        )
        ax2.semilogy(
            xvals,
            timestepping,
            marker=markers[n],
            label="timestepping " + str(npart) + r"$^2$",
            linestyle="--",
            c="C" + str(n),
        )

    else:
        if log:
            ax.semilogy(
                xvals,
                walltimes,
                marker=markers[n],
                label="walltime " + str(npart) + r"$^2$",
                linestyle="-",
                c="C" + str(n),
            )
            ax.semilogy(
                xvals,
                timestepping,
                marker=markers[n],
                label="timestepping " + str(npart) + r"$^2$",
                linestyle="--",
                c="C" + str(n),
            )
        else:
            ax.plot(
                xvals,
                walltimes,
                marker=markers[n],
                label="walltime " + str(npart) + r"$^2$",
                linestyle="-",
                c="C" + str(n),
            )
            ax.plot(
                xvals,
                timestepping,
                marker=markers[n],
                label="timestepping " + str(npart) + r"$^2$",
                linestyle="--",
                c="C" + str(n),
            )

if both:
    for ax in [ax1, ax2]:
        ax.set_xlabel("cell_size_max/cell_size_min")
        ax.set_ylabel("time (s)")
        ax.legend()
else:
    ax.set_xlabel("cell_size_max/cell_size_min")
    ax.set_ylabel("time (s)")
    ax.legend()

suffix = ""
if log:
    suffix = "-log"
if both:
    suffix = "-lin-log"
fname = "celldepth_benchmark" + suffix + ".png"

plt.tight_layout()
#  plt.show()
plt.savefig(fname)
print("Saved", fname)
