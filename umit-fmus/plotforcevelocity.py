import matplotlib.pyplot as plt
import sys
import numpy as np

fig = 1
for f in sys.argv[1::]:
    plt.figure(fig)
    fig += 1
    # Parse filename
    dash = f.split('-')
    method = dash[1]
    dt = float(dash[2]) * 0.001
    dot = dash[4].split('.')
    print  dot[0]
    gain = float(dot[0])*0.1
    print 'dt = %f, gain = %f' % (dt, gain)

    data = np.genfromtxt(f, delimiter=',')
    print data.shape
    
    # Translate method to more intuitively understood string
    trans = {
    'gs': 'Gauss-Seidel (serial)',
    'jacobi': 'Jacobi (parallel)',
    }

    plt.plot(data[:,0], data[:,[2,7]])
    plt.axis([0,100,0,180])
    plt.title('method=%s, dt=%.3f, gain=%.1f' % (trans[method], dt, gain))
    plt.gca().set_autoscale_on(False)
    

    # Change file extension to .svg
    parts = f.split('.')
    plt.savefig('.'.join(parts[0:-1] + ['svg']))

plt.show()
