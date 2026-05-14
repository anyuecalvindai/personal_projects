/*
*PIC Vlasov-Poisson Iterator
*Goal: produce a convergent potential map for a tee-fusor system to see if we can get a relatively uniform field
*using a second anode grid, and get a double potential well setup
*Second goal: see if it works with a fully spherical chamber and grids.


axes:
x-side tube
y-vertical tube diameter along long tube
z-long tube
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
#include "meshvectorfield.hpp"
#include "ibsimu.hpp"
#include "error.hpp"
#include "particlediagplotter.hpp"
#include "geomplotter.hpp"
#include "config.h"
#include "stl_solid.hpp"
#include "particles.hpp"

using namespace std;

const double beam_current = 2.0 *1e-3;
const long N_clouds = 10000;
const double IQ = beam_current/((double) N_clouds);
const double m = 28.0134;
const double q = 1;

double h = 0.0003;
double cathodepot = -5000.0;
double anodepot = 0.0; 
double anode_r = 15*1.33 * 1e-3;

double sim_x = 0.048+0.03175; 
double sim_y = 0.096;
double sim_z = 0.063;

int n_nodes_x = (int) (sim_x/h);
int n_nodes_y = (int) (sim_y/h);
int n_nodes_z = (int) (sim_z/h);

random_device rd1{};
mt19937 gen{rd1()};
normal_distribution<double> d1{0.0,1.0};  
double gaussian(){
    return d1(gen);
}

ParticleP3D sample(double rad, const Geometry &geom) {
    while (true) {
        double x = gaussian();
        double y = gaussian();
        double z = gaussian();  //define 3 gaussians for x y z components
        double normalisation = 1/pow(x*x + y*y + z*z, 0.5);  //normalisation factor Weisstein, Eric W. "Sphere Point Picking." From MathWorld--A Wolfram Resource. https://mathworld.wolfram.com/SpherePointPicking.html
        double x_f = x * normalisation * rad;  
        double y_f = y * normalisation * rad;
        double z_f = z * normalisation * rad;   

        int32_t i = (int32_t)((x_f - geom.origo(0)) / geom.h());   
        int32_t j = (int32_t)((y_f - geom.origo(1)) / geom.h());
        int32_t k = (int32_t)((z_f - geom.origo(2)) / geom.h());

        if (geom.mesh(i, j, k) == 0){
            return ParticleP3D(0, x_f, 0.0, y_f, 0.0, z_f, 0.0);  //check for bad particle definitions. If inside a solid node, keep looping in while true until good definition
        }
    }
}

void add_particles(ParticleDataBase3D &pdb, Geometry &geom) {
    for (long i = 1; i <= N_clouds; i++ ){
        pdb.add_particle(IQ, 1, m, sample(anode_r, geom));
    }
}


double epot_max_error(const EpotField &epot, const EpotField &epot_old) {
    double max = 0.0;
    double norm2 = 0.0;
    int nc = epot.nodecount();
    for (int a = 0; a < nc; a++) {
        double diff = fabs(epot(a) - epot_old(a));
        if (diff > max)
            max = diff;
        norm2 += diff*diff;
    }
    ibsimu.message(1) << "Epot max error = " << max << " V\n";
    ibsimu.message(1) << "Epot L2 norm = " << sqrt(norm2) << " V\n";
    return max;
}

void sim(int argc, char **argv){
    string run = "run2/";

    string geom_fn = "geom.dat";
    ifstream is_geom(geom_fn.c_str());
    if (!is_geom.good())
        throw( Error( ERROR_LOCATION, (std::string)"couldn\'t open file \'" + geom_fn + "\'" ) );
    Geometry geom( is_geom );
    is_geom.close();
    geom.build_surface();

    EpotBiCGSTABSolver solver(geom);

    EpotField epot(geom);
    EpotField epot_old(geom); //for convergence evaluation. stores last value
    MeshScalarField scharge(geom);
    MeshVectorField bfield;

    EpotEfield efield(epot);
    
    ParticleDataBase3D pdb(geom);
    pdb.set_surface_collision(true);

    Convergence conv;
    conv.add_epot(epot);
    conv.add_scharge(scharge);

    solver.set_eps(0.05);

    for (int j = 1; j <= 2; j++){
        solver.solve(epot, scharge);
        efield.recalculate();
        pdb.clear();
        add_particles(pdb, geom);
        pdb.iterate_trajectories(scharge, efield, bfield);
        conv.evaluate_iteration();
        if (j>1){
            epot_max_error(epot,epot_old);
        }
        epot_old = epot;
    }

    ofstream ofconv(run + "fusorsim_conv_1_10.dat");
    conv.print_history(ofconv);
    ofconv.close();

    MeshScalarField tdens(geom);
    pdb.build_trajectory_density_field(tdens);

    GeomPlotter gplotter(geom);
    gplotter.set_size(2048, 2048);
    gplotter.set_epot(&epot);
    gplotter.set_particle_database(&pdb);
    gplotter.set_particle_div(1);
    gplotter.set_trajdens(&tdens);
    gplotter.set_fieldgraph_plot(FIELD_TRAJDENS);

    gplotter.set_view(VIEW_XY, -1);
    gplotter.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter.plot_png((run + "trajdens_xy.png").c_str());

    gplotter.set_view(VIEW_XZ, -1);
    gplotter.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter.plot_png((run + "trajdens_xz.png").c_str());

    gplotter.set_view(VIEW_YZ, 105);
    gplotter.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter.plot_png((run + "trajdens_yz_x0.png").c_str());

    gplotter.set_particle_database(NULL);
    gplotter.set_trajdens(NULL);
    gplotter.set_fieldgraph_plot(FIELD_EPOT);

    gplotter.set_view(VIEW_XY, -1);
    gplotter.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter.plot_png((run + "epot_xy.png").c_str());

    gplotter.set_view(VIEW_XZ, -1);
    gplotter.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter.plot_png((run + "epot_xz.png").c_str());

    gplotter.set_view(VIEW_YZ, 105);
    gplotter.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter.plot_png((run + "epot_yz_x0.png").c_str());

    ofstream otraj(run + "trajectory_lengths.dat");
    otraj << "# particle    length (m)\n";
    for (size_t i = 0; i < pdb.size(); i++) {
        otraj << setw(6) << i << " " << setw(14) << pdb.traj_length(i) << "\n";
    }
    otraj.close();

    const uint32_t cx = 105;
    const uint32_t cy = 160;
    const uint32_t cz = 105;

    ofstream opot_z(run + "potential_radial_z.dat");
    opot_z << "# r (m)    potential (V)\n";
    for (uint32_t c = cz; c < geom.size(2); c++) {
        double r = (c - cz) * h;
        opot_z << setw(14) << r << " " << setw(14) << epot(cx, cy, c) << "\n";
    }
    opot_z.close();

    ofstream opot_y(run + "potential_radial_y.dat");
    opot_y << "# r (m)    potential (V)\n";
    for (uint32_t b = cy; b < geom.size(1); b++) {
        double r = (b - cy) * h;
        opot_y << setw(14) << r << " " << setw(14) << epot(cx, b, cz) << "\n";
    }
    opot_y.close();

    ofstream opot_x(run + "potential_radial_x.dat");
    opot_x << "# r (m)    potential (V)\n";
    for (uint32_t a = cx; a < geom.size(0); a++) {
        double r = (a - cx) * h;
        opot_x << setw(14) << r << " " << setw(14) << epot(a, cy, cz) << "\n";
    }
    opot_x.close();

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

    return(0);
}