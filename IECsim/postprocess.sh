#!/bin/bash
RUN=$1
if [ -z "${RUN}" ]; then
    echo "usage: $0 <run_dir>" >&2
    exit 1
fi
source venv/bin/activate
cp fusorsim.cpp ${RUN}/fusorsimrun${RUN}.cpp
python postprocessscripts/pot_radial_all.py
python postprocessscripts/traj_distribution.py
python postprocessscripts/pot_error.py
python postprocessscripts/centralheight.py
python postprocessscripts/epot_error_rms.py
python postprocessscripts/memory_estimate.py
python postprocessscripts/timestep_animation.py
python postprocessscripts/particle_animation.py
