import csv, argparse, sys, math, subprocess, os
import arviz as az
import numpy as np
import scipy.stats
import emcee
import matplotlib.pyplot as plt
from AcousticPulse import compile_LOH1
from multiprocessing import Pool

class Benchmark():
    def __init__(self, verbose=False, level=1):
        self.level=level
        self.verbose = verbose
        compile_LOH1()
        candidate = self.run_with_parameters([0.0, 2.0])
        self.data = self.extract_max_velocities_and_timestamps(candidate+"/acoustic.csv")
        subprocess.run(["bash", "-c", f"rm -r {candidate}"])

    def get_input_sizes(self):
        return 2

    def get_output_sizes(self):
        return 1

    def run_with_parameters(self, parameters):
        outputPath = f"sol-{parameters[0]}-{parameters[1]}"

        config = """{
    "endTime": 2.0,
    "gridLevels": """ + str(self.level) + """,
    "firstPlotTimeStamp": 0.0,
    "numberOfSnapshots": 0,
    "outputPath": """ + '"' + outputPath + '"' + """,
    "timeStepLogInterval": 10.0,
    "targetDevice": 0,
    "pointSourcePosition": [
        """ + str(parameters[0]) + """,
        """ + str(parameters[1]) + """
    ]
}"""

        configFile = f"config-{outputPath}.json"
        file = open(configFile, "w")
        file.write(config)
        file.close()

        subprocess.run(["bash", "-c", f"./acoustic-pulse -cf {configFile}" + (
            "" if self.verbose else ">/dev/null"
            )])
        subprocess.run(["bash", "-c", f"rm -r {configFile}"])
        return outputPath

    # For a given file name representing the csv output to the acoustic model,
    # this parses the csv file and extracts the largest velocities for each tracer,
    # and returns the value of the highest measured velocity as well as the timestamp
    # at which said velocity was measured
    def extract_max_velocities_and_timestamps(self, fileName):
        inputFile = open(fileName, "r")
        csvData = list(csv.reader(inputFile, delimiter=","))

        header = csvData[0]
        timeIndex = header.index(" t")
        dataIndex = header.index(" data(0)")
        firstNumberIndex = header.index("number(0)")
        secondNumberIndex = header.index(" number(1)")
        positionIndexes = [header.index(" x(0)"), header.index(" x(1)")]
        if " x(2)" in header:
            positionIndexes.append(header.index(" x(2)"))

        # Drop the header, plus any trailing blank line the writer may have left behind. Slicing the
        # last row off unconditionally would discard a real snapshot in a file without one.
        # magnitude of velocity
        # csvData = [[float(row[firstNumberIndex]), float(row[secondNumberIndex]), float(row[timeIndex]), np.sqrt(float(row[dataIndex])**2+float(row[dataIndex+1])**2+(float(row[dataIndex+2])**2) if dim==3 else 0)] for row in csvData[1:]]
        # pressure
        csvData = [[float(row[firstNumberIndex]), float(row[secondNumberIndex]), float(row[timeIndex]), float(row[dataIndex])] for row in csvData[1:]]
        checkFirstIndexLength = set([(data[firstNumberIndex], data[secondNumberIndex]) for data in csvData])

        max_v, max_dt = [], []

        for a, b in checkFirstIndexLength:
            data = [d for d in csvData if d[firstNumberIndex]==a and d[secondNumberIndex]==b]
            dt = list(np.array(data).T[timeIndex])
            data = list(np.array(data).T[3])
            index_of_max = data.index(np.max(data))
            max_v.append(data[index_of_max])
            max_dt.append(dt[index_of_max])

        return max_v+max_dt

    def __call__(self, parameters):

        # Cut-off prior, i.e. if value is outside of domain, return a very large number
        if (
               not ( -4.5 < parameters[0] < 4.5)
            or not (0 < parameters[1] < 9.0)
            # or not (-4.5 < parameters[2] < 4.5)
          ):

            return [[-1234]]

        outFile = self.run_with_parameters(parameters)
        model_output = self.extract_max_velocities_and_timestamps(outFile+"/acoustic.csv")
        subprocess.run(["bash", "-c", f"rm -r {outFile}"])

        likelihood_std_dev_time     = 1.5
        likelihood_std_dev_height   = 0.1
        likelihood_cov_matrix_diag = [likelihood_std_dev_height**2]*4 + [likelihood_std_dev_time**2]*4

        posterior = scipy.stats.multivariate_normal.logpdf(model_output, self.data, likelihood_cov_matrix_diag)
        return [[posterior]]

if __name__ == '__main__':
    benchmark = Benchmark(level=2)

    nWalkers = 8 #number of parallel chains sampling
    nSteps = 200
    #start with randomly distributed values within the domain
    # p0 = np.random.rand(nwalkers, benchmark.get_input_sizes()) * 9 - 4.5
    # p0.T[1] += 4.5
    p0=np.random.uniform(low=[-2.5, 2.], high=[2.5, 7.], size=(nWalkers, 2))

    with Pool(processes=nWalkers) as pool:
        # More info about the emcee sampler at https://emcee.readthedocs.io/en
        sampler = emcee.EnsembleSampler(nWalkers, benchmark.get_input_sizes(), benchmark, pool=pool)
        # run sampler
        state = sampler.run_mcmc(
            initial_state=p0,
            nsteps=nSteps,
            progress=True
        ) #number of samples

    # plot results
    inference_data = az.from_emcee(sampler)
    az.plot_pair(inference_data)
    plt.title(az.summary(inference_data, round_to=2))
    plt.tight_layout()
    plt.savefig(f'emcee_inference{nSteps}.png')

    print(az.summary(inference_data, round_to=2))

