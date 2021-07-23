import numpy as np
from scipy import io
import os
import time

def tracer_seeds_generate(Type=1, a=-0.5, b=0.5 ,N_x=10, N_y=10, N_z=10):
	x_s = np.linspace(a,b,N_x)
	if Type==2: #slide
		y_s = np.linspace(a,b,N_y)
	if Type==3: #volume
		y_s = np.linspace(a,b,N_y)
		z_s = np.linspace(a,b,N_z)
		
	if Type==1:
		print("creating tracer seeds on the line of x-axis")
		f=open('line.dat','w')
		for i in range(N_x):
			x=str(x_s[i]);	y=str(0);	z=str(0)	
			line=" "+x+" "+y+" "+z+"\n"
			f.write(line)
		f.close()
		
	if Type==2:
		print("creating tracer seeds on x-y plane")	
		f=open('slide.dat','w')
		for i in range(N_x):
			for j in range(N_y):
				x=str(x_s[i]);	y=str(y_s[j]);	z=str(0)
				line=" "+x+" "+y+" "+z+"\n"
				f.write(line)
		f.close()
	  
	if Type==3:
		print("creating tracer seeds over domain")
		f=open('volume.dat','w')
		for i in range(N_x):
			for j in range(N_y):
					for k in range(N_z):
						x=str(x_s[i]);	y=str(y_s[j]);	z=str(z_s[k])
						line=" "+x+" "+y+" "+z+"\n"
						f.write(line)
		f.close()	  















