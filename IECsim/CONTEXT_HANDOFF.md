# IEC Fusor Simulation — Handoff Context

## Project goal
3D self-consistent Vlasov–Poisson simulation of a DN63-tee IEC fusor using IBSimu v1.0.6dev. Search for double-well / virtual-cathode formation. The standard `CONTEXT.md` has the original technical scope; THIS file captures everything since.

---

## Currently running (check status with `ps -p <PID>`)
- **run_io_50A** (PID 245365) — `fusorsim_ions_only`, -100 kV / 50 A ions only, 5-iter cap, α=0.3 under-relaxation. Monitor `bacbi5hyr`.
- **fusor_with_anode** (PID 244866) — Laplace-only solve at -10 kV with anode grid, built from cad_70_transp STL files. Monitor `bh2kske3d`.

Both write to `run_io_50A/` and `run_with_anode/` respectively.

---

## Repo layout (only the bits that matter now)

```
~/personal_projects/IECsim/
├── fusorsim.cpp                  ← standard sim (surface source, ions+electrons, under-relax α=0.3, j≤5 cap)
├── fusorsim_non_sphere.cpp       ← volumetric source, ions+electrons (slow-ion gas-discharge analogue)
├── fusorsim_ions_only.cpp        ← surface source ions ONLY (no electrons) — currently used for io sweep
├── fusor_no_anode.cpp            ← geometry + Laplace ONLY, no anode grid
├── fusor_with_anode.cpp          ← geometry + Laplace ONLY, with anode (matches no_anode using SAME STL files)
├── geom.dat                      ← standard mesh (with anode). Old: built with "dn63 tee.stl" etc.
├── geom_cat_only.dat             ← no-anode mesh
├── geom_with_anode.dat           ← matching with-anode mesh from cad_70_transp STL files
├── cad_70_transp/                ← STL files in use now: tee_or.stl, cathodegridnostalk_or.stl, anodegrid_or.stl
├── voltage sweep runs/           ← run12–run18 (5 mA voltage sweep at -10→-100 kV)
│   └── voltage_sweep_comparison.docx
├── current sweep 100kV/          ← run18 baseline + run19–run23 (current sweep at -100 kV, surface source, ions+electrons)
│   └── current_sweep_comparison.docx + height_vs_K.png
├── testing runs/                 ← run1–run11 (older, ignore)
├── run_ns_10kV_10A/              ← volumetric source -10 kV / 10 A (has scharge_*.png)
├── run_ns_25kV_10A/              ← volumetric source -25 kV / 10 A (has scharge_*.png)
├── run_ns_10kV_10A_old/, run_ns_25kV_10A_old/  ← archived pre-scharge versions
├── run_io_2mA/ … run_io_10A/     ← ions-only at -100 kV — DONE, has scharge_*.png
├── run_io_50A/                   ← ions-only -100 kV / 50 A — RUNNING (PID 245365)
├── run_io_*_no_scharge/          ← archived earlier io sweep without scharge plotting; ignore
├── run_no_anode/                 ← no-anode Laplace + compare plots (FILES CURRENTLY RIGHT after rerun)
├── run_no_anode_old/             ← earlier no-anode run with file-name mismatch — ignore
├── run_with_anode/               ← matched-STL with-anode Laplace — IN PROGRESS
├── run24_diagnostic_report.docx  ← bistability + under-relaxation diagnostic doc (good content)
├── run24_oscillation.png, run24_zoom.png  ← bistable -1 kV / 10 A plots
├── run_no_anode/radial_compare_{x,y,z}.png       ← 1D radial overlays comparing no-anode vs with-anode (-10 kV vacuum). VALID.
├── run_no_anode/epot_compare_{xy,xz,yz_x0}.png   ← 2D side-by-side. CURRENTLY suspect because old standard geom.dat used different STL files than the no-anode rerun. Will be regenerated against run_with_anode/ once it finishes.
└── pot0plots/all_vacc_{x,y,z}.png  ← initial vacuum radial overlay at all voltages tested (1,5,10,20,25,30,40,50,75,100 kV). VALID.
```

---

## Workflow rules — MANDATORY ORDER

1. Edit cathodepot / beam_current / `string run = "runN/"` in the cpp file.
2. `./setrun.sh runN` — ONLY edits `fusorsim.cpp`. For `fusorsim_non_sphere.cpp` / `fusorsim_ions_only.cpp` / `fusor_no_anode.cpp` / `fusor_with_anode.cpp` use `sed -i` directly OR edit by hand.
3. `mkdir -p runN/plots runN/timestep_pics runN/pout`
4. `rm -f <binary>; make <binary>` — ALWAYS rm before make (avoids stale x86 vs ARM binary issues)
5. `nohup ./<binary> > runN/fusorsim.log 2>&1 &` then capture `$!` as PID
6. Use Monitor tool to watch for "Ending simulation" + PID exit.

