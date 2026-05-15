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
#include "particlegraph.hpp"
#include "config.h"
#include "stl_solid.hpp"
#include "particles.hpp"

using namespace std;

const double beam_current = 50.000;   // 50A ions
const long N_clouds = 800000;
const double IQ = beam_current/((double) N_clouds);
//const double m = 28.0134; //nitrogen molecule
const double m = 2.0141017778; //deuterium

const double beam_current_e = 50.000; // 50A electrons
const long N_clouds_e = N_clouds;
const double IQ_e = beam_current_e / ((double) N_clouds_e);
const double m_e = 0.000548579909;   // electron mass in amu

const double q = 1;

double h = 0.0003;
double cathodepot = -100000.0;            // -100kV cathode
double anodepot = 0.0;
double anode_r = 15*1.33 * 1e-3;
const double cathode_r = 0.005;      // 5mm cathode radius

double sim_x = 0.048+0.03175; 
double sim_y = 0.096;
double sim_z = 0.063;

int n_nodes_x = (int) (sim_x/h);
int n_nodes_y = (int) (sim_y/h);
int n_nodes_z = (int) (sim_z/h);

const uint32_t cx = 105;
const uint32_t cy = 160;
const uint32_t cz = 105;

int n_iter = 20;

random_device rd1{};
mt19937 gen{rd1()};
normal_distribution<double> d1{0.0,1.0};  
double gaussian(){
    return d1(gen);
}

