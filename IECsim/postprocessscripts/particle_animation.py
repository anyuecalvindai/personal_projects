import os
import re
from glob import glob
import numpy as np
import matplotlib.pyplot as plt
import imageio.v2 as imageio

run = 'run8/'
plots = run + 'plots/'
pout = run + 'pout/'
os.makedirs(plots, exist_ok=True)

frames_dir = plots + 'particle_frames/'
os.makedirs(frames_dir, exist_ok=True)

files = glob(pout + 'pout_*.txt')

def step_num(p):
    m = re.search(r'pout_(\d+)\.txt', os.path.basename(p))
    return int(m.group(1)) if m else -1

files = sorted(files, key=step_num)

if not files:
    raise SystemExit('no pout_*.txt files found in ' + pout)

frame_paths = []
for f in files:
    s = step_num(f)
    data = np.loadtxt(f)
    if data.ndim == 1:
        data = data.reshape(1, -1)
    x = data[:, 0]
    z = data[:, 2]

    fig, ax = plt.subplots(figsize=(7, 7))
    ax.scatter(x, z, s=2, c='blue')
    ax.set_xlim(-0.05, 0.05)
    ax.set_ylim(-0.05, 0.05)
    ax.set_xlabel('x (m)')
    ax.set_ylabel('z (m)')
    ax.set_title(f'step {s}')
    ax.set_aspect('equal')

    frame_path = frames_dir + f'particles_{s:06d}.png'
    plt.savefig(frame_path, dpi=100)
    plt.close()
    frame_paths.append(frame_path)

frames = [imageio.imread(p) for p in frame_paths]
out = plots + 'particle_animation.gif'
imageio.mimsave(out, frames, duration=0.1)
print('wrote', out, 'with', len(frames), 'frames')
