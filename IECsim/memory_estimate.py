import numpy as np
import os

run = 'run7/'

N_particles = 1000000
bytes_per_particle = 108
bytes_per_step = 56

data = np.loadtxt(run + 'trajectory_steps.dat', comments='#', dtype=np.int64)
if data.ndim == 1:
    data = data.reshape(1, -1)

iterations = data[:, 0]
total_steps = data[:, 1]

total_bytes = N_particles * bytes_per_particle + total_steps * bytes_per_step
total_mb = total_bytes / (1024**2)
total_gb = total_bytes / (1024**3)

os.makedirs(run + 'plots', exist_ok=True)
with open(run + 'plots/memory_estimate.txt', 'w') as f:
    f.write('# iteration    total_steps    total_MB    total_GB\n')
    for i, it in enumerate(iterations):
        f.write(f'{it:10d} {total_steps[i]:20d} {total_mb[i]:14.4f} {total_gb[i]:14.6f}\n')
