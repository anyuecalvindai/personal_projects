# IBSimu IEC Fusor Simulation — Session Context
# Read this file before doing anything. Ask all clarifying questions before starting any workflow.

---

## Project Overview

This is a 3D self-consistent Vlasov-Poisson simulation of an IEC (Inertial Electrostatic Confinement) fusor using IBSimu v1.0.6dev (89c1280). The physical device is a DN63 conflat tee with spherical cathode and anode wire grids. The simulation goal is to characterise the electrostatic potential well structure and search for double well / virtual cathode formation at Gu & Miley (2000) parameters.

---

## File Structure

```
~/personal_projects/IECsim/
├── fusorsim.cpp          ← MAIN SIMULATION FILE. All edits go here.
├── geom.dat              ← Pre-built geometry (DO NOT DELETE OR REGENERATE)
├── Makefile
├── setrun.sh             ← Sets run directory, creates subdirs, updates sed in all scripts
├── runsim.sh             ← rm fusorsim + make + nohup run
├── postprocess.sh        ← Runs all Python postprocessing scripts
├── venv/                 ← Python virtual environment (activate before postprocess)
├── postprocessscripts/   ← All Python postprocessing scripts
│   ├── pot_radial_all.py
│   ├── pot_error.py
│   ├── traj_distribution.py
│   ├── centralheight.py
│   ├── epot_error_rms.py
│   ├── memory_estimate.py
│   ├── timestep_animation.py
│   └── particle_animation.py
├── cad_70_transp/        ← STL files (already committed to git)
├── run1/ ... run11/      ← Completed run directories. DO NOT TOUCH.
└── run12/ ...            ← Future runs
```

**The correct path for fusorsim.cpp is `~/personal_projects/IECsim/fusorsim.cpp`. There is no /cad subdirectory for this file.**

---

## Geometry

- Mesh: h = 0.0003m, 265×320×210 = 17,808,000 nodes
- Origin: Vec3D(-0.03175, -0.048, -0.03175)
- Centre nodes: cx=105, cy=160, cz=105
- STL solids:
  - Solid 7: tee chamber wall (0V, Neumann on box boundaries 1-6)
  - Solid 8: cathode grid (~5.5mm radius, DIRICHLET = cathodepot)
  - Solid 9: anode grid (~19.95mm radius, DIRICHLET = 0V)
- geom.dat is pre-built and loaded from file — never regenerate it

---

## Mandatory Workflow — Follow Exactly

For every run:

```bash
# Step 1: Edit only permitted parameters in fusorsim.cpp (see below)
# Step 2:
./setrun.sh runN          # updates string run, creates runN/ runN/plots/ runN/timestep_pics/ runN/pout/

# Step 3: CRITICAL — ALWAYS delete binary first
rm fusorsim
make fusorsim             # compiles fresh x86 binary

# Step 4: Verify run string before committing
grep 'string run' fusorsim.cpp   # must show runN

# Step 5: Run with nohup
nohup ./fusorsim 2>&1 | tee runN/fusorsim.log &

# Step 6: Wait for completion
tail -f runN/fusorsim.log        # wait for "Ending simulation"

# Step 7: Postprocess
source venv/bin/activate
./postprocess.sh runN
```

### Why rm fusorsim is mandatory

This VM is x86. Code is sometimes edited on a local ARM Mac. If a stale ARM binary exists, make detects no source changes and skips recompilation, silently running the wrong binary. Output goes to the wrong run directory and the entire compute run is wasted. Always delete first.

---

## fusorsim.cpp — What You May and May Not Change

### MAY change (for parameter sweeps):
- cathodepot value and its inline comment
- beam_current and beam_current_e values and their inline comments
- N_clouds and N_clouds_e values
- n_iter value
- string run value (but use setrun.sh instead)

### MUST NOT change under any circumstances:
- Any include statements
- Any function signatures or bodies (sample, add_ions, add_electrons, epot_max_error, snapshot)
- Any geometry parameters (h, sim_x, sim_y, sim_z, anode_r, cathode_r, cx, cy, cz)
- The while loop condition structure
- Any output file names or paths
- Any plotting code (gplotter calls)
- Any radial potential scan loops
- Any comments not immediately adjacent to a changed parameter
- The geom.build_surface() call
- The geom.set_boundary() call (it already references cathodepot variable)

---

## Key Parameters