ParticleP3D sample(double rad, const Geometry &geom) { //uniform sampling on surface of sphere.
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

void add_ions(ParticleDataBase3D &pdb, Geometry &geom) {
    for (long i = 1; i <= N_clouds; i++ ){
        pdb.add_particle(IQ, 1, m, sample(anode_r, geom)); //add all particles to pdb
    }
}

void add_electrons(ParticleDataBase3D &pdb_e, Geometry &geom) {
    for (long i = 1; i <= N_clouds_e; i++) {
        pdb_e.add_particle(IQ_e, -1, m_e, sample(cathode_r, geom));
    }
}


double epot_max_error(const EpotField &epot, const EpotField &epot_old) {//compute maximum error between this run and last run's epot for all mesh nodes. 
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

// void snapshot(ParticleDataBase3D &pdb, string fn, int step) {
//     ofstream of(fn.c_str());
//     for (size_t i = 0; i < pdb.size(); i++) {
//         Particle3D p = pdb.particle(i);
//         if (p.get_status() != PARTICLE_OK) continue;
//         of << p[1] << " " << p[3] << " " << p[5] << "\n";
//     }
// }

void sim(int argc, char **argv){
    string run = "run23/";

    string geom_fn = "geom.dat";
    ifstream is_geom(geom_fn.c_str());
    if (!is_geom.good())
        throw( Error( ERROR_LOCATION, (std::string)"couldn\'t open file \'" + geom_fn + "\'" ) );
    Geometry geom( is_geom );
    is_geom.close();
    geom.build_surface();
    geom.set_boundary(8, Bound(BOUND_DIRICHLET, cathodepot));

    EpotBiCGSTABSolver solver(geom);

    EpotField epot(geom);
    EpotField epot_old(geom);
    MeshScalarField scharge(geom);
    MeshVectorField bfield;

    EpotEfield efield(epot);
    
    ParticleDataBase3D pdb(geom);
    pdb.set_surface_collision(true);

    ParticleDataBase3D pdb_e(geom); 
    pdb_e.set_surface_collision(true);

    Convergence conv;
    conv.add_epot(epot);
    conv.add_scharge(scharge);

    solver.set_eps(0.05);

    solver.solve(epot, scharge);
    efield.recalculate();


    //PLOTTING + DATA FOR INITIAL POTENTIAL

    GeomPlotter gplotter(geom);
    gplotter.set_size(2048, 2048);
    gplotter.set_epot(&epot);
    gplotter.set_fieldgraph_plot(FIELD_EPOT);

    gplotter.set_view(VIEW_XY, -1);
    gplotter.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter.plot_png((run + "epot_xy_initial.png").c_str());

    gplotter.set_view(VIEW_XZ, -1);
    gplotter.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter.plot_png((run + "epot_xz_initial.png").c_str());

    gplotter.set_view(VIEW_YZ, 105);
    gplotter.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter.plot_png((run + "epot_yz_x0_initial.png").c_str());
    
    double error = 1e10; //large initial number to make sure solver iterates twice.
    int j = 1;
    double error_thresh = 0.001 * fabs(cathodepot); //volts. corresponds to 0.1% error with 5kV


    ofstream osteps(run + "trajectory_steps.dat");
    osteps << "# iteration    total_steps\n";

    ofstream opot_iter_z(run + "potential_radial_z_all.dat");  //open z radial pot datastream
    opot_iter_z << "# iteration    r (m)    potential (V)\n";

    ofstream opot_iter_y(run + "potential_radial_y_all.dat");//open y radial pot datastream
    opot_iter_y << "# iteration    r (m)    potential (V)\n";

    ofstream opot_iter_x(run + "potential_radial_x_all.dat");//open x radial pot datastream
    opot_iter_x << "# iteration    r (m)    potential (V)\n";

    ofstream oerr(run + "epot_error.dat"); //open epot_error convergence datastream
    oerr << "# iteration    epot_max_error (V)    epot_L2_norm (V)\n";




    //MAIN SELF-CONSISTENT LOOP
    // Require 3 consecutive iterations below error_thresh to confirm
    // a stable convergence (not just a one-off lucky match).
    int converged_count = 0;
    while (j <= 20) {
        solver.solve(epot, scharge);
        efield.recalculate();
        scharge.clear();
        pdb.clear();
        pdb_e.clear();
        add_ions(pdb, geom);
        add_electrons(pdb_e, geom);
        pdb.iterate_trajectories(scharge, efield, bfield);
        pdb_e.iterate_trajectories(scharge, efield, bfield);
        osteps << setw(10) << j << " " << setw(20) << pdb.get_statistics().sum_steps() << "\n";
        conv.evaluate_iteration();

        for (int32_t c = -(int32_t)cz; c < (int32_t)geom.size(2) - (int32_t)cz; c++) { //export z radial pot to .dat for this iteration
        double r = c * h;
        opot_iter_z << setw(10) << j << " " << setw(14) << r << " " << setw(14) << epot(cx, cy, cz + c) << "\n";
        }

        for (int32_t c = -(int32_t)cy; c < (int32_t)geom.size(1) - (int32_t)cy; c++) { //export y radial pot to .dat for this iteration
            double r = c * h;
            opot_iter_y << setw(10) << j << " " << setw(14) << r << " " << setw(14) << epot(cx, cy + c, cz) << "\n";
        }

        for (int32_t c = -(int32_t)cx; c < (int32_t)geom.size(0) - (int32_t)cx; c++) { //export x radial pot to .dat for this iteration
            double r = c * h;
            opot_iter_x << setw(10) << j << " " << setw(14) << r << " " << setw(14) << epot(cx + c, cy, cz) << "\n";
        }

        if (j > 1){
            error = epot_max_error(epot, epot_old);
            oerr << setw(10) << j << " " << setw(20) << error << "\n";
            if (error <= error_thresh) {
                converged_count++;
            } else {
                converged_count = 0;
            }
            if (converged_count >= 3) {
                epot_old = epot;
                j++;
                break;
            }
        }

        epot_old = epot;
        j++;
    }
    osteps.close();
    opot_iter_z.close(); //close data streams
    opot_iter_y.close();
    opot_iter_x.close();
    oerr.close();

    

    ofstream ofconv(run + "fusorsim_conv_1_10.dat");
    conv.print_history(ofconv);
    ofconv.close();

    MeshScalarField tdens(geom);
    pdb.build_trajectory_density_field(tdens);

    // Plot 1: Ions only (default colour - red)
    GeomPlotter gplotter_ions(geom);
    gplotter_ions.set_size(2048, 2048);
    gplotter_ions.set_epot(&epot);
    gplotter_ions.set_particle_database(&pdb);
    gplotter_ions.set_particle_div(1000, 0);
    gplotter_ions.set_qm_discretation(false);
    gplotter_ions.set_fieldgraph_plot(FIELD_TRAJDENS);
    gplotter_ions.set_trajdens(&tdens);

    gplotter_ions.set_view(VIEW_XY, -1);
    gplotter_ions.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter_ions.plot_png((run + "trajdens_ions_xy.png").c_str());

    gplotter_ions.set_view(VIEW_XZ, -1);
    gplotter_ions.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter_ions.plot_png((run + "trajdens_ions_xz.png").c_str());

    gplotter_ions.set_view(VIEW_YZ, 105);
    gplotter_ions.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter_ions.plot_png((run + "trajdens_ions_yz_x0.png").c_str());

    // Plot 2: Electrons only - qm_discretation(true) so electron colour index
    // differs from ion colour index (cannot set explicit blue: clear_colors/
    // add_color are on ParticleGraph but not exposed via GeomPlotter).
    MeshScalarField tdens_e(geom);
    pdb_e.build_trajectory_density_field(tdens_e);

    GeomPlotter gplotter_electrons(geom);
    gplotter_electrons.set_size(2048, 2048);
    gplotter_electrons.set_epot(&epot);
    gplotter_electrons.set_particle_database(&pdb_e);
    gplotter_electrons.set_particle_div(1000, 0);
    gplotter_electrons.set_qm_discretation(true);
    gplotter_electrons.set_fieldgraph_plot(FIELD_TRAJDENS);
    gplotter_electrons.set_trajdens(&tdens_e);

    gplotter_electrons.set_view(VIEW_XY, -1);
    gplotter_electrons.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter_electrons.plot_png((run + "trajdens_electrons_xy.png").c_str());

    gplotter_electrons.set_view(VIEW_XZ, -1);
    gplotter_electrons.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter_electrons.plot_png((run + "trajdens_electrons_xz.png").c_str());

    gplotter_electrons.set_view(VIEW_YZ, 105);
    gplotter_electrons.set_ranges(-0.05, -0.05, 0.05, 0.05);
    gplotter_electrons.plot_png((run + "trajdens_electrons_yz_x0.png").c_str());

    // Plot 3 (combined ions + electrons) is produced by
    // postprocessscripts/trajdens_combined.py since GeomPlotter does not
    // expose direct ParticleGraph colour control.

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

    // centre node indices
    //x (0 -(-0.03175)) / 0.0003 = 105  (side tube)
    //y  (0 -(-0.048))   / 0.0003 = 160  (vertical diameter of long tube)
    //z(0 - (-0.03175)) / 0.0003 = 105  (long tube)

    //radial scan along z (lengthways) 
    ofstream opot_z(run + "potential_radial_z.dat");
    opot_z << "# r (m)    potential (V)\n";
    for (int32_t c = -(int32_t)cz; c < (int32_t)geom.size(2) - (int32_t)cz; c++) {
        double r = c * h;
        opot_z << setw(14) << r << " " << setw(14) << epot(cx, cy, cz + c) << "\n";
    }
    opot_z.close();

    //radial scan along y axis (vertical tube) 
    ofstream opot_y(run + "potential_radial_y.dat");
    opot_y << "# r (m)    potential (V)\n";
    for (int32_t b = -(int32_t)cy; b < (int32_t)geom.size(1) - (int32_t)cy; b++) {
        double r = b * h;
        opot_y << setw(14) << r << " " << setw(14) << epot(cx, cy + b, cz) << "\n";
    }
    opot_y.close();





    //radial scan along x axis (side tube)
    ofstream opot_x(run + "potential_radial_x.dat");
    opot_x << "# r (m)    potential (V)\n";
    for (int32_t a = -(int32_t)cx; a < (int32_t)geom.size(0) - (int32_t)cx; a++) {
        double r = a * h;
        opot_x << setw(14) << r << " " << setw(14) << epot(cx + a, cy, cz) << "\n";
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
