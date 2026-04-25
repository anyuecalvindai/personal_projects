import pandas as pd
import matplotlib.pyplot as plt

run = 'run8/'
plots = run + 'plots/'

df = pd.read_csv(run + 'epot_error.dat', sep=r'\s+', comment='#', header=None, names=['iteration', 'epot_max_error'])

fig, ax = plt.subplots(figsize=(10, 6))
ax.plot(df['iteration'], df['epot_max_error'], marker='o')
ax.set_xlabel('iteration')
ax.set_ylabel('epot max error (V)')
ax.set_title('Epot convergence per iteration')
ax.grid(True)
ax.axhline(y=5.0, color='r', linestyle='--', label='5V threshold')
ax.legend()
plt.savefig(plots + 'epot_convergence.png', dpi=150)