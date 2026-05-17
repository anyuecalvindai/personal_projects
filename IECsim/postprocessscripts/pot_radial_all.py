import os
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.animation as animation

run = 'run_io_50A/'
plots = run + 'plots/'
os.makedirs(plots, exist_ok=True)

axes = ['x', 'y', 'z']
labels = ['X axis (side tube)', 'Y axis (long tube vertical diameter)', 'Z axis (long tube)']

for axis, label in zip(axes, labels):
    df = pd.read_csv(run + f'potential_radial_{axis}_all.dat', sep=r'\s+', comment='#', header=None, names=['iteration', 'r', 'potential'])
    iterations = df['iteration'].unique()

    #static plot of all iterations
    fig, ax = plt.subplots(figsize=(10, 6))
    for i in iterations:
        data = df[df['iteration'] == i]
        ax.plot(data['r'], data['potential'], alpha=0.5, label=f'iter {i}')
    ax.set_xlabel('r (m)')
    ax.set_ylabel('potential (V)')
    ax.set_title(f'Radial potential - {label} - all iterations')
    ax.grid(True)
    ax.axvline(x=0, color='k', linestyle='--', linewidth=0.5)
    ax.legend(fontsize=6)
    plt.savefig(plots + f'potential_radial_{axis}_all_iterations.png', dpi=150)
    plt.close()

    #animation
    fig, ax = plt.subplots(figsize=(10, 6))

    def animate(i, df=df, iterations=iterations, ax=ax):
        ax.clear()
        data = df[df['iteration'] == iterations[i]]
        ax.plot(data['r'], data['potential'])
        ax.set_xlabel('r (m)')
        ax.set_ylabel('potential (V)')
        ax.set_title(f'Radial potential - {label} - iteration {iterations[i]}')
        ax.grid(True)
        ax.axvline(x=0, color='k', linestyle='--', linewidth=0.5)

    ani = animation.FuncAnimation(fig, animate, frames=len(iterations), interval=500)
    ani.save(plots + f'potential_radial_{axis}_animation.gif', writer='pillow')
    plt.close()