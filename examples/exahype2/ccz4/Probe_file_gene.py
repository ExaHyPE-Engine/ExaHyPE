import numpy as np
import matplotlib.pyplot as plt
from scipy import io
import os
import time
import glob

Line=0
slide=1

N=40
N2=N
start=-0.45
end=0.45
x_s = np.linspace(start,end,N)
y_s = np.linspace(start,end,N2)
if Line:
	f=open('Line.dat','w')

	for i in range(N):
		x = x_s[i] 
		x = str(x)
		y = x
		y = str(y)
		z = x
		z = str(z)
		line=" "+x+" "+y+" "+z+"\n"
		f.write(line)

	f.close()

if slide:
	f=open('slide.dat','w')
	for i in range(N):
		for j in range(N2):
			x = x_s[i] 
			x = str(x)
			y = y_s[j]
			y = str(y)
			z = 0.0
			z = str(z)
			line=" "+x+" "+y+" "+z+"\n"
			f.write(line)

	f.close()















