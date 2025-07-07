import argparse
import umbridge

parser = argparse.ArgumentParser(description='Model output test.')
parser.add_argument('--url', metavar='url', type=str,
                    help='the URL on which the model is running, for example http://localhost:4242')
args = parser.parse_args()

print(f"Connecting to host URL {args.url}")
model = umbridge.HTTPModel(args.url, "TafjordLandslide")

inputs = [200, 1000]
outputs = []

for friction in inputs:
    outputs.append(model([[friction]]))

print(outputs)  # Simulation runs should differ for different frictions
