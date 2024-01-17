import subprocess

meshes = (3, )

for mesh in meshes:
    meshsize = 0.99 * 1.0/mesh
    options = ["-m", "release", "--meshsize", meshsize]
    subprocess.run(["python3", "dg.py"] + options)