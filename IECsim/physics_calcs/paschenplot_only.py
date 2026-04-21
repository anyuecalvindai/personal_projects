import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from scipy.interpolate import UnivariateSpline

df = pd.read_csv('pschen.csv')


df['pressure_mTorr'] = df['Pressure/mbar'] * 750.062
df['voltage_V'] = df['Striking Voltage/kV'] * 1000
df['pd'] = (df['pressure_mTorr'] / 1000) * 3  # pressure in Torr × d in cm
pressure_abs_uncs_mBar = [0.05,0.05,0.05,0.05,5,50,50,0.005,0.005,0.005]
voltage_abs_unc_kV = 0.005
# convert to matching units

df['pd_unc'] = [(u * 750.062 / 1000) * 3 for u in pressure_abs_uncs_mBar]

df = df.sort_values('pd')
log_pd = np.log10(df['pd'])
log_V = np.log10(df['voltage_V'])

# for polynomial fit
coeffs = np.polyfit(log_pd, log_V, 2)
poly = np.poly1d(coeffs)

pd_fit = np.logspace(np.log10(df['pd'].min()), np.log10(df['pd'].max()), 500)
V_fit = 10**poly(np.log10(pd_fit))

# for spline fit
# pd_fit = np.logspace(np.log10(df['pd'].min()), np.log10(df['pd'].max()), 500)
# spline = UnivariateSpline(log_pd, log_V, s=0.1)
# V_fit = 10**spline(np.log10(pd_fit))



fig, ax = plt.subplots(figsize=(10, 6))
#ax.scatter(df['pd'], df['voltage_V'], marker='o', label='experimental data', zorder=5)
ax.errorbar(df['pd'], df['voltage_V'],
            yerr=voltage_abs_unc_kV * 1000,
            xerr=df['pd_unc'],
            fmt='o', capsize=4, markersize=4,label='experimental data')
ax.plot(pd_fit, V_fit, 'r-', label='polynomial fit (2nd degree)')
ax.set_xscale('log')
ax.set_yscale('log')
ax.set_xlabel('pd (Torr·cm)')
ax.set_ylabel('breakdown voltage (V)')
ax.set_title('Paschen curve - experimental data')
ax.grid(True, which='both', alpha=0.3)
ax.legend()
plt.savefig('paschen_curve_experimental.png', dpi=150)