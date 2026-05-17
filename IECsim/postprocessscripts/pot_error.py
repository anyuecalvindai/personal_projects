import os
import glob
import re
import pandas as pd
import matplotlib.pyplot as plt

run = 'run_ns_25kV_10A/'
plots = run + 'plots/'
os.makedirs(plots, exist_ok=True)

# Parse V_cathode from the run's copied cpp (whichever it is)
threshold = None
for cpp in glob.glob(os.path.join(run, 'fusorsim*.cpp')):
    with open(cpp) as f:
        for line in f:
            m = re.match(r"\s*double\s+cathodepot\s*=\s*(-?[\d.]+)", line)
            if m:
                threshold = 0.001 * abs(float(m.group(1)))  # 0.1% of |V_cathode|
                break
    if threshold is not None:
        break
if threshold is None:
    threshold = 5.0  # fallback if cpp not available

df = pd.read_csv(run + 'epot_error.dat', sep=r'\s+', comment='#', header=None, names=['iteration', 'epot_max_error'])

fig, ax = plt.subplots(figsize=(10, 6))
ax.plot(df['iteration'], df['epot_max_error'], marker='o')
ax.set_xlabel('iteration')
ax.set_ylabel('epot max error (V)')
ax.set_title('Epot convergence per iteration')
ax.grid(True)
ax.axhline(y=threshold, color='r', linestyle='--',
           label=f'{threshold:.3g} V threshold (0.1% of |V_cathode|)')
ax.legend()
plt.savefig(plots + 'epot_convergence.png', dpi=150)