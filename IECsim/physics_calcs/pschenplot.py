import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from scipy.optimize import curve_fit

df = pd.read_csv('pschen.csv')

df['pressure_mTorr'] = df['Pressure/mbar'] * 750.062
df['voltage_V'] = df['Striking Voltage/kV'] * 1000
df['pd'] = (df['pressure_mTorr'] / 1000) * 3  # pressure in Torr × d in cm

log_pd = np.log10(df['pd'])
log_V = np.log10(df['voltage_V'])
coeffs = np.polyfit(log_pd, log_V, 2)
poly = np.poly1d(coeffs)




pd_fit = np.logspace(np.log10(df['pd'].min()), np.log10(df['pd'].max()), 500)
V_fit = 10**poly(np.log10(pd_fit))

# theoretical Paschen curve
A = 12      # cm^-1 Torr^-1 for nitrogen
B = 342      # V cm^-1 Torr^-1 for nitrogen
gamma = 10  # Agarwal et al. 2017, copper cathode in N2

pd_theory = np.logspace(-2, np.log10(df['pd'].max()), 500)
V_theory = (B * pd_theory) / (np.log(A * pd_theory) - np.log(np.log(1 + 1/gamma)))


fig, ax = plt.subplots(figsize=(10, 6))
ax.scatter(df['pd'], df['voltage_V'], marker='o', label='experimental data', zorder=5)
ax.plot(pd_fit, V_fit, 'r-', label='polynomial fit (degree 2)')
ax.plot(pd_theory, V_theory, 'b--', label='theoretical Paschen (Townsend)')
ax.set_xscale('log')
ax.set_yscale('log')
ax.set_xlabel('pd (Torr·cm)')
ax.set_ylabel('breakdown voltage (V)')
ax.set_title('Paschen curve')
ax.grid(True, which='both', alpha=0.3)
ax.legend()
plt.savefig('paschen_curve.png', dpi=150)

denom = np.log(A * pd_theory) - np.log(np.log(1 + 1/gamma))
print("pd range:", pd_theory.min(), pd_theory.max())
print("A*pd range:", (A * pd_theory).min(), (A * pd_theory).max())
print("denom range:", denom.min(), denom.max())
print("V_theory range:", V_theory.min(), V_theory.max())