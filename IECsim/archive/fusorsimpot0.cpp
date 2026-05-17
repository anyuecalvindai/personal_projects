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

void sim(int argc, char **argv){
    string geom_fn = "geom.dat";
    ifstream is_geom(geom_fn.c_str());
    if (!is_geom.good())
        throw( Error( ERROR_LOCATION, (std::string)"couldn\'t open file \'" + geom_fn + "\'" ) );
    Geometry geom( is_geom );
    is_geom.close();
    EpotBiCGSTABSolver solver(geom); //declare biconjugate gradient stabilized method for solver

    EpotField epot(geom); //declare epot field, space charge, b field objects
    MeshScalarField scharge(geom);
    //MeshVectorField bfield; //empty: no b field limit 

    EpotEfield efield(epot); //declare e field
    solver.set_eps(0.05);
    solver.solve(epot, scharge);
    

    GeomPlotter gplotter(geom);
    gplotter.set_size(2048, 2048);
    gplotter.set_ranges(-0.05,-0.05, 0.05,0.05);
    gplotter.set_mesh(true);
    gplotter.set_epot(&epot);
    
    // gplotter.set_view(VIEW_XZ, -1);
    // gplotter.plot_png("-1_xz.png");

    // gplotter.set_view(VIEW_XY, -1);
    // gplotter.plot_png("-1_xy.png");

    gplotter.set_view(VIEW_YZ,-1);
    gplotter.plot_png("-1_yz_0.05eps_wider.png");
    
}

int main(int argc, char **argv) {
    try {
        ibsimu.set_message_threshold(MSG_VERBOSE, 1);
        ibsimu.set_thread_count(6);
        sim(argc, argv);
    } catch(Error e) {
        e.print_error_message(ibsimu.message(0));
        exit(1);
    }
    return 0;
}