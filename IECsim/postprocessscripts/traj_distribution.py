import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

run = 'run9/'
plots = run + 'plots/'
os.makedirs(plots, exist_ok=True)

df = pd.read_csv(run + 'trajectory_lengths.dat', sep=r'\s+', comment='#', header=None, names=['particle', 'length'])
df.to_csv(run + 'trajectory_lengths.csv', index=False)

fig, ax = plt.subplots(figsize=(10,7))
ax.hist(df['length'], bins=50)
ax.set_xlabel('trajectory length (m)')
ax.set_ylabel('number of particles')
plt.savefig(plots + 'traj_distribution.png')