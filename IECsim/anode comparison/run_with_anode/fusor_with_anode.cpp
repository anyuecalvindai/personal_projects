/*
 * fusor_with_anode.cpp
 *
 * Generates a geometry containing only the DN63 tee chamber and cathode grid
 * (no anode grid), saves it to geom_cat_only.dat, then performs a
 * vacuum Laplace solve and dumps potential maps + radial profiles
 * to run_with_anode/ for direct comparison against the standard geom.dat
 * (tee + cathode + anode) under identical solver settings.
 *
 * Output files (run_with_anode/):
 *   geom_cat_only.dat
 *   epot_xy_with_anode.png
 *   epot_xz_with_anode.png
 *   epot_yz_x0_with_anode.png
 *   potential_radial_x_with_anode.dat
 *   potential_radial_y_with_anode.dat
 *   potential_radial_z_with_anode.dat
 */
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <random>
#include <map>
#include <string>
#include <cmath>

#include "epot_gssolver.hpp"
#include "epot_mgsolver.hpp"
#include "epot_bicgstabsolver.hpp"
#include "particledatabase.hpp"
#include "geometry.hpp"
#include "convergence.hpp"
#include "func_solid.hpp"
#include "epot_efield.hpp"
#include "meshvectorfield.hpp"
#include "ibsimu.hpp"
#include "error.hpp"
#include "geomplotter.hpp"
#include "config.h"
#include "stl_solid.hpp"
#include "particles.hpp"

using namespace std;

double h = 0.0003;
double cathodepot = -10000.0;          // -10 kV cathode for direct comparison with run25
double anode_r = 15*1.33 * 1e-3;       // ~19.95 mm (unused here, retained for context)
const double cathode_r = 0.005;

double sim_x = 0.048+0.03175;
double sim_y = 0.096;
double sim_z = 0.063;

int n_nodes_x = (int) (sim_x/h);
int n_nodes_y = (int) (sim_y/h);
int n_nodes_z = (int) (sim_z/h);

// Centre indices (same as standard fusorsim.cpp)
const uint32_t cx = 105;
const uint32_t cy = 160;
const uint32_t cz = 105;

void sim(int argc, char **argv){
    string run = "run_with_anode/";

    // --- Build the WITH-anode geometry, matching fusor_with_anode but
    //     including the anode grid. Same STL files in cad_70_transp/.   ---
    Geometry geom(MODE_3D,
                  Int3D(n_nodes_x, n_nodes_y, n_nodes_z),
                  Vec3D(-0.03175, -0.048, -0.03175),
                  h);

    Solid *s1 = new STLSolid("cad_70_transp/tee_or.stl");
    geom.set_solid(7, s1);
    Solid *s2 = new STLSolid("cad_70_transp/cathodegridnostalk_or.stl");
    geom.set_solid(8, s2);
    Solid *s3 = new STLSolid("cad_70_transp/anodegrid_or.stl");
    geom.set_solid(9, s3);

    for (uint32_t i = 1; i <= 6; i++) {
        geom.set_boundary(i, Bound(BOUND_NEUMANN, 0.0));
    }
    geom.set_boundary(7, Bound(BOUND_DIRICHLET, 0.0));            // chamber wall
    geom.set_boundary(8, Bound(BOUND_DIRICHLET, cathodepot));     // cathode grid
    geom.set_boundary(9, Bound(BOUND_DIRICHLET, 0.0));            // anode grid

    geom.build_mesh();
    geom.build_surface();

    geom.save("geom_with_anode.dat", true);

    // --- Laplace solve (no space charge) ---
    EpotBiCGSTABSolver solver(geom);
    EpotField epot(geom);
    MeshScalarField scharge(geom);                // remains zero
    EpotEfield efield(epot);

    solver.set_eps(0.05);
    solver.solve(epot, scharge);
    efield.recalculate();

    // --- Potential field plots (clearly labelled as "no anode") ---
    GeomPlotter gplotter(geom);
    gplotter.set_size(2048, 2048);
    gplotter.set_epot(&epot);
    gplotter.set_fieldgraph_plot(FIELD_EPOT);

    gplotter.set_view(VIEW_XY, -1);
    gplotter.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter.plot_png((run + "epot_xy_with_anode.png").c_str());

    gplotter.set_view(VIEW_XZ, -1);
    gplotter.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter.plot_png((run + "epot_xz_with_anode.png").c_str());

    gplotter.set_view(VIEW_YZ, 105);
    gplotter.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter.plot_png((run + "epot_yz_x0_with_anode.png").c_str());

    // --- Radial potential profiles along each axis ---
    {
        ofstream opot_z(run + "potential_radial_z_with_anode.dat");
        opot_z << "# r (m)    potential (V)    [no-anode geometry, cathode at " << cathodepot << " V]\n";
        for (int32_t c = -(int32_t)cz; c < (int32_t)geom.size(2) - (int32_t)cz; c++) {
            double r = c * h;
            opot_z << setw(14) << r << " " << setw(14) << epot(cx, cy, cz + c) << "\n";
        }
        opot_z.close();
    }
    {
        ofstream opot_y(run + "potential_radial_y_with_anode.dat");
        opot_y << "# r (m)    potential (V)    [no-anode geometry, cathode at " << cathodepot << " V]\n";
        for (int32_t b = -(int32_t)cy; b < (int32_t)geom.size(1) - (int32_t)cy; b++) {
            double r = b * h;
            opot_y << setw(14) << r << " " << setw(14) << epot(cx, cy + b, cz) << "\n";
        }
        opot_y.close();
    }
    {
        ofstream opot_x(run + "potential_radial_x_with_anode.dat");
        opot_x << "# r (m)    potential (V)    [no-anode geometry, cathode at " << cathodepot << " V]\n";
        for (int32_t a = -(int32_t)cx; a < (int32_t)geom.size(0) - (int32_t)cx; a++) {
            double r = a * h;
            opot_x << setw(14) << r << " " << setw(14) << epot(cx + a, cy, cz) << "\n";
        }
        opot_x.close();
    }
}

int main(int argc, char **argv){
    try {
        ibsimu.set_message_threshold(MSG_VERBOSE, 1);
        ibsimu.set_thread_count(4);
        sim(argc, argv);
    } catch (Error e) {
        e.print_error_message(ibsimu.message(0));
        exit(1);
    }
    return 0;
}
