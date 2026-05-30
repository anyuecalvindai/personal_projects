import os
import glob
import re
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator

run = 'run25/'
plots = run + 'plots/'
os.makedirs(plots, exist_ok=True)

# Parse V_cathode and beam_current from the run's copied cpp (whichever it is)
threshold = None
v_kV = None
beam_A = None
for cpp in glob.glob(os.path.join(run, 'fusorsim*.cpp')):
    with open(cpp) as f:
        for line in f:
            m = re.match(r"\s*double\s+cathodepot\s*=\s*(-?[\d.]+)", line)
            if m and threshold is None:
                threshold = 0.001 * abs(float(m.group(1)))
                v_kV = float(m.group(1)) / 1000.0
            m2 = re.match(r"\s*const\s+double\s+beam_current\s*=\s*([\d.eE+\-]+)", line)
            if m2 and beam_A is None:
                beam_A = float(m2.group(1))
    if threshold is not None:
        break
if threshold is None:
    threshold = 5.0

def fmt_I(I):
    if I is None: return ""
    if abs(I) >= 1.0: return f"{I:.3g} A"
    if abs(I) >= 0.001: return f"{I*1000:.3g} mA"
    return f"{I*1e6:.3g} uA"

label_parts = []
if beam_A is not None: label_parts.append(fmt_I(beam_A))
if v_kV is not None: label_parts.append(f"{v_kV:.0f} kV")
title = "Iteration error" + (": " + ", ".join(label_parts) if label_parts else "")

df = pd.read_csv(run + 'epot_error.dat', sep=r'\s+', comment='#', header=None, names=['iteration', 'epot_max_error'])

fig, ax = plt.subplots(figsize=(10, 6))
ax.plot(df['iteration'], df['epot_max_error'], marker='o', markersize=10, linewidth=2)
ax.set_xlabel('Iteration')
ax.set_ylabel('Epot max error (V)')
ax.set_title(title)
ax.grid(True, alpha=0.3)
ax.axhline(y=threshold, color='r', linestyle='--', linewidth=1.5,
           label=f'{threshold:.3g} V threshold (0.1% of |V_cathode|)')
ax.legend(fontsize=10)
ax.xaxis.set_major_locator(MaxNLocator(integer=True))
fig.tight_layout()
fig.savefig(plots + 'epot_convergence.png', dpi=150)