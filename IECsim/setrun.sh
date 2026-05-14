#!/bin/bash
RUN=$1
mkdir -p ${RUN}/plots
mkdir -p ${RUN}/timestep_pics
mkdir -p ${RUN}/pout
sed -i "s/string run = \"[^\"]*\"/string run = \"${RUN}\/\"/" fusorsim.cpp
sed -i "s|run = '[^']*'|run = '${RUN}/'|" postprocessscripts/traj_distribution.py
sed -i "s|run = '[^']*'|run = '${RUN}/'|" postprocessscripts/pot_radial_all.py
sed -i "s|run = '[^']*'|run = '${RUN}/'|" postprocessscripts/pot_error.py
sed -i "s|run = '[^']*'|run = '${RUN}/'|" postprocessscripts/centralheight.py
sed -i "s|run = '[^']*'|run = '${RUN}/'|" postprocessscripts/epot_error_rms.py
sed -i "s|run = '[^']*'|run = '${RUN}/'|" postprocessscripts/memory_estimate.py
sed -i "s|run = '[^']*'|run = '${RUN}/'|" postprocessscripts/timestep_animation.py
sed -i "s|run = '[^']*'|run = '${RUN}/'|" postprocessscripts/particle_animation.py
sed -i "s|run = '[^']*'|run = '${RUN}/'|" postprocessscripts/trajdens_combined.py
