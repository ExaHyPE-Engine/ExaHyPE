import subprocess
import os


"""
Perform git clean -xdf, 
Execute test1.py -m release,
Run ./peano4matrixfree > out.txt
Post-process
"""

# git clean
subprocess.run(["git", "clean", "-xdf"], check=True)

# Run test Python script
poly_degree = 3
subprocess.run(["python3", "test1.py", "-m", "release", "-p", str(poly_degree)], check=True)

# Obtain the path to the generated executable peano4matrixfree
exec_path = os.path.join(os.getcwd(), "peano4matrixfree")

# Run the executable ./peano4matrixfree
output = subprocess.run([exec_path], capture_output=True, text=True, check=True)
output_filename = "out.txt" # for executable terminal output 
with open(output_filename, "w") as outfile:
    outfile.write(output.stdout)
    print("Executable output saved to out.txt")

# Post-process
subprocess.run(["python3", "parseTest1.py"], check=True)
