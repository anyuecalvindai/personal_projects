import os
import pandas as pd
import numpy as np

run = 'run18/'
plots = run + 'plots/'
os.makedirs(plots, exist_ok=True)

df = pd.read_csv(run + 'epot_error.dat', sep=r'\s+', comment='#', header=None, names=['iteration', 'epot_max_error'])

#RMS of the max errors across all iterations

#RMS = l2norm/sqrt(n)

rms = np.sqrt(np.mean(df['epot_max_error']**2))

with open(plots + 'epot_rms.txt', 'w') as f:
    f.write(f'RMS epot error across all iterations: {rms:.4f} V\n')
    f.write(f'Mean epot error: {df["epot_max_error"].mean():.4f} V\n')
    f.write(f'Max epot error: {df["epot_max_error"].max():.4f} V\n')
    f.write(f'Min epot error: {df["epot_max_error"].min():.4f} V\n')
    f.write(f'Number of iterations: {len(df)}\n')

print(f'RMS epot error: {rms:.4f} V')