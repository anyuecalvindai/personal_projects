# Claude Code Prompt — Current Sweep Runs at -100kV

## First Steps

1. Read `~/personal_projects/IECsim/CONTEXT.md` before doing anything else.
2. **If you have ANY doubt about the workflow, file structure, compilation process, or postprocessing — ask now before starting.** It is completely fine to ask questions. It is NOT fine to make assumptions and proceed. Each incorrect run wastes significant cloud compute time and money.
3. Do not hallucinate file paths, function names, or parameter names. If a file is not where expected, report it rather than guessing.

---

## Context

The voltage sweep (runs 12–18) is complete and confirmed approximately linear vacuum field scaling at 2mA across all voltages — convergence in 2 iterations with identical virtual anode height fraction at every voltage. This confirms space charge is negligible at low current. The cathode voltage is now fixed at -100kV for a current sweep spanning 4 orders of magnitude to find the perveance threshold for double well formation.

Perveance K = I/V^(3/2) at -100kV (V^(3/2) = 1000):

| Run | Current | K (mA/kV^(3/2)) | vs Gu & Miley threshold (0.34) |
|-----|---------|-----------------|--------------------------------|
| run18 (done) | 2mA | 0.002 | 170× below |
| run19 | 10mA | 0.01 | 34× below |
| run20 | 100mA | 0.1 | 3.4× below |
| run21 | 1A | 1.0 | 3× above |
| run22 | 10A | 10.0 | 29× above |
| run23 | 50A | 50.0 | 147× above |

Threshold crossing expected between run20 and run21. Double well physics expected from run21 onwards.

---

## Paths

- `~/personal_projects/IECsim/fusorsim.cpp` — main simulation file
- **There is no /cad subdirectory. Do not look anywhere else.**

---

## Mandatory Workflow — Execute Exactly

For every run:

```bash
# Step 1: Edit only permitted parameters (see below)
# Step 2:
./setrun.sh runN

# Step 3: MANDATORY — always delete binary first
rm fusorsim
make fusorsim

# Step 4: Verify run string before running
grep 'string run' fusorsim.cpp   # must show runN

# Step 5: Run with nohup
nohup ./fusorsim 2>&1 | tee runN/fusorsim.log &

# Step 6: Wait for "Ending simulation" in log before next run
tail -f runN/fusorsim.log

# Step 7: Postprocess
source venv/bin/activate
./postprocess.sh runN
```

**If a run crashes, STOP and report. Do not continue to the next run.**

### Why `rm fusorsim` is mandatory
Without it, `make` may silently use a stale binary compiled on a different architecture, sending all output to the wrong run directory and wasting the entire compute run.

---

## fusorsim.cpp — STRICT Change Rules

**ONLY change `beam_current`, `beam_current_e`, and their inline comments. Nothing else.**

For each run, set both `beam_current` and `beam_current_e` to the same value (matched ion and electron currents):

| Run | beam_current | beam_current_e | Comment |
|-----|--------------|----------------|---------|
| run19 | 0.010 | 0.010 | // 10mA |
| run20 | 0.100 | 0.100 | // 100mA |
| run21 | 1.000 | 1.000 | // 1A |
| run22 | 10.000 | 10.000 | // 10A |
| run23 | 50.000 | 50.000 | // 50A |

DO NOT change:
- `cathodepot` (already -100000.0 from voltage sweep)
- `N_clouds`, `N_clouds_e` (both 800000 from voltage sweep — confirmed memory-safe)
- `m`, `m_e`, `q`
- Any geometry parameter (h, sim_x, sim_y, sim_z, anode_r, cathode_r, cx, cy, cz)
- `n_iter` (20)
- The error threshold formula (`fabs(cathodepot) * 0.001`)
- Any include, function, loop, output path, plotting code, or comment unrelated to the changed parameters

---

## Completed Runs — DO NOT TOUCH

Runs 1–18 are complete. Do not modify any run directory or any file from those runs.

---

## Post-Processing: Per-Run Word Document

After each run completes and postprocess.sh finishes, create `runN/runN_summary.docx`.

**Before writing any code: read `/mnt/skills/public/docx/SKILL.md` and follow it.**

Each document contains (minimal text, just labels):

1. Title: `Run N — Current Sweep: [current], -100kV, 800K particles, Deuterium`
2. **Radial Potential Profiles** — generate matplotlib PNG from `runN/potential_radial_z_all.dat` showing all iterations overlaid, save to `runN/plots/radial_potential_overlay.png` then embed
3. **Final Potential Field** — embed `runN/epot_xz.png`, `runN/epot_xy.png`, `runN/epot_yz_x0.png`
4. **Trajectory Density** — embed `runN/trajdens_xz.png`, `runN/trajdens_xy.png` (and ion/electron/combined variants if available)
5. **Memory Estimate** — embed `runN/plots/memory_table.png` if it exists
6. **Convergence** — embed `runN/plots/epot_error_plot.png` if it exists

---

## Post-Processing: Bundle Comparison Document

After ALL runs 19–23 complete and all per-run documents are done, create `current_sweep_comparison.docx` at `~/personal_projects/IECsim/`.

**Before writing any code: read `/mnt/skills/public/docx/SKILL.md`.**

Contents:

### Section 1: Overview Table
Columns: Run, Current (A), Perveance K, Converged (Y/N), Iterations to converge, Final Epot error (V), Avg trajectory steps, Virtual anode height (% of cathode pot). Pull data from each run's `epot_error.dat`, `trajectory_steps.dat`, and `potential_radial_z_all.dat` (final iteration).

### Section 2: Superimposed Radial Potential Profiles
Single matplotlib figure showing final converged radial potential profile (z-axis) for all 6 runs (including run18 at 2mA for reference) on the same axes. Use a sequential colormap (e.g. plasma) ordered by current. Save as `current_sweep_radial_overlay.png` then embed. Figure should have:
- x-axis: radial distance (m), centred at 0
- y-axis: potential (V)
- legend showing each current
- title: "Radial Potential Profile — Current Sweep, -100kV Deuterium"
- vertical dashed lines marking cathode radius (0.005m) and anode radius (0.01995m)

### Section 3: Virtual Anode Height vs Current
Single matplotlib figure showing virtual anode height (potential maximum between cathode and anode, as % of cathode potential) vs current on log-log axes. Mark Gu & Miley threshold K = 0.34 mA/kV^(3/2) with a vertical line. This is the key figure for showing space charge transition.

### Section 4: Superimposed Convergence
Single figure showing epot max error vs iteration for all runs on same axes, labelled by current.

### Section 5: Trajectory Density Comparison
2-column grid of `runN/trajdens_xz.png` for each run, labelled by current.

### Section 6: Final Potential Field Comparison
2-column grid of `runN/epot_xz.png` for each run, labelled by current.

### Section 7: Memory Summary
Table showing memory usage across all runs.

### Section 8: Perveance Analysis
Brief table showing perveance K for each run and whether it exceeds the Gu & Miley threshold. Highlight runs where double well structure appears.

---

## Install if Needed

```bash
source venv/bin/activate
pip install python-docx matplotlib numpy pandas
```

---

## Final Note

The high-current runs (1A, 10A, 50A) may have dramatically different convergence behaviour than the low-current runs. Monitor the first high-current run carefully before leaving the next runs to complete overnight. If convergence is unstable or memory issues appear, STOP and report rather than continuing.
