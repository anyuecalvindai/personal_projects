# Claude Code Prompt — Trajectory Plotting Improvements

## Context
Read `~/personal_projects/IECsim/CONTEXT.md` first.

**Before writing any code: verify all method names from the actual header files at `/home/calvindai1234/include/ibsimu-1.0.6dev/`. Do not guess method names.**

Relevant headers to check:
- `geomplotter.hpp`
- `geomplot.hpp`
- `particlegraph.hpp`

---

## Task

Modify the plotting section in `~/personal_projects/IECsim/fusorsim.cpp` to improve trajectory visualisation.

Add to includes:
```cpp
#include "particlegraph.hpp"
```

Change all existing `gplotter.set_particle_div(1)` calls to `gplotter.set_particle_div(1000)`.

After the main Vlasov loop and after `tdens` is built, replace the existing trajectory density plot block with three separate sets of PNG outputs as described below.

---

## Plot 1 — Ions Only (Red)

```cpp
GeomPlotter gplotter_ions(geom);
gplotter_ions.set_size(2048, 2048);
gplotter_ions.set_epot(&epot);
gplotter_ions.set_particle_database(&pdb);
gplotter_ions.set_particle_div(1000, 0);
gplotter_ions.set_qm_discretation(false);  // single colour — defaults to red Vec3D(1.0, 0.2, 0.2)
gplotter_ions.set_fieldgraph_plot(FIELD_TRAJDENS);
gplotter_ions.set_trajdens(&tdens);
```

Save XY, XZ, YZ views to:
- `run + "trajdens_ions_xy.png"`
- `run + "trajdens_ions_xz.png"`
- `run + "trajdens_ions_yz_x0.png"`

Use the same view and range settings as the existing trajectory plots:
- XY: `VIEW_XY, -1`, ranges `-0.05, -0.05, 0.05, 0.05`
- XZ: `VIEW_XZ, -1`, ranges `-0.05, -0.05, 0.05, 0.05`
- YZ: `VIEW_YZ, 105`, ranges `-0.05, -0.05, 0.05, 0.05`

---

## Plot 2 — Electrons Only (Blue)

Build a trajectory density field for electrons:
```cpp
MeshScalarField tdens_e(geom);
pdb_e.build_trajectory_density_field(tdens_e);
```

Create a separate plotter:
```cpp
GeomPlotter gplotter_electrons(geom);
gplotter_electrons.set_size(2048, 2048);
gplotter_electrons.set_epot(&epot);
gplotter_electrons.set_particle_database(&pdb_e);
gplotter_electrons.set_particle_div(1000, 0);
gplotter_electrons.set_qm_discretation(false);
gplotter_electrons.set_fieldgraph_plot(FIELD_TRAJDENS);
gplotter_electrons.set_trajdens(&tdens_e);
```

For the electron colour: check `particlegraph.hpp` for `clear_colors()` and `add_color(const Vec3D &color)`. If these are accessible via the GeomPlotter/GeomPlot interface, use:
```cpp
// blue: Vec3D(0.2, 0.2, 1.0)
```

If not directly accessible via GeomPlotter, use `set_qm_discretation(true)` — electrons (charge -1) will get a different colour index to ions (charge +1) from the default colour cycle.

Save to:
- `run + "trajdens_electrons_xy.png"`
- `run + "trajdens_electrons_xz.png"`
- `run + "trajdens_electrons_yz_x0.png"`

---

## Plot 3 — Superimposed Ions + Electrons

Check `geomplotter.hpp` and `geomplot.hpp` for whether the underlying `Frame` is accessible via a `get_frame()` or similar method. If it is, construct two `ParticleGraph` objects directly:

```cpp
// Ion graph — red
ParticleGraph pg_ions(geom, pdb, 1000, 0, false);
pg_ions.clear_colors();
pg_ions.add_color(Vec3D(1.0, 0.2, 0.2));  // red

// Electron graph — blue  
ParticleGraph pg_electrons(geom, pdb_e, 1000, 0, false);
pg_electrons.clear_colors();
pg_electrons.add_color(Vec3D(0.2, 0.2, 1.0));  // blue
```

Add both to the same frame and plot.

If direct frame access is not available via GeomPlotter, produce the superimposed plot in postprocessing instead — see the postprocessing task below.

Save to:
- `run + "trajdens_combined_xy.png"`
- `run + "trajdens_combined_xz.png"`
- `run + "trajdens_combined_yz_x0.png"`

---

## Postprocessing: Superimposed Plot (Fallback)

If the superimposed C++ plot is not feasible, write a Python script `postprocessscripts/trajdens_combined.py` that:

1. Reads `runN/trajdens_ions_xz.png` and `runN/trajdens_electrons_xz.png` using PIL
2. Composites them using matplotlib with the ion image in red channel and electron image in blue channel
3. Saves the result to `runN/plots/trajdens_combined_xz.png`
4. Repeats for XY and YZ views
5. Follows `run = 'run9/'` pattern so `setrun.sh` can update it

Add to `setrun.sh`:
```bash
sed -i "s|run = '[^']*'|run = '${RUN}/'|" postprocessscripts/trajdens_combined.py
```

Add to `postprocess.sh`:
```bash
python postprocessscripts/trajdens_combined.py
```

---

## Important Notes

- Do not change any other part of `fusorsim.cpp`
- Do not change any parameter values
- Verify all method signatures from headers before writing code
- If any method does not exist in the headers, report it and use the fallback approach