After the run:
```bash
for s in pot_radial_all.py pot_error.py centralheight.py memory_estimate.py trajdens_combined.py; do
    sed -i "s|run = '[^']*'|run = '${RUN}/'|" postprocessscripts/$s
done
cp <binary>.cpp ${RUN}/fusorsimrun${RUN}.cpp
python postprocessscripts/pot_radial_all.py
python postprocessscripts/pot_error.py
python postprocessscripts/centralheight.py
python postprocessscripts/memory_estimate.py
python postprocessscripts/build_run_docx.py ${RUN}
```

---

## Code changes already in place

### All cpp files (current state)
- **Loop**: `int converged_count = 0; while (j <= 5)` — j≤5 cap (was j≤10, then j≤20, now j≤5 for fast turnaround)
- **Under-relaxation**: `epot(a) = alpha * epot(a) + (1.0 - alpha) * epot_old(a)` with `const double alpha = 0.3;`. Applied after `solver.solve()` and BEFORE `efield.recalculate()`. Only for j>1.
- **3-converged-iter rule**: `converged_count` tracks consecutive sub-threshold iters; loop breaks when count reaches 3.

### fusorsim_non_sphere.cpp
- New `sample_volume(rad, geom)` does rejection sampling in a sphere of radius rad (rejects outside sphere AND inside mesh-solid nodes). Both species sampled at `anode_r` volumetrically (full ball INCLUDING the core r<cathode_r).
- Has scharge plot block (writes `scharge_xy.png`, `scharge_xz.png`, `scharge_yz_x0.png`).

### fusorsim_ions_only.cpp
- All electron-related code commented out (no `beam_current_e`, no `pdb_e`, no `add_electrons`, no electron trajectory plotting block).
- HAS scharge plot block.

### fusor_no_anode.cpp / fusor_with_anode.cpp
- Build geometry inline from cad_70_transp STL files (`tee_or.stl`, `cathodegridnostalk_or.stl`, plus `anodegrid_or.stl` for with-anode).
- Single Laplace solve only, no trajectories, no electrons.
- Save geom to `geom_cat_only.dat` / `geom_with_anode.dat`.
- Plot epot_{xy,xz,yz_x0}_{no,with}_anode.png and radial profile .dat files.
- `cathodepot = -10000.0` (-10 kV for direct comparison with voltage-sweep run12).

---

## Plotting rules / conventions (consistent across ALL sim files)

- **GeomPlotter set_size**: 2048 x 2048
- **set_ranges**: -0.05, -0.05, 0.05, 0.05 for every view (m)
- **VIEW_XY, -1**: looking down z-axis (long tube). Filename ends in `_xy.png`
- **VIEW_XZ, -1**: looking down y-axis (vertical tube). Filename `_xz.png`
- **VIEW_YZ, 105**: looking down x-axis (side tube). Filename `_yz_x0.png`
- **set_particle_div(1000, 0)**: every 1000th particle drawn in trajdens
- **set_qm_discretation(false)** for ions, **(true)** for electrons → different IBSimu color cycle index
- **Combined trajdens (red ions, blue electrons, magenta overlap)** generated postprocess by `postprocessscripts/trajdens_combined.py` — reads PNGs, composites via PIL into RGB. Output: `runN/plots/trajdens_combined_*.png`

Coordinate axes (per CONTEXT.md):
- x = side tube
- y = vertical tube along long tube's diameter
- z = long tube (axial)

---

## KEY RESULTS SUMMARY

### 1. Voltage sweep at 2 mA (runs 12–18 in `voltage sweep runs/`)
- Voltages: -10, -20, -30, -40, -50, -75, -100 kV
- **6.864% geometric baseline** — V(r=0) = 0.93136 × V_cathode at EVERY voltage. Pure geometric effect (cathode grid transparency, no space-charge contribution).
- Converged in 2 iter (electron+ion space charge cancels at this current).

### 2. Current sweep at -100 kV (runs 18–23 in `current sweep 100kV/`)
- Currents: 2 mA, 10 mA, 100 mA, 1 A, 10 A, 50 A — perveance K = 0.002 → 50 mA/kV^1.5
- V(r=0) drift: 0V → 9V → 105V → 495V — small drops (more negative), not the predicted virtual anode
- **No double well formed even at K = 147× Gu-Miley threshold.** Likely because tee geometry lets ions escape down side tubes before central density can build up; AND because the standard electron source (at rest at cathode shell) dumps excessive negative space charge that counteracts ion-induced positive shift.

### 3. -1 kV / 10 A bistable oscillation (run24_no_relax)
- Period-2 limit cycle: V(r=0) ∈ {-1228 V (shallow), -4940 V (deep)}
- Confirmed numerically: V(r=0) can be more negative than V_cathode via Poisson with negative ρ; Gauss-shell theorem explains why electron shell at cathode dominates over fast-ion focus at r=0.
- Diagnostic report: `run24_diagnostic_report.docx`
- Bistability damped by α=0.3 under-relaxation (run24 with relax shows V settling ~-1700 V, no period-2)

