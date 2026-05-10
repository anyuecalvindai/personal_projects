import os
import re
from glob import glob
import imageio.v2 as imageio

run = 'run8/'
plots = run + 'plots/'

files = glob(run + 'timestep_epot_xz_*.png')

def step_num(p):
    m = re.search(r'timestep_epot_xz_(\d+)\.png', os.path.basename(p))
    return int(m.group(1)) if m else -1

files = sorted(files, key=step_num)

if not files:
    raise SystemExit('no timestep_epot_xz_*.png files found in ' + run)

os.makedirs(plots, exist_ok=True)

frames = [imageio.imread(f) for f in files]
out = plots + 'timestep_animation.gif'
imageio.mimsave(out, frames, duration=0.1)
print('wrote', out, 'with', len(frames), 'frames')
