import pandas as pd

run = 'run5/'
plots = run + 'plots/'

axes = ['x', 'y', 'z']
#plucks epot values at r=0 for all xyz, averages. i.e. will be the average virtual anode height or average virtual cathode depth.
with open(plots + 'centre_potential.txt', 'w') as f:
    for axis in axes:
        df = pd.read_csv(run + f'potential_radial_{axis}_all.dat', sep=r'\s+', comment='#',
                         header=None, names=['iteration', 'r', 'potential'])
        r0 = df[df['r'].abs() < 1e-10]#may not be ex
        avg = r0['potential'].mean()
        f.write(f'{axis} axis: average potential at r=0 = {avg:.4f} V\n')
        f.write(r0[['iteration', 'potential']].to_string(index=False))
        f.write('\n\n')