```cpp
const double beam_current = 0.002;      // 2mA ions
const double beam_current_e = 0.002;    // 2mA electrons
const long N_clouds = 100000;
const long N_clouds_e = 100000;
const double m = 2.0141017778;          // deuterium
const double m_e = 0.000548579909;      // electron mass in amu
double cathodepot = -5000.0;            // cathode voltage
double anode_r = 15*1.33 * 1e-3;       // anode radius ~19.95mm
const double cathode_r = 0.005;         // cathode radius 5mm
int n_iter = 20;
double error_thresh = 5.0;              // volts convergence threshold
```

---

## Run History

| Run | Voltage | Current | N_ions | N_electrons | Notes |
|-----|---------|---------|--------|-------------|-------|
| run1-4 | -5kV | 2mA | 100K | none | Early development, various bugs |
| run5 | -5kV | 2mA | 100K | none | 40 iter, ions only, scharge bug present, vacuum field approx |
| run6 | -5kV | 2mA | 100M | none | Crashed OOM |
| run7 | -5kV | 2mA | 100K | none | Ions only, scharge bug present |
| run8 | -5kV | 2mA | 100K | none | PIC time stepping + animation test |
| run9 | -5kV | 2mA | 100K | 100K | scharge bug fixed, ions + electrons, 20 iter, ~135s |
| run10 | -5kV | 2mA | 100K | 100K | Same as run9, manually run |
| run11 | -5kV | 2mA | 800K | 800K | MEMORY TEST — verify 800K fits in 64GB before voltage sweep |

run12 onwards: voltage sweep at constant 2mA, 800K particles (pending run11 confirmation)

---

## Known Bugs and Fixes

### scharge bug (FIXED in run9+)
Earlier runs (run1-run7) did NOT call scharge.clear() before particle deposition. Space charge accumulated across iterations. Current correct order:

```cpp
solver.solve(epot, scharge);
efield.recalculate();
scharge.clear();
pdb.clear();
pdb_e.clear();
add_ions(pdb, geom);
add_electrons(pdb_e, geom);
pdb.iterate_trajectories(scharge, efield, bfield);
pdb_e.iterate_trajectories(scharge, efield, bfield);
```

### ARM/x86 binary mismatch
Always rm fusorsim before make fusorsim.

---

## Physics Context

### Goal
Find double potential well (virtual cathode) structure observed by Gu & Miley (2000) at high perveance. No double well found at 5kV/2mA — K = 1.79e-4 mA/kV^(3/2), far below threshold K > 0.34.

### Sweep plan
- run11: 800K particle memory test at 5kV/2mA
- run12-18: Voltage sweep 10kV to 100kV at constant 2mA, 800K particles
- Future: current sweep, then combined Gu & Miley parameters (80mA, 15kV)

### Key references
- Gu & Miley (2000) IEEE Trans. Plasma Sci. 28(1)
- Thorson et al. (1997) Phys. Plasmas 4(1)
- Rider (1995, 1997) Phys. Plasmas
- Kalvas et al. (2010) Rev. Sci. Instrum. — IBSimu

---

## Solver

BiCGSTAB-ILU0 (EpotBiCGSTABSolver) with eps=0.05, imax=10000. Do not change.

---

## Memory Constraints (64GB VM)

| Configuration | Total Memory |
|--------------|--------------|
| 100K ions + 100K electrons | ~7.3 GB |
| 800K ions + 800K electrons (maxsteps=1000) | ~46.4 GB |
| 900K ions + 900K electrons | ~56.4 GB |
| 1M ions + 1M electrons | ~62.5 GB — too risky |

IBSimu default _maxsteps = 1000 caps electron trajectories.

---

## Skill Files (ALWAYS CHECK BEFORE CREATING OUTPUT FILES)

Skill files at /mnt/skills/public/ encode best practices for file creation. Always read the relevant SKILL.md before creating any document. Available:
- /mnt/skills/public/docx/SKILL.md — Word documents
- /mnt/skills/public/pdf/SKILL.md — PDFs
- /mnt/skills/public/xlsx/SKILL.md — Spreadsheets
- /mnt/skills/public/pptx/SKILL.md — Presentations

---

## Git

Remote: https://github.com/anyuecalvindai/personal_projects.git
Auth: personal access token as password
Credential helper: git config --global credential.helper store

---

## Python Environment

source ~/personal_projects/IECsim/venv/bin/activate
Packages: pandas, numpy, matplotlib, scipy, pillow, imageio, python-docx

---

## IBSimu Installation

Prefix: /home/calvindai1234/
Headers: /home/calvindai1234/include/ibsimu-1.0.6dev/
Library: /home/calvindai1234/lib/libibsimu-1.0.6dev.a
PKG_CONFIG_PATH and LD_LIBRARY_PATH in ~/.bashrc
