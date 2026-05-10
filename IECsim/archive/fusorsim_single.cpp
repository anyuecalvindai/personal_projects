/*
*single particle tracker
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

//physical parameters
//const double u = 1.66053906892; //atomic mass unit
const double T_0 = 20.0 + 273.0;//starting temperature
const double V = 337126.98779 * 1e-9; //calculated from CAD
const double R_const = 8.314; //(joules per mole per kelvin)
const double k = 1.3806488 * 1e-23; //boltzmann constant
const double p_mbar = 9 * 1e-3; 
const double n = p_mbar/(k*T_0); //number density
const long N_lab = round(n*V); //lab no. of particles. comes out as 750041320067570 particles: too many.
// can simplify by treating particles as "clouds" (Vlasov 1950)
const double beam_current = 2.0 *1e-3;
const long N_clouds = 1;
const double IQ = beam_current/((double) N_clouds);
const double m = 28.0134; // u
const double q = 1; //e

// //Thermal gas molecule speed calculations
// const double a = sqrt(k*T_0/m);

double h = 0.0003; //mesh cell size
double cathodepot = -5000.0; //cathode and anode potentials in volts
double anodepot = 0.0; 
double anode_r = 15*1.33 * 1e-3; //(roughly 19.95mm)


double sim_x = 0.048+0.03175; 
double sim_y = 0.096;  //measured half
double sim_z = 0.063;  //measured half (radius)

int n_nodes_x = (int) (sim_x/h);
int n_nodes_y = (int) (sim_y/h);
int n_nodes_z = (int) (sim_z/h);


random_device rd1{};
mt19937 gen{rd1()}; //mersenne-twister-engine: random number generator. 
normal_distribution<double> d1{0.0,1.0};  
double gaussian(){
    return d1(gen);  //pluck a random number from the gaussian
}

ParticleP3D sample(double rad){ //samples a point on a sphere of radius r
    double x = gaussian();
    double y = gaussian();
    double z = gaussian();
    double normalisation = 1/pow(x*x + y*y + z*z ,0.5); //Muller 1959, Marsaglia 1972, as seen in Weisstein (Wolfram Mathworld)
    double x_f = x * normalisation * rad;
    double y_f = y * normalisation * rad;
    double z_f = z * normalisation * rad;
    //return Vec3D(x * normalisation * anode_r, y * normalisation * anode_r, z * normalisation * anode_r);

    return ParticleP3D(0, x_f, 0.0 , y_f, 0.0, z_f, 0.0);
}


void sim(int argc, char **argv){
    // string geom_fn = "geom.dat";
    // ifstream is_geom(geom_fn.c_str());
    // if (!is_geom.good())
    //     throw( Error( ERROR_LOCATION, (std::string)"couldn\'t open file \'" + geom_fn + "\'" ) );
    // Geometry geom( is_geom );
    // is_geom.close();
    //geom.build_surface();
    Geometry geom(MODE_3D, Int3D(n_nodes_x, n_nodes_y, n_nodes_z), Vec3D(-0.03175,-0.048,-0.03175), h); //define geometry. cuboid with same x,y,z dimensions as tee. 

    Solid *s1 = new STLSolid("dn63 tee.stl");
    geom.set_solid(7,s1);
    Solid *s2 = new STLSolid("cathodegridnostalk.stl");
    geom.set_solid(8,s2);
    Solid *s3 = new STLSolid("anodegrid_correct_wire.stl");
    geom.set_solid(9,s3);
    
    for (uint32_t i = 1; i <= 6; i++){ //loop to set Neumann boundary condition for 6 simulation space faces
        geom.set_boundary(i, Bound(BOUND_NEUMANN, 0.0));
    }

    geom.set_boundary(7, Bound(BOUND_DIRICHLET, 0.0));
    geom.set_boundary(8, Bound(BOUND_DIRICHLET, cathodepot));
    geom.set_boundary(9, Bound(BOUND_DIRICHLET, anodepot));

    geom.build_mesh(); //create node mesh

    EpotBiCGSTABSolver solver(geom); //declare biconjugate gradient stabilized method for solver

    EpotField epot(geom); //declare epot field, space charge, b field objects
    MeshScalarField scharge(geom);
    MeshVectorField bfield; //empty: no b field limit 

    EpotEfield efield(epot); //declare e field
    ParticleDataBase3D pdb(geom);  //initialise particle database
    //pdb.set_surface_collision(true); 

    // Emittance emit;  //declare emittance statistics class. no idea about the math, but qualitatively its a measure of beam quality
    // Convergence conv;
    // conv.add_epot(epot);
    // conv.add_scharge(scharge);
    // conv.add_emittance(0, emit);

    //particle declaration loop
    /*
    * We randomly sample n points on a sphere as particles in our system. 
    * A
    */
    solver.solve(epot, scharge);
    efield.recalculate();

    for (long i = 1; i <= N_clouds; i++ ){
        pdb.add_particle(IQ, 1, m, sample(anode_r));
    }
    
    pdb.iterate_trajectories(scharge, efield, bfield);
    MeshScalarField tdens(geom);
    pdb.build_trajectory_density_field(tdens);

    GeomPlotter gplotter(geom);
    gplotter.set_size(2048, 2048);
    gplotter.set_view(VIEW_XZ, -1);
    gplotter.set_ranges(-0.03175, -0.03175, 0.04745, 0.03095);
    gplotter.set_epot(&epot);
    gplotter.set_particle_database(&pdb);
    gplotter.set_particle_div(1); // plot all particles
    gplotter.set_trajdens(&tdens);
    gplotter.set_fieldgraph_plot(FIELD_TRAJDENS);
    gplotter.plot_png("trajdens_xz_1_1.png");

    gplotter.set_particle_database(NULL);
    gplotter.set_trajdens(NULL);
    gplotter.set_fieldgraph_plot(FIELD_EPOT);

    gplotter.set_size(2048, 2048);
    gplotter.set_view(VIEW_XZ, -1);
    gplotter.set_epot(&epot);
    gplotter.plot_png("-1_xz_final_1_1.png");

}


int main( int argc, char **argv )
{
    try {
	ibsimu.set_message_threshold( MSG_VERBOSE, 1 );
	ibsimu.set_thread_count( 4 );
        sim( argc, argv );
    } catch ( Error e ) {
        e.print_error_message( ibsimu.message( 0 ) );
        exit( 1 );
    }

    return( 0 );
}