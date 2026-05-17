"""Composite trajdens_ions_*.png and trajdens_electrons_*.png into combined
RGB overlays. Ion intensity -> red channel, electron intensity -> blue channel.

White background pixels become black in the composite (no ion/electron there);
red areas = ion only, blue = electron only, magenta = both.

Writes runN/plots/trajdens_combined_{xy,xz,yz_x0}.png.
"""
import os
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from PIL import Image

run = 'run_io_10mA/'
plots_dir = run + 'plots/'
os.makedirs(plots_dir, exist_ok=True)

views = ['xy', 'xz', 'yz_x0']

for view in views:
    ion_path = f'{run}trajdens_ions_{view}.png'
    e_path = f'{run}trajdens_electrons_{view}.png'

    if not os.path.exists(ion_path) or not os.path.exists(e_path):
        print(f'  skip {view}: missing {ion_path} or {e_path}')
        continue

    ion_gray = np.array(Image.open(ion_path).convert('L'))
    e_gray = np.array(Image.open(e_path).convert('L'))

    if ion_gray.shape != e_gray.shape:
        e_img = Image.open(e_path).convert('L').resize(
            (ion_gray.shape[1], ion_gray.shape[0]))
        e_gray = np.array(e_img)

    # Invert so 0 = background (white), 255 = full density (originally dark)
    ion_intensity = 255 - ion_gray
    e_intensity = 255 - e_gray

    h, w = ion_intensity.shape
    composite = np.zeros((h, w, 3), dtype=np.uint8)
    composite[..., 0] = ion_intensity  # R = ions
    composite[..., 2] = e_intensity    # B = electrons
    # G left at 0; overlap shows up as magenta

    out_path = f'{plots_dir}trajdens_combined_{view}.png'
    fig, ax = plt.subplots(figsize=(8, 8))
    ax.imshow(composite, interpolation='nearest')
    ax.set_xticks([])
    ax.set_yticks([])
    ax.set_title(f'Trajectory density - ions (red) + electrons (blue), {view}')
    fig.tight_layout()
    fig.savefig(out_path, dpi=150, bbox_inches='tight', pad_inches=0.1)
    plt.close(fig)
    print(f'  wrote {out_path}')
