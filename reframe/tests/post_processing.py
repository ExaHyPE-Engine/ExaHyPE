import numpy as np

def getTime(sline, token):
    return sline.split(token)[-1].split()[0].replace("s","")

def getTimeStepping(fname):
    t_timest=""

    with open(fname) as f:
        for line in f:
            l = line.strip()
            if "time stepping:" in l:
                t_timest = getTime(l, "stepping:")
            else:
                continue
    return t_timest