### 4. Volumetric source (runs run_ns_*) at -10 kV and -25 kV / 10 A
- V_center shift -140 V (10 kV) and -94 V (25 kV) — SMALLER than surface source (run25 was -334 V at same params)
- Slow ions inside the cathode DO contribute positive space charge → partially cancels electron-shell negative contribution. Mechanism confirmed in direction but insufficient magnitude to flip sign.

### 5. Ions-only current sweep at -100 kV (CURRENT WORK, in `run_io_*`)
- 2 mA: V drift +6 V
- 10 mA: +60 V
- 100 mA: +793 V
- 1 A: +5371 V (settling around -87.8 kV)
- **10 A: V(r=0) oscillating ~-72 to -81 kV — 15-22 kV upward shift**
- **50 A: V(r=0) iter 2 = +15,067 V — POSITIVE! Unambiguous virtual anode.** Subsequent iters oscillate down (-17, -39, -55 kV) due to insufficient damping at α=0.3 at this current. Errors ~193 kV at iter 2 → 23 kV at iter 5. NOT converged but virtual-anode existence proven.

**Physical interpretation (user's framing)**: removing electrons reveals that ion-only space charge produces a SIGNIFICANT positive shift in V(r=0). The standard simulations underestimated the virtual-anode physics because **electrons launched at rest at the cathode shell deposit a huge amount of negative space charge in a thin spherical shell at low velocity (high residence time per cell), flattening the central potential**. Fast ions have very short residence time at r=0 → small positive contribution. The asymmetry of electron-stationary-at-cathode vs ion-fast-at-center is the root cause.

### 6. No-anode geometry (run_no_anode)
- Vacuum Laplace at -10 kV WITHOUT anode grid
- V(r=0) = -9376.5 V (vs -9313.6 V with anode) — **63 V more negative without anode**
- Height (no anode): 6.235%; height (with anode): 6.864%. Anode grid acts as a Dirichlet 0V constraint that "blunts" the cathode field at outer radii.
- Comparison: `run_no_anode/radial_compare_{x,y,z}.png` valid. 2D `epot_compare_*` plots are CURRENTLY in `run_no_anode/` but **suspect** because the with-anode reference (`voltage sweep runs/run12/epot_*_initial.png`) was generated from old `geom.dat` built with different STL files than the no-anode rerun. Regenerate once `run_with_anode/` finishes (uses identical cad_70_transp STL files).

---

## Outstanding work

1. **run_io_50A** (in progress) — wait for completion, postprocess, check V(r=0).
2. **fusor_with_anode** (in progress) — wait, then regenerate `run_no_anode/epot_compare_*.png` using `run_with_anode/` as the reference (same STL files for both → guaranteed apples-to-apples comparison).
3. **Compile final report docx**: user wants a complete document with results when 50A finishes. Should include:
   - Physical interpretation (electron-stationary-at-cathode dominates ρ → suppresses ion virtual anode)
   - All io sweep numbers + scharge maps
   - Comparison to original current sweep (with electrons)
   - Vacuum no-anode vs with-anode comparison
4. **Optional**: re-do ions-only with volumetric source for ions to test if launching ions at rest throughout the volume helps further (slow-ion population is the missing ingredient).

---

## Permissions config

`.claude/settings.local.json` allows `Bash`, `Edit`, `Write`, `Monitor`, `TodoWrite`, `Read` with deny list for `rm -rf /*`, `rm -rf ~/*`, `git push --force*`, `sudo *`. Already in place; no permission prompts.

---

## Caveats to know

- `setrun.sh` only edits `fusorsim.cpp` — does NOT edit `fusorsim_non_sphere.cpp`, `fusorsim_ions_only.cpp`, `fusor_no_anode.cpp`, `fusor_with_anode.cpp`. Use sed directly:
  ```bash
  sed -i 's|^const double beam_current = .*|const double beam_current = X;   // ...|' <cpp>
  sed -i 's|^    string run = .*|    string run = "runN/";|' <cpp>
  sed -i 's|^double cathodepot = .*|double cathodepot = -X.0;             // -X kV cathode|' <cpp>
  ```
- `postprocess.sh` invokes `cp fusorsim.cpp` — so for non-fusorsim binaries copy the cpp manually:
  `cp <binary>.cpp runN/fusorsim_run<runN>.cpp`
- MC noise floor with 800K particles is ~3-17 V regardless of voltage — at -1 kV this exceeds the 1 V threshold, so j=5 cap will trigger before 3-converged-iter rule. Not a bug — physical noise.
- Memory peaked ~9 GB for ions+electrons at -100 kV, ~40 GB at -1 kV (slow particles = long trajectories = high cell-residence memory).
- `build_trajectory_density_field` and `iterate_trajectories` are **time-independent steady-state**; IQ = current in A, scharge deposition is along trajectories weighted by residence time. Per-particle simulation time IS tracked in `particle[0]` (the t component of ParticleP3D state).

---

## Final document writing pattern

User's preferred docx style: minimal text, embed plots with captions, tables for numerical data, italicized notes for caveats. See `run24_diagnostic_report.docx` as a good template.
