import sys
import matplotlib.pyplot as plt
import numpy as np
print sys.argv[1]
data = np.genfromtxt('tmpfilename.mat', delimiter=' ', names=['x', 'y', 'z'])
x = data['x']
y = data['y']

plt.plot(x, y, linestyle='-', color='r')
plt.show()
