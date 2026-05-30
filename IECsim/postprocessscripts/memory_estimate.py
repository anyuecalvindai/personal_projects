import os
import re

run = 'run_e_anode_10A/'


def parse_n_clouds(cpp_path):
    n = None
    n_e = None
    with open(cpp_path) as f:
        for line in f:
            m = re.match(r"\s*const\s+long\s+N_clouds\s*=\s*(\d+)\s*;", line)
            if m:
                n = int(m.group(1))
                continue
            m = re.match(r"\s*const\s+long\s+N_clouds_e\s*=\s*(\S+?)\s*;", line)
            if m:
                tok = m.group(1).strip()
                if tok == "N_clouds":
                    n_e = n
                else:
                    try:
                        n_e = int(tok)
                    except ValueError:
                        n_e = None
    return n, n_e


def parse_ion_e_steps(log_path):
    ion_steps = []
    e_steps = []
    pair = []
    pat = re.compile(r"\s*total\s+steps\s*=\s*(\d+)")
    with open(log_path) as f:
        for line in f:
            m = pat.match(line)
            if m:
                pair.append(int(m.group(1)))
                if len(pair) == 2:
                    ion_steps.append(pair[0])
                    e_steps.append(pair[1])
                    pair = []
    return ion_steps, e_steps


cpp_path = None
for fname in sorted(os.listdir(run)):
    if fname.startswith("fusorsim") and fname.endswith(".cpp"):
        cpp_path = os.path.join(run, fname)
        break
if cpp_path is None:
    cpp_path = "fusorsim.cpp"

N_clouds, N_clouds_e = parse_n_clouds(cpp_path)
if N_clouds is None:
    N_clouds = 800000
if N_clouds_e is None:
    N_clouds_e = N_clouds

ion_steps, e_steps = parse_ion_e_steps(run + "fusorsim.log")
if not ion_steps:
    # Fall back to trajectory_steps.dat (ion-only) if log not parseable
    import numpy as np
    data = np.loadtxt(run + "trajectory_steps.dat", comments="#", dtype=np.int64)
    if data.ndim == 1:
        data = data.reshape(1, -1)
    ion_steps = data[:, 1].tolist()
    e_steps = [0] * len(ion_steps)

bytes_per_particle = 108
bytes_per_step = 56
bytes_per_double = 8

# Mesh: 265 * 320 * 210 = 17,808,000 nodes (h=0.3mm, see fusorsim.cpp)
N_nodes = 265 * 320 * 210
mesh_scalar_fields = 4  # epot, epot_old, scharge, tdens
mesh_vector_components = 3  # efield (3 components)
mesh_bytes = (mesh_scalar_fields + mesh_vector_components) * N_nodes * bytes_per_double

# BiCGSTAB-ILU0 solver workspace estimate:
#   ~7 Krylov-like vectors of size N + ILU0 factors for a 7-point 3D Laplacian stencil
n_krylov = 7
ilu0_nz_factor = 7
solver_bytes = (n_krylov + ilu0_nz_factor) * N_nodes * bytes_per_double

# Geometry (mesh int IDs + boundary metadata)
geom_bytes = N_nodes * 4

constant_overhead_bytes = mesh_bytes + solver_bytes + geom_bytes

particles_bytes = (N_clouds + N_clouds_e) * bytes_per_particle

os.makedirs(run + "plots", exist_ok=True)
with open(run + "plots/memory_estimate.txt", "w") as f:
    f.write(f"# Memory breakdown for {run}\n")
    f.write(f"# N_clouds (ions)     = {N_clouds}\n")
    f.write(f"# N_clouds_e (electrons) = {N_clouds_e}\n")
    f.write(f"# bytes/particle = {bytes_per_particle}, bytes/step = {bytes_per_step}\n")
    f.write(f"# Mesh nodes = {N_nodes} ({mesh_scalar_fields} scalar + 1 vector(3) fields)\n")
    f.write(f"# Mesh-field memory     = {mesh_bytes/1024**3:.3f} GB\n")
    f.write(f"# Solver workspace est. = {solver_bytes/1024**3:.3f} GB\n")
    f.write(f"# Geometry est.         = {geom_bytes/1024**3:.3f} GB\n")
    f.write(f"# Constant overhead total = {constant_overhead_bytes/1024**3:.3f} GB\n")
    f.write(f"# Particle data (constant)  = {particles_bytes/1024**3:.3f} GB\n")
    f.write("#\n")
    f.write("# iter  ion_steps  e_steps  traj_MB  overhead_MB  total_GB\n")
    for i, (its, ets) in enumerate(zip(ion_steps, e_steps), start=1):
        traj_bytes = (its + ets) * bytes_per_step
        total_bytes = particles_bytes + traj_bytes + constant_overhead_bytes
        f.write(
            f"{i:6d} {its:12d} {ets:12d} "
            f"{traj_bytes/1024**2:10.2f} "
            f"{(particles_bytes + constant_overhead_bytes)/1024**2:12.2f} "
            f"{total_bytes/1024**3:9.4f}\n"
        )